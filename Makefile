all: fbdisp.c
	gcc --std=gnu99 -o fbdisp fbdisp.c `sdl-config --cflags` -lSDL -lm

clean:
	rm -rf fbdisp
