#ifndef CONFIGFILE_H
#define CONFIGFILE_H

#include <map>
#include <string>

class config_file {
public:
	void
	write();

	void
	read();

	std::size_t
	unlocked_levels() const;

	void
	set_unlocked_levels(std::size_t value);

private:
	std::map<std::string, std::string> fields_;
};

#endif
