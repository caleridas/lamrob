#include "texgen.h"
#include "tilegen.h"

int main()
{
	texture_generator texgen;
	std::unique_ptr<uint8_t[]> data = texgen.make_rgba(1024, 1024);
	write_rgba_to_png(1024, 1024, data.get(), 1024, "/tmp/textures.png");
}
