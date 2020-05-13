#include "audioplayer.h"

#include <pthread.h>

audioplayer::handle::~handle()
{
	reset();
}

audioplayer::handle::handle() noexcept
	: descr_(nullptr)
{}

audioplayer::handle::handle(const handle & other) noexcept
	: descr_(other.descr_)
{
	reset();
	if (descr_) {
		descr_->refcount.fetch_add(1, std::memory_order_relaxed);
	}
}

audioplayer::handle::handle(handle && other) noexcept
	: descr_(nullptr)
{
	swap(other);
}

audioplayer::handle & audioplayer::handle::operator=(const handle & other) noexcept
{
	handle tmp(other);
	swap(tmp);
	return *this;
}

audioplayer::handle & audioplayer::handle::operator=(handle && other) noexcept
{
	swap(other);
	return *this;
}

void
audioplayer::handle::swap(handle & other) noexcept
{
	std::swap(descr_, other.descr_);
}

void
audioplayer::handle::reset() noexcept
{
	if (descr_) {
		if (descr_->refcount.fetch_sub(1, std::memory_order_release) == 1) {
			std::atomic_thread_fence(std::memory_order_acquire);
			delete descr_;
		}
		descr_ = nullptr;
	}
}

audioplayer::handle::handle(stream_descriptor * descr)
	: descr_(descr)
{
}

audioplayer::~audioplayer()
{
	stop_device_and_thread();
}

audioplayer::audioplayer()
	: streams_(nullptr), expired_streams_(nullptr), frame_time_(0), exit_thread_(false)
{
	start_device_and_thread();
}


audioplayer::handle
audioplayer::play(const float * samples, std::size_t num_samples, float factor)
{
	clear_expired();

	stream_descriptor * descr = new stream_descriptor;
	descr->start_time = frame_time_.load(std::memory_order_relaxed);
	descr->num_samples = num_samples;
	descr->current_pos = 0;
	descr->samples = samples;
	descr->factor = factor;
	descr->state.store(stream_descriptor::active, std::memory_order_relaxed);
	descr->refcount.store(2, std::memory_order_relaxed);

	stream_descriptor * current_head = streams_.load(std::memory_order_relaxed);
	do {
		descr->next = current_head;
	} while (!streams_.compare_exchange_weak(current_head, descr, std::memory_order_release));

	return handle(descr);
}

void
audioplayer::stop(handle & hdl)
{
	stream_descriptor::state_t prev_state = hdl.descr_->state.load(std::memory_order_relaxed);
	while (prev_state == stream_descriptor::active) {
		if (hdl.descr_->state.compare_exchange_weak(prev_state, stream_descriptor::request_deactivate, std::memory_order_relaxed)) {
			break;
		}
	}

	hdl.reset();
	clear_expired();
}

namespace {

void
check_snd_error(const char * prefix, int error)
{
	if (error < 0) {
		throw std::runtime_error(std::string(prefix) + snd_strerror(error));
	}
}

}

void
audioplayer::start_device_and_thread()
{
	int error;

	/* Open the PCM device in playback mode */
	error = snd_pcm_open(&pcm_, "default", SND_PCM_STREAM_PLAYBACK, 0);
	check_snd_error("Open: ", error);

	snd_pcm_hw_params_t *params;

	/* Allocate parameters object and fill it with default values*/
	snd_pcm_hw_params_alloca(&params);

	snd_pcm_hw_params_any(pcm_, params);

	/* Set parameters */
	error = snd_pcm_hw_params_set_access(pcm_, params, SND_PCM_ACCESS_RW_INTERLEAVED);
	check_snd_error("hw_params_set_access: ", error);

	error = snd_pcm_hw_params_set_format(pcm_, params, SND_PCM_FORMAT_S16_LE);
	check_snd_error("hw_params_set_format: ", error);

	error =  snd_pcm_hw_params_set_channels(pcm_, params, 1);
	check_snd_error("hw_params_set_channels: ", error);

	sample_rate_ = 48000;
	error = snd_pcm_hw_params_set_rate_near(pcm_, params, &sample_rate_, 0);
	check_snd_error("hw_params_set_rate_near: ", error);

	period_size_ = 512;
	error = snd_pcm_hw_params_set_period_size_near(pcm_, params, &period_size_, 0);
	check_snd_error("hw_params_set_period_size_near: ", error);

	buffer_size_ = 2048;
	error = snd_pcm_hw_params_set_buffer_size_near(pcm_, params, &buffer_size_);
	check_snd_error("hw_params_set_buffer_size_near: ", error);

	error = snd_pcm_hw_params(pcm_, params);
	check_snd_error("hw_params: ", error);

	frame_time_.store(0, std::memory_order_relaxed);
	thread_ = std::thread([this](){thread_function();});

	/* try to set RT priority for audio thread, but accept if this fails */
	sched_param param;
	param.sched_priority = 1;
	pthread_setschedparam (thread_.native_handle(), SCHED_FIFO, &param);
}

