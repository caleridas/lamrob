#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include <vector>
#include <memory>

#include <atomic>
#include <mutex>
#include <thread>
#include <list>

#include <alsa/asoundlib.h>

class audioplayer {
private:
	struct stream_descriptor {
		enum state_t {
			active = 0,
			request_deactivate = 1,
			inactive = 2
		};

		uint64_t start_time;
		std::size_t num_samples;
		std::size_t current_pos;
		const float * samples;
		float factor;

		stream_descriptor * next;
		std::atomic<state_t> state;
		std::atomic<int> refcount;
	};

public:
	class handle {
	public:
		~handle();
		handle() noexcept;
		handle(const handle & other) noexcept;
		handle(handle && other) noexcept;

		handle &
		operator=(const handle & other) noexcept;

		handle &
		operator=(handle && other) noexcept;

		void
		swap(handle & other) noexcept;

		void
		reset() noexcept;

	private:
		handle(stream_descriptor * descr);

		stream_descriptor * descr_;

		friend class audioplayer;
	};

	~audioplayer();

	audioplayer();

	handle
	play(const float * samples, std::size_t num_samples, float factor = 1.0);

	void
	stop(handle & hdl);

private:
	stream_descriptor *
	unlink_stream(stream_descriptor * prev, stream_descriptor * current, stream_descriptor * next);

	void
	clear_expired();

	void
	start_device_and_thread();

	void
	stop_device_and_thread();

	void
	thread_function();

	/* playback mixer thread */
	std::thread thread_;

	/* configuration data */
	snd_pcm_t * pcm_;
	unsigned int sample_rate_;
	snd_pcm_uframes_t period_size_;
	snd_pcm_uframes_t buffer_size_;

	/* data shared between main / mixer thread */
	std::atomic<stream_descriptor *> streams_;
	std::atomic<stream_descriptor *> expired_streams_;
	std::atomic<uint64_t> frame_time_;
	std::atomic<bool> exit_thread_;

	/* data private to mixer thread */
	snd_pcm_uframes_t buffered_;
};

#endif
