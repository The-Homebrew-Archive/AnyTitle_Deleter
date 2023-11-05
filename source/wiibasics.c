/*-------------------------------------------------------------
 
wiibasics.c -- basic Wii initialization and functions
 
Copyright (C) 2008 tona
Unless other credit specified
 
This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any
damages arising from the use of this software.
 
Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:
 
1.The origin of this software must not be misrepresented; you
must not claim that you wrote the original software. If you use
this software in a product, an acknowledgment in the product
documentation would be appreciated but is not required.
 
2.Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.
 
3.This notice may not be removed or altered from any source
distribution.
 
-------------------------------------------------------------*/

#include <stdio.h>
#include <gccore.h>
#include <wiiuse/wpad.h>

#include "wiibasics.h"
#include "id.h"
#include "uninstall.h"

#define MAX_WIIMOTES 4

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

/* Basic init taken pretty directly from the libOGC examples */
void basicInit(void)
{
	// Initialise the video system
	VIDEO_Init();
	
	// This function initialises the attached controllers
	WPAD_Init();
	
	// Obtain the preferred video mode from the system
	// This will correspond to the settings in the Wii menu
	rmode = VIDEO_GetPreferredMode(NULL);

	// Allocate memory for the display in the uncached region
	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	
	// Initialise the console, required for printf
	console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);
	
	// Set up the video registers with the chosen mode
	VIDEO_Configure(rmode);
	
	// Tell the video hardware where our display memory is
	VIDEO_SetNextFramebuffer(xfb);
	
	// Make the display visible
	VIDEO_SetBlack(FALSE);

	// Flush the video register changes to the hardware
	VIDEO_Flush();

	// Wait for Video setup to complete
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();


	// The console understands VT terminal escape codes
	// This positions the cursor on row 2, column 0
	// we can use variables for this with format codes too
	// e.g. printf ("\x1b[%d;%dH", row, column );
	printf("\x1b[2;0H");
}

void miscInit(void)
{
	s32 ret;

	ret = Identify_SU();
	if (ret < 0){
		wait_anyKey();
	}
	
	printf("\n\tInitializing Filesystem driver...");
	fflush(stdout);
	
	ret = ISFS_Initialize();
	if (ret < 0) {
		printf("\n\tError! ISFS_Initialize (ret = %d)\n", ret);
		wait_anyKey();
	} else {
		printf("OK!\n");
	}
	
	printf("\tWiping off fingerprints...\n");
	fflush(stdout);
	Uninstall_FromTitle(TITLE_ID(1, 0));
}

void miscDeInit(void)
{
	fflush(stdout);
	ISFS_Deinitialize();
}

u32 getButtons(void)
{
	WPAD_ScanPads();
	return WPAD_ButtonsDown(0);
}

u32 wait_anyKey(void) {
	u32 pressed;
	while(!(pressed = getButtons())) {
		VIDEO_WaitVSync();
	}
	return pressed;
}

u32 wait_key(u32 button) {
	u32 pressed;
	do {
		VIDEO_WaitVSync();
		pressed = getButtons();
	} while(!(pressed & button));
	return pressed;
}
