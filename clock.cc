#include "clock.h"

#include <chrono>

namespace {

double current_time = 0.0;

}

double get_current_time()
{
	return current_time;
}

void update_current_time()
{
	current_time = (std::chrono::steady_clock::now() - std::chrono::steady_clock::time_point()) / std::chrono::milliseconds(1) * .001;
}
