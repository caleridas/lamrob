#include "configfile.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <memory>
#include <sstream>

namespace {

template<typename T>
T
get_field(
	const std::map<std::string, std::string> & fields,
	const char * name,
	T default_value)
{
	T value = default_value;
	auto i = fields.find(name);
	if (i != fields.end()) {
		std::istringstream(i->second) >> value;
	}

	return value;
}

template<typename T>
void
put_field(
	std::map<std::string, std::string> & fields,
	const char * name,
	T value)
{
	std::ostringstream os;
	os << value;
	fields[name] = os.str();
}

std::string
serialize_fields(
	const std::map<std::string, std::string> & fields)
{
	std::string s;
	for (const auto & item : fields) {
		s = s + item.first + "=" + item.second + "\n";
	}
	return s;
}

std::map<std::string, std::string>
deserialize_fields(
	const std::string & input)
{
	std::map<std::string, std::string> fields;

	std::istringstream is(input);

	std::string line;
	while (std::getline(is, line, '\n')) {
		std::istringstream ls(line);

		std::string key, value;
		if (!std::getline(ls, key, '=')) {
			continue;
		}
		if (!std::getline(ls, value, '\n')) {
			continue;
		}

		fields[key] = value;
	}
	return fields;
}

static const char * path_parts[] = {
	".local",
	"share",
	"lamrob"
};

bool
make_config_file_path()
{
	static bool path_creation_attempted = false;
	static bool path_creation_success = false;

	if (!path_creation_attempted) {
		path_creation_attempted = true;

		char * home = getenv("HOME");
		if (home && *home) {
			bool success = true;
			path_creation_success = true;
			std::string path(home);
			for (const char * part : path_parts) {
				path = path + "/" + part;
				if (!::mkdir(path.c_str(), 0777)) {
					if (errno != EEXIST) {
						success = false;
					}
					break;
				}
			}
			path_creation_success = success;
		}
	}

	return path_creation_success;
}

std::string
build_config_filename()
{
	char * home = getenv("HOME");
	if (!home || !*home) {
		return "";
	}

	std::string path(home);
	for (const char * part : path_parts) {
		path = path + "/" + part;
	}

	path += "/config.ini";

	return path;
}

const std::string &
get_config_filename()
{
	static std::string config_filename = build_config_filename();
	return config_filename;
}

}

#include <iostream>
#include <string.h>

void
config_file::write()
{
	if (!make_config_file_path()) {
		return;
	}

	int fd = ::open(get_config_filename().c_str(), O_RDWR | O_CREAT | O_TRUNC | O_CLOEXEC, 0666);
	if (fd < 0) {
		return;
	}

	std::string s = serialize_fields(fields_);
	::write(fd, s.c_str(), s.size());
	::close(fd);
}

void
config_file::read()
{
	fields_.clear();

	int fd = ::open(get_config_filename().c_str(), O_RDONLY);
	if (fd < 0) {
		return;
	}

	std::string data;
	std::unique_ptr<char[]> buffer(new char[4096]);
	for(;;) {
		ssize_t count = ::read(fd, &buffer[0], 4096);
		if (count <= 0) {
			break;
		}
		data.insert(data.end(), &buffer[0], &buffer[count]);
	}

	::close(fd);

	fields_ = deserialize_fields(data);
}

std::size_t
config_file::unlocked_levels() const
{
	return get_field<uint64_t>(fields_, "unlocked_levels", 0);
}

void
config_file::set_unlocked_levels(std::size_t value)
{
	put_field<uint64_t>(fields_, "unlocked_levels", value);
}
