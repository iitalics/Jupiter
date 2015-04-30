# Welcome to the jupiter source!
#
# to make the runtime library (located in runtime/) use
#    make runtime
#
# to make the compiler (located in src/) use
#    make jup
#
# to make the example programs (located in bin/) use
#    make tests


ifdef MINGW
CXX        = mingw32-g++
else
CXX        = clang++
endif
CC         = clang
OPTFLAGS   = -O0 -g
CXXFLAGS   = $(OPTFLAGS) $(EXTFLAGS) -Wall -std=c++11
LINK       = $(OPTFLAGS)

RUNTIME    = runtime/runtime.a
SRCS       = $(wildcard src/*.cpp) $(wildcard src/*.c)
SRCOBJS    = $(SRCS:src/%=obj/%.o)
OBJS       = $(SRCOBJS) $(wildcard obj/*.a)

TEST_SRCS  = $(wildcard bin/*.j)

ifeq ($(OS), Windows_NT)
TESTS      = $(TEST_SRCS:bin/%.j=./jup-%.exe)
OUT        = ./jup.exe
else
TESTS      = $(TEST_SRCS:bin/%.j=./jup-%)
OUT        = ./jup
endif


all: runtimelib jup tests

jup: $(OUT)
tests: $(TESTS)
runtimelib:	
	make -C runtime/ $(RUNTIME:runtime/%=%)


# --- compiler ---

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

clean: clean-tests
ifdef VERBOSE
	rm -f $(OUT) $(SRCOBJS)
else
	@printf " CLEAN COMPILER\n"
	@rm -f $(OUT) $(SRCOBJS)
endif

rebuild: clean all


# --- tests ---

bin/%.ll: $(OUT) bin/%.j
ifdef VERBOSE
	$(OUT) $(@:bin/%.ll=bin/%.j) > $@
else
	@printf " JUP    $@\n"
	@$(OUT) $(@:bin/%.ll=bin/%.j) > $@
endif

bin/%.s: bin/%.ll
	@llc -o $@ $<

./jup-%: bin/%.s
ifdef VERBOSE
	$(CC) -o $@ $< $(RUNTIME)
else
	@printf " CC     $@\n"
	@$(CC) -o $@ $< $(RUNTIME)
endif
./jup-%.exe: bin/%.s
ifdef VERBOSE
	$(CC) -o $@ $< $(RUNTIME)
else
	@printf " CC     $@\n"
	@$(CC) -o $@ $< $(RUNTIME)
endif

clean-tests:
ifdef VERBOSE
	rm -f $(TESTS)
else
	@printf " CLEAN TESTS\n"
	@rm -f $(TESTS)
endif





keymaps: EXTFLAGS += '-DNO_KEY_MAPS'
keymaps:     ARGV := '-keymaps'
keymaps: rebuild go




