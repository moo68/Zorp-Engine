CC 		= gcc
CFLAGS 	= $(shell pkg-config --cflags sdl3 vulkan) -Wall -Wextra -Werror -std=c11
LIBS 	= $(shell pkg-config --libs sdl3 vulkan)

build/ZorpEngine: src/main.c
	$(CC) $(CFLAGS) src/main.c -o build/ZorpEngine $(LIBS)

release:
	$(CC) $(CFLAGS) -DNDEBUG src/main.c -o build/ZorpEngine $(LIBS)

clean:
	rm -f build/ZorpEngine

