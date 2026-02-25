BIN := REPLACEMENT
PKG_CONF := $(shell pkg-config --libs --cflags glfw3 cglm freetype2 assimp) -lm
INCLUDES := -I includes
S := src
MEMDBG := -fsanitize=address -g
CFLAGS := -g -o2

debug: $S/main.c $S/glad.c $S/utils.c $S/log.c $S/mesh.c $S/physics.c
	gcc $(PKG_CONF) $(INCLUDES) $(CFLAGS) $S/main.c $S/glad.c $S/log.c $S/utils.c $S/mesh.c $S/thing.c $S/physics.c -o $(BIN) -O2;
	./$(BIN)

release: $S/main.c $S/glad.c $S/utils.c $S/log.c $S/mesh.c $S/physics.c
	gcc $(PKG_CONF) $(INCLUDES) $S/main.c $S/glad.c $S/log.c $S/utils.c $S/mesh.c $S/thing.c $S/physics.c -o $(BIN) -o2;
	./$(BIN)
