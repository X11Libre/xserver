#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_image.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <ctype.h>
#include <unistd.h>

/*
 * This is a test which try to test correct glamor colors when rendered.
 * It should be run with fullscreen Xephyr (with glamor) with present and with 
 * etalon high-level Xserver (can be any, on CI - Xvfb). For testing this test
 * creates an image in Xephyr X server, which filled by one of colors defined in
 * test_pixels. Then it captures central pixel from both Xephyr and Xserver above.
 * If pixels differ - test failed. Sleep is used to ensure than presentation on both 
 * Xephyr and Xvfb kicks (xcb_aux_sync was not enough) and test results will be actual
 */

#define WIDTH 300
#define HEIGHT 300

int get_display_pixel(xcb_connection_t* c, xcb_drawable_t win);
void draw_display_pixel(xcb_connection_t* c, xcb_drawable_t win, uint32_t pixel_color);

int get_display_pixel(xcb_connection_t* c, xcb_drawable_t win)
{
	xcb_image_t *image;
	uint32_t    pixel;
	int format = XCB_IMAGE_FORMAT_XY_PIXMAP;

	image = xcb_image_get (c, win,
		 0, 0, WIDTH, HEIGHT,
		 UINT32_MAX,
		 format);
	if (!image) {
	  printf("xcb_image_get failed: exiting\n");
	  exit(1);
	}

	pixel = xcb_image_get_pixel(image, WIDTH/2, HEIGHT/2);

	return pixel;
}

void draw_display_pixel(xcb_connection_t* c, xcb_drawable_t win, uint32_t pixel_color)
{
	xcb_gcontext_t       foreground;
	uint32_t             mask = 0;

	xcb_rectangle_t rectangles[] = {
	  {0, 0, WIDTH, HEIGHT},
	};

	foreground = xcb_generate_id (c);
	mask = XCB_GC_FOREGROUND | XCB_GC_LINE_WIDTH | XCB_GC_SUBWINDOW_MODE;

	uint32_t values[] = {
		pixel_color,
		20,
		XCB_SUBWINDOW_MODE_INCLUDE_INFERIORS
	};

	xcb_create_gc (c, foreground, win, mask, values);

	xcb_poly_fill_rectangle (c, win, foreground, 1, rectangles);
	xcb_aux_sync ( c );
}


int main(int argc, char* argv[])
{
	xcb_connection_t    *c, *r;
	xcb_screen_t        *screen1, *screen2;
	xcb_drawable_t       win1, win2;
    char *name_test = NULL, *name_relevant = NULL;
	uint32_t pixel_server1, pixel_server2;
	int result = 0;
	uint32_t test_pixels[3] = {0xff0000, 0x00ff00, 0x0000ff};
	int gv;

        while ((gv = getopt(argc, argv, "t:r:")) != -1) {
          switch (gv) {
	  case 't':
		name_test = optarg;
		break;
	  case 'r':
		name_relevant = optarg;
		break;
	  case '?':
            if (optopt == 't' || optopt == 'r') {
              fprintf(stderr,
                      "Option -%c requires an argument - test screen name.\n",
                      optopt);
            } else if (isprint(optopt)) {
              fprintf(stderr, "Unknown option `-%c'.\n", optopt);
            } else {
              fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
            }
                return 1;
	  default:
		abort ();
          }
        }

        printf("test=%s, rel=%s\n", name_test, name_relevant);

	c = xcb_connect (name_test, NULL);
	r = xcb_connect (name_relevant, NULL);

	/* get the first screen */
	screen1 = xcb_setup_roots_iterator (xcb_get_setup (c)).data;

    win1 = xcb_generate_id (c);
    xcb_create_window (c,                    /* Connection          */
                       XCB_COPY_FROM_PARENT,          /* depth (same as root)*/
                       win1,                        /* window Id           */
                       screen1->root,                  /* parent window       */
                       0, 0,                          /* x, y                */
                       WIDTH, HEIGHT,                /* width, height       */
                       20,                            /* border_width        */
                       XCB_WINDOW_CLASS_INPUT_OUTPUT, /* class               */
                       screen1->root_visual,           /* visual              */
                       0, NULL );                     /* masks, not used yet */


    /* Map the window on the screen */
    xcb_map_window (c, win1);
    xcb_aux_sync(c);

	/* get the first screen */
	screen2 = xcb_setup_roots_iterator (xcb_get_setup (r)).data;

	/* root window */
	win2 = screen2->root;

	for(int i = 0; i < 3; i++)
	{
		draw_display_pixel(c, win1, test_pixels[i]);
		xcb_aux_sync(r);
		pixel_server1 = get_display_pixel(c, win1);
		sleep(1);
		pixel_server2 = get_display_pixel(r, win2);
		xcb_aux_sync(r);
		printf("p=0x%x, p2=0x%x\n", pixel_server1, pixel_server2);
		result+= pixel_server1 == pixel_server2;
	}
	return result == 3 ? 0 : 1;
}
