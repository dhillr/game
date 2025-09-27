all:
	gcc -g -I./include -L./lib src/main.c src/glad.c src/stb.c -lglfw3dll -lopengl32 -lm -o bin/main.exe