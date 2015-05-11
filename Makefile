# Welcome to the jupiter source!
#
# to make the runtime library (located in lib/) use
#    make runtime
#
# to make the compiler (located in src/) use
#    make jup
#
# to make the toolchain (located in src/jupc/) use
#    make jupc
#
# to make the example programs (located in examples/) use
#    make tests


ifdef MINGW
CXX        = mingw32-g++
else
CXX        = clang++
endif
CC         = clang
OPTFLAGS   = -O0 -g
CXXFLAGS   = $(OPTFLAGS) $(EXTFLAGS) -Wall -std=c++11
LINKFLAGS  = -O2 -g 
LINK       = $(OPTFLAGS)

RUNTIME    = lib/runtime.a
SRCS       = $(wildcard src/*.cpp) $(wildcard src/*.c)
OBJS       = $(SRCS:src/%=obj/%.o)

TEST_SRCS  = $(wildcard examples/*.j)
JUPC_SRCS  = $(wildcard src/jupc/jupc.cpp)
JUPC_OBJS  = $(JUPC_SRCS:src/jupc/%=obj/jupc/%.o)

ifeq ($(OS), Windows_NT)
TESTS      = $(TEST_SRCS:examples/%.j=./jup-%.exe)
JUP        = ./jup.exe
JUPC       = ./jupc.exe
else
TESTS      = $(TEST_SRCS:examples/%.j=./jup-%)
JUP        = ./jup
JUPC       = ./jupc
endif


all: runtime jup jupc tests

jup: $(JUP)
jupc: $(JUPC)
tests: $(JUPC) $(TESTS)
runtime:	
	@make -C lib/ $(RUNTIME:lib/%=%)

obj:
ifdef VERBOSE
	mkdir -p ./obj/jupc
else
	@printf " MKDIR\n"
	@mkdir -p ./obj/jupc
endif


# --- compiler ---

$(JUP): obj $(OBJS)
ifdef VERBOSE
	$(CXX) $(LINKFLAGS) -o $(JUP) $(OBJS) $(LINK) 
else
	@printf " LINK   $@\n"
	@$(CXX) $(LINKFLAGS) -o $(JUP) $(OBJS) $(LINK) 
endif

obj/%.cpp.o: src/%.cpp
ifdef VERBOSE
	$(CXX) $(CXXFLAGS) -c -o $@ $<
else
	@printf " CXX    $@\n"
	@$(CXX) $(CXXFLAGS) -c -o $@ $<
endif

$(JUPC): $(JUPC_OBJS)
ifdef VERBOSE
	$(CXX) $(LINKFLAGS) -o $@ $(JUPC_OBJS) $(LINK) 
else
	@printf " LINK   $@\n"
	@$(CXX) $(LINKFLAGS) -o $@ $(JUPC_OBJS) $(LINK) 
endif


clean: clean-tests
ifdef VERBOSE
	rm -f $(JUP) $(OBJS) $(JUPC) $(JUPC_OBJS)
else
	@printf " CLEAN COMPILER\n"
	@rm -f $(JUP) $(OBJS) $(JUPC) $(JUPC_OBJS)
endif

rebuild: clean all


# --- tests ---

./jup-%.exe: examples/%.j
	$(JUPC) $< -o $@

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




