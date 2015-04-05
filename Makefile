CXX        = mingw32-g++
OPTFLAGS   = -O0 -g
CXXFLAGS   = $(OPTFLAGS) $(EXTFLAGS) -Wall -std=c++11

LINK       = $(OPTFLAGS)

OUT        = bin/jup.exe
SRCS       = $(wildcard src/*.cpp) $(wildcard src/*.c)
SRCOBJS    = $(SRCS:src/%=obj/%.o)
OBJS       = $(SRCOBJS) $(wildcard obj/*.a)

ARGV       =


all: $(OUT)

$(OUT): $(OBJS)
ifdef VERBOSE
	$(CXX) $(CFLAGS) -o $(OUT) $(OBJS) $(LINK) 
else
	@printf " LINK   $@\n"
	@$(CXX) $(CFLAGS) -o $(OUT) $(OBJS) $(LINK) 
endif

obj/%.cpp.o: src/%.cpp
ifdef VERBOSE
	$(CXX) $(CXXFLAGS) -c -o $@ $<
else
	@printf " CXX    $@\n"
	@$(CXX) $(CXXFLAGS) -c -o $@ $<
endif

clean:
ifdef VERBOSE
	rm -f $(OUT) $(SRCOBJS)
else
	@printf " CLEAN\n"
	@rm -f $(OUT) $(SRCOBJS)
endif

strip: $(OUT)
ifdef VERBOSE
	strip $(OUT)
else
	@printf " STRIP  $(OUT)\n"
	@strip $(OUT)
endif

rebuild: clean all

publish: OPTFLAGS := -O3
publish:     LINK += -mwindows
publish: rebuild $(OUT) 


keymaps: EXTFLAGS += '-DNO_KEY_MAPS'
keymaps:     ARGV := '-keymaps'
keymaps: rebuild go


go: $(OUT)
	cd bin && $(OUT:bin/%=./%) $(ARGV)
