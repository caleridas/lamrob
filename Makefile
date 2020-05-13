CXXFLAGS+=-g -O2 -Wall --std=c++11 -pthread `pkg-config --cflags cairo`
LDFLAGS+=-g -std=c++11 -lX11 -lGL -pthread `pkg-config --libs cairo` -lasound

OBJFILES = \
	main.o view.o tiles.o texgen.o tilegen.o board_view.o run_controller.o \
	command_tile_owner.o \
	command_queue.o command_tile_repository.o \
	grid.o puzzle.o clock.o noise2d.o robot_view.o \
	main_screen.o start_screen.o background.o icon.o \
	audioplayer.o bgmusic.o configfile.o

lambrob: $(OBJFILES)
	$(CXX) $(LDFLAGS) -o $@ $^

clean:
	rm -rf $(OBJFILES) maze .dep

DEPEND = $(patsubst %.o, .dep/%.o.d, $(OBJFILES))

.dep/%.o.d: %.cc
	@mkdir -p $(dir $@)
	@$(CXX) -MM $(CFLAGS) $(CPPFLAGS) -MT $(<:.cc=.o) -MP -MF $@ $<
	@echo MAKEDEP $<

depend: $(DEPEND)
ifeq ($(shell if [ -e .dep ] ; then echo yes ; fi),yes)
-include $(DEPEND)
endif