void
audioplayer::stop_device_and_thread()
{
	exit_thread_.store(true, std::memory_order_relaxed);
	thread_.join();
	int error = snd_pcm_close(pcm_);
	check_snd_error("pcm_close: ", error);
}

audioplayer::stream_descriptor *
audioplayer::unlink_stream(stream_descriptor * prev, stream_descriptor * current, stream_descriptor * next)
{
	/* If this is not first element of list, then we can unlink
	 * very easily because no other thread can change anything but
	 * head of list. */
	if (prev) {
		prev->next = next;
		return prev;
	}

	/* Current element is head of list. Try to swap out, but guard against
	 * possibility that another thread is doing head insertion */
	stream_descriptor * tmp = current;
	if (streams_.compare_exchange_strong(tmp, next, std::memory_order_relaxed)) {
		return nullptr;
	}

	/* Another thread raced, performing head insertion. Need to traverse list
	 * from beginning to determine correct previous element. */
	prev = streams_.load(std::memory_order_relaxed);
	for (;;) {
		if (prev->next == current) {
			prev->next = next;
			return prev;
		}
		prev = tmp;
	}
}

void
audioplayer::thread_function()
{
	buffered_ = 0;

	std::unique_ptr<short[]> buffer(new short[period_size_]);
	std::unique_ptr<float[]> samples(new float[period_size_]);

	while (!exit_thread_.load(std::memory_order_relaxed)) {
		for (std::size_t n = 0; n < period_size_; ++n) {
			samples[n] = 0.;
		}

		stream_descriptor * prev = nullptr;
		stream_descriptor * current = streams_.load(std::memory_order_acquire);
		uint64_t frame_time = frame_time_.load(std::memory_order_relaxed);

		while (current) {
			std::size_t begin = current->start_time <= frame_time ? 0 : current->start_time - frame_time;
			if (begin < period_size_) {
				std::size_t count = std::min(current->num_samples - current->current_pos, period_size_ - begin);
				for (std::size_t n = 0; n < count; ++n) {
					samples[n + begin] += current->samples[n + current->current_pos] * current->factor;
				}
				current->current_pos += count;
			}
			stream_descriptor * next = current->next;

			bool deactivate =
				current->current_pos == current->num_samples ||
				current->state.load(std::memory_order_relaxed) == stream_descriptor::request_deactivate;
			if (deactivate) {
				current->state.store(stream_descriptor::inactive, std::memory_order_relaxed);
				prev = unlink_stream(prev, current, next);
				stream_descriptor * tmp = expired_streams_.load(std::memory_order_relaxed);
				do {
					current->next = tmp;
				} while (!expired_streams_.compare_exchange_weak(tmp, current, std::memory_order_release));
			} else {
				prev = current;
			}
			current = next;
		}

		for (std::size_t n = 0; n < period_size_; ++n) {
			float sample = samples[n];
			sample = std::max(std::min(sample, float(1.0)), float(-1.0));
			buffer[n] = 32767 * sample;
		}

		int error = snd_pcm_writei(pcm_, &buffer[0], period_size_);
		if (error < 0) {
			snd_pcm_prepare(pcm_);
			buffered_ = 0;
		} else {
			buffered_ += period_size_;
			if (buffered_ >= buffer_size_) {
				snd_pcm_start(pcm_);
			}
		}
		frame_time_.store(frame_time_.load(std::memory_order_relaxed) + period_size_, std::memory_order_relaxed);
	}
}

void
audioplayer::clear_expired()
{
	stream_descriptor * current = expired_streams_.exchange(nullptr, std::memory_order_acquire);
	while (current) {
		stream_descriptor * next = current->next;
		if (current->refcount.fetch_sub(1, std::memory_order_release) == 1) {
			std::atomic_thread_fence(std::memory_order_acquire);
			delete current;
		}
		current = next;
	}
}
