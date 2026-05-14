CC 		= gcc
CFLAGS 	= $(shell pkg-config --cflags sdl3 vulkan) -Iinclude -Wall -Werror -Wextra -std=c11
LIBS 	= $(shell pkg-config --libs sdl3 vulkan)

SRCS = $(wildcard src/*.c) $(wildcard src/Vulkan/*.c)

SHADER_DIR = assets/shaders
SHADER_OUT = build/shaders
SHADERS = $(SHADER_OUT)/simple.vert.spv $(SHADER_OUT)/simple.frag.spv

$(SHADER_OUT)/%.vert.spv: $(SHADER_DIR)/%.vert | $(SHADER_OUT)
	glslc $< -o $@
$(SHADER_OUT)/%.frag.spv: $(SHADER_DIR)/%.frag | $(SHADER_OUT)
	glslc $< -o $@

build/ZorpEngine: $(SRCS) $(SHADERS) | build
	$(CC) $(CFLAGS) $(SRCS) -o build/ZorpEngine $(LIBS)

release: | build
	$(CC) $(CFLAGS) -DNDEBUG $(SRCS) -o build/ZorpEngine $(LIBS)

$(SHADER_OUT):
	mkdir -p $(SHADER_OUT)

build:
	mkdir -p build

clean:
	rm -f build/ZorpEngine build/shaders/*.spv

