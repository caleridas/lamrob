#ifndef BGMUSIC_H
#define BGMUSIC_H

#include <string>
#include <thread>
#include <unordered_map>

#include "audioplayer.h"

void play_background_music(audioplayer * pl);

class bg_music_player {
public:
	explicit bg_music_player(audioplayer * pl);

	~bg_music_player();

private:
	void
	thread_function();

	void
	synthesize_patches();

	audioplayer * pl_;
	std::atomic<bool> stop_;
	std::thread thread_;

	std::unordered_map<std::string, std::unique_ptr<float[]>> patches_;

	static constexpr std::size_t patch_length = 24000;
};

#endif
