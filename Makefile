# Welcome to the jupiter source!
#
# to make the runtime library (located in runtime/) use
#    make runtime
#
# to make the compiler (located in src/) use
#    make jup
#
# to make the toolchain (located in src/jupc/) use
#    make jupc
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
CXXFLAGS   = $(OPTFLAGS) -Ilib $(EXTFLAGS) -Wall -std=c++11
LINKFLAGS  = -O2 -g 
LINK       = $(OPTFLAGS)

RUNTIME    = runtime/runtime.a
SRCS       = $(wildcard src/*.cpp) $(wildcard src/*.c)
OBJS       = $(SRCS:src/%=obj/%.o)

TEST_SRCS  = $(wildcard bin/*.j)
JUPC_SRCS  = $(wildcard src/jupc/jupc.cpp)
JUPC_OBJS  = $(JUPC_SRCS:src/jupc/%=obj/jupc/%.o)

ifeq ($(OS), Windows_NT)
TESTS      = $(TEST_SRCS:bin/%.j=./jup-%.exe)
OUT        = ./jup.exe
JUPC       = ./jupc.exe
else
TESTS      = $(TEST_SRCS:bin/%.j=./jup-%)
OUT        = ./jup
JUPC       = ./jupc
endif


all: runtimelib jup jupc tests

jup: $(OUT)
jupc: $(JUPC)
tests: $(TESTS)
runtimelib:	
	make -C runtime/ $(RUNTIME:runtime/%=%)

obj:
ifdef VERBOSE
	mkdir -p ./obj/jupc
else
	@printf " MKDIR\n"
	@mkdir -p ./obj/jupc
endif


# --- compiler ---

$(OUT): $(OBJS)
ifdef VERBOSE
	$(CXX) $(LINKFLAGS) -o $(OUT) $(OBJS) $(LINK) 
else
	@printf " LINK   $@\n"
	@$(CXX) $(LINKFLAGS) -o $(OUT) $(OBJS) $(LINK) 
endif

obj/%.cpp.o: src/%.cpp obj
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
	rm -f $(OUT) $(JUPC) $(SRCOBJS)
else
	@printf " CLEAN COMPILER\n"
	@rm -f $(OUT) $(SRCOBJS) $(JUPC) $(JUPC_OBJS)
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




