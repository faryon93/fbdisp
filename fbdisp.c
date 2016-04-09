#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/mman.h>
#include <SDL.h>

// ----------------------------------------------------------------------------------
//  constants
// ----------------------------------------------------------------------------------

#define DISPLAY_DEPTH		32
#define DISPLAY_BPP 		DISPLAY_DEPTH / 8
#define DISPLAY_REFRESH		30


// ----------------------------------------------------------------------------------
//  global variables
// ----------------------------------------------------------------------------------

/** the pixels contained in the framebuffer. */
static uint8_t *framebuffer = NULL;


// ----------------------------------------------------------------------------------
//  local function
// ----------------------------------------------------------------------------------

static void setpixel(SDL_Surface *screen, int x, int y, Uint8 r, Uint8 g, Uint8 b)
{
    Uint32 *pixmem32;
    Uint32 colour;  
 
    colour = SDL_MapRGB(screen->format, r, g, b);
  
    pixmem32 = (Uint32*)screen->pixels + (y * screen->w) + x;
    *pixmem32 = colour;
}

static int pollevent()
{
	SDL_Event event;
	while(SDL_PollEvent(&event)) 
    {      
		switch (event.type) 
		{
			case SDL_QUIT:
				return 1;
				break;
		}
    }

    return 0;
}

static void drawfb(SDL_Surface* screen, int fbw, int fbh, int fbbpp, int scale)
{ 
    if(SDL_MUSTLOCK(screen)) 
    {
        if(SDL_LockSurface(screen) < 0)
        	return;
    }

    for(int y = 0; y < fbh; y++)
    {
        for(int x = 0; x < fbw; x++) 
        {
        	// the intermediate pixels are left black to 
        	// create an led matrix style look
        	setpixel(screen, scale * x, scale * y,
        			 framebuffer[(y * fbw + x) * fbbpp + 2],
        			 framebuffer[(y * fbw + x) * fbbpp + 1],
        			 framebuffer[(y * fbw + x) * fbbpp]);
        }
    }

    if(SDL_MUSTLOCK(screen))
    	SDL_UnlockSurface(screen);
  
    SDL_Flip(screen); 
}


// ----------------------------------------------------------------------------------
//  entry point
// ----------------------------------------------------------------------------------

int main(int argc, char *argv[])
{
	int fd, memsize, keypress = 0, scale = 1;
	struct fb_var_screeninfo vinfo;
	SDL_Surface *screen;
    
	// make sure all cmd args are present
	if (argc < 2)
	{
		printf("usage: ./fbdisp devfile <scale>\n");
		return -1;
	}

	// has the user supllied a scale?
	if (argc >= 3)
		scale = atoi(argv[2]);

	// open the framebuffer device file
	fd = open(argv[1], O_RDONLY);
	if (-1 == fd)
	{
		perror("open");
		return -1;
	}

	// inquire screen infos
    if (ioctl(fd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
        perror("FBIOGET_VSCREENINFO");
        close(fd);
        return -1;
    }

    // map the framebuffer pixels to userspace
    memsize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;
    framebuffer = (unsigned char*)mmap(0, memsize, PROT_READ, MAP_SHARED, fd, 0);
	if (framebuffer == MAP_FAILED)
	{
		perror("mmap");
		close(fd);
		return -1;
	}

	// initialize sdl
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		munmap(framebuffer, memsize);
		close(fd);
		return -1;
	}

	if (!(screen = SDL_SetVideoMode(scale * vinfo.xres, scale* vinfo.yres, DISPLAY_DEPTH, 0)))
    {
    	munmap(framebuffer, memsize);
		close(fd);
        SDL_Quit();
        return -1;
    }
    SDL_WM_SetCaption("fbdisp", "fbdisp");

    // main render looop
	while (!keypress)
	{
		// draw the content of the framebuffer
		drawfb(screen, vinfo.xres, vinfo.yres, vinfo.bits_per_pixel / 8, scale);

		// wait for next cycle
		keypress = pollevent();
		usleep(DISPLAY_REFRESH * 1000);
	}


	// free all ressources
	SDL_Quit();
	munmap(framebuffer, memsize);
	close(fd);
}