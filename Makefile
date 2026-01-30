BIN := REPLACEMENT
PKG_CONF := $(shell pkg-config --libs --cflags glfw3 cglm)
INCLUDES := -I includes
S := src
MEMDBG := fsanitize=address

debug: $S/main.c $S/glad.c $S/darr.c
	gcc $(PKG_CONF) $(INCLUDES) $S/main.c $S/glad.c $S/darr.c -o $(BIN);
	./$(BIN)
