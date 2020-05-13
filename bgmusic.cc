#include "bgmusic.h"

#include <cmath>
#include <iostream>
#include <memory>

namespace {

double envelope(std::size_t attack, std::size_t sustain, std::size_t decay_e, std::size_t n) {
	if (n < attack) {
			return n * 1.0 / attack;
	} else if (n < attack + sustain) {
			return 1.0;
	} else {
			return 1.0 * exp( -1.0 * (n - attack - sustain) / decay_e);
	}
}

std::unique_ptr<float[]> synth(float freq, std::size_t num_samples)
{
	std::unique_ptr<float[]> samples(new float[num_samples]);
	for (std::size_t n = 0; n < num_samples; ++n) {
		samples[n] = sin(2 * M_PI * n * freq / 48000 + sin(2 * M_PI * n * freq * 1.001 / 48000) * 0.8) * envelope(1000, 0, 8000, n);
		if (n > num_samples - 256) {
			samples[n] *= (num_samples - n) / 256.;
		}
	}
	return samples;
}

}

bg_music_player::bg_music_player(audioplayer * pl)
	: pl_(pl)
	, stop_(false)
	, thread_([this](){thread_function();})
{
}

bg_music_player::~bg_music_player()
{
	stop_.store(true, std::memory_order_relaxed);
	thread_.join();
}

void
bg_music_player::thread_function()
{
	synthesize_patches();

	static const std::string pat = "aefgdfefgd" "aefgdfgaeg" "eaefgaeagf" "eBefgeBeBc" "eAefgeAeBA";

	for (;;) {
		std::size_t count = 0;
		for (char note : pat) {
			if (stop_) {
				return;
			}
			const auto& patch = patches_[std::string(1, note)];
			pl_->play(patch.get(), patch_length, (count % 5 == 0) ? 0.10 : 0.08);
			++count;
			usleep(150 * 1000 );
		}
	}
}

void
bg_music_player::synthesize_patches()
{
	struct pitch_t {
		const char * sym;
		float pitch;
	};

	static const pitch_t pitches[] = {
		{"C", -21}, {"D", -19}, {"E", -17}, {"F", -16}, {"G", -14}, {"A", -12}, {"B", -10},
		{"c", -9}, {"d", -7}, {"e", -5}, {"f", -4}, {"g", -2}, {"a", 0}, {"b", 2}
	};

	for (const pitch_t pitch : pitches) {
		patches_[pitch.sym] = synth(220 * exp(log(2) * pitch.pitch/12.), patch_length);
	}
}

