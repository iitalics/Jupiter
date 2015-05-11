CC     = clang
CFLAGS = -Wall -O2 -g


OUT    = runtime.a
SRCS   = $(wildcard *.c)
OBJS   = $(SRCS:%=obj/%.o)



all: $(OUT)

obj:
	mkdir obj

$(OUT): obj $(OBJS)
	ar rcs $(OUT) $(OBJS)

obj/%.o: ./%
	$(CC) $(CFLAGS) -c -o $@ $<


clean:
	rm -f $(OBJS) $(OUT)

rebuild: clean all
