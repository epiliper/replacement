BIN := REPLACEMENT
PKG_CONF := $(shell pkg-config --libs --cflags glfw3 cglm freetype2) -lm
INCLUDES := -I includes
S := src
MEMDBG := -fsanitize=address

debug: $S/main.c $S/glad.c
	gcc $(PKG_CONF) $(INCLUDES) $S/main.c $S/glad.c -o $(BIN);
	./$(BIN)
