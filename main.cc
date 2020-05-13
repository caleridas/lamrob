#include <thread>

#include "audioplayer.h"
#include "background.h"
#include "bgmusic.h"
#include "clock.h"
#include "configfile.h"
#include "main_screen.h"
#include "puzzle.h"
#include "start_screen.h"
#include "view.h"

int main()
{
	update_current_time();
	srandom(time(nullptr));

	config_file cf;
	cf.read();

	audioplayer audio;
	application app;

	const std::vector<puzzle> & puzzles = get_puzzles();

	std::shared_ptr<background_renderer> bg = std::make_shared<background_renderer>();

	std::size_t current_level = 0;

	start_screen s(&app, bg);
	main_screen m(&app, bg);

	s.set_unlocked_levels(cf.unlocked_levels());

	s.set_start_level_handler(
		[&m, &app, &current_level, &puzzles](std::size_t level)
		{
			current_level = level;
			app.set_active_view(&m);
			m.start_level(puzzles[level]);
		});

	m.set_exit_main_screen_handler(
		[&s, &app]()
		{
			app.set_active_view(&s);
		});

	m.set_complete_level_handler(
		[&cf, &m, &s, &app, &current_level, &puzzles]()
		{
			++current_level;

			if (current_level > cf.unlocked_levels()) {
				cf.set_unlocked_levels(current_level);
				cf.write();

				s.set_unlocked_levels(current_level);
			}
			if (current_level < puzzles.size()) {
				m.start_level(puzzles[current_level]);
			} else {
				app.set_active_view(&s);
			}
		});

	bg_music_player music(&audio);
	app.set_active_view(&s);
	app.toggle_fullscreen();

	app.run();

	return 0;
}
