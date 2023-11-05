/*-------------------------------------------------------------
 
main.c -- main and menu code.
 
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
#include <stdlib.h>
#include <string.h>
#include <gccore.h>
#include <wiiuse/wpad.h>

#include "detect_settings.h"
#include "titles.h"
#include "uninstall.h"
#include "wiibasics.h"

/* Constants */
// ITEMS_PER_PAGE must be evenly divisible by COLUMNS
#define ITEMS_PER_PAGE 	30
#define COLUMNS 			3
#define ROWS					(ITEMS_PER_PAGE/COLUMNS)
#define MAX_TITLES 			256
#define NUM_TYPES 			7

u8 system_region;
u32 page = 0, selected = 0, menu_index = 0, num_titles, max_selected;
static u32 titles[MAX_TITLES] ATTRIBUTE_ALIGN(32);
static char page_header[2][80];
static char page_contents[ITEMS_PER_PAGE][64];
u32 types[] = {1, 0x10000, 0x10001, 0x10002, 0x10004, 0x10005, 0x10008};

char *titleText(u32 title){
	static char text[10];
	
	if (types[menu_index-1] == 1){
		// If we're dealing with System Titles, use custom names
		switch (title){
			case 1:
				strcpy(text, "BOOT2");
			break;
			case 2:
				strcpy(text, "SYSMENU");
			break;
			case 0x100:
				strcpy(text, "BC");
			break;
			case 0x101:
				strcpy(text, "MIOS");
			break;
			default:
				sprintf(text, "IOS%u", title);
			break;
		}
	} else {
		// Otherwise, just convert the title to ASCII
		int i =32, j = 0;
		do {
			u8 temp;
			i -= 8;
			temp = (title >> i) & 0x000000FF;
			if (temp < 0x20 || temp > 0x7E)
				text[j] = '.';
			else
				text[j] = temp;
			j++;
		} while (i > 0);
		text[4] = 0;
	}
	return text;
}

void uninstallChecked(u32 kind, u32 title) {
	u64 tid = TITLE_ID(kind, title);
	u64 sysmenu_ios = get_title_ios(TITLE_ID(1, 2));
	
	printf("\tTitle id: %08x-%08x (%s)\n", kind, title, titleText(title));
	
	// Don't uninstall System titles if we can't find sysmenu IOS
	if (kind == 1){
		if (sysmenu_ios == 0) {
			printf("\tSafety Check! Can't detect Sysmenu IOS, system title deletes disabled\n");
			printf("\tPlease report this to the author\n");
			printf("\tPress any key...\n\n");
			wait_anyKey();
			return;
		} 
	}
	
	// Don't uninstall hidden channel titles if we can't find sysmenu region
	if (kind == 0x10008){	
		if (system_region == 0) {
			printf("\tSafety check! Can't detect Sysmenu Region, hidden channel deletes disabled\n");
			printf("\tPlease report this to the author\n");
			printf("\tPress any key...\n\n");
			wait_anyKey();
			return;
		}
	}
	
	// Fail for uninstalls of various titles.
	if (tid ==TITLE_ID(1, 1))
		printf("\tBrick protection! Can't delete boot2!\n");
	else if (tid == TITLE_ID(1, 2))
		printf("\tBrick protection! Can't delete System Menu!\n");
	else if(tid == sysmenu_ios) 
		printf("\tBrick protection! Can't delete Sysmenu IOS!\n");
	else if(tid  == TITLE_ID(0x10008, 0x48414B00 | system_region))
		printf("\tBrick protection! Can't delete your region's EULA!\n");
	else if(tid  == TITLE_ID(0x10008, 0x48414C00 | system_region))
		printf("\tBrick protection! Can't delete your region's rgnsel!\n");
	else {
		
		
		// Display a warning if you're deleting the current IOS, although it won't break operation to delete it
		if (title == IOS_GetVersion())
			printf("\tWARNING: This is the currently loaded IOS!\n\t- However, deletion should not affect current usage.\n");
		
		// Display a warning if you're deleting the Homebrew Channe's IOS
		if (tid == get_title_ios(TITLE_ID(0x10001, 0x48415858)))
			printf("\tWARNING: This is the IOS used by the Homebrew Channel!\n\t- Deleting it will cause the Homebrew Channel to stop working!\n");
		
		printf("\tAre you sure you want to uninstall this title? (A = Yes, B = No)\n");
		if (wait_key(WPAD_BUTTON_A | WPAD_BUTTON_B) & WPAD_BUTTON_B){
			return;
		} else {
		//printf("\tUninstall!!!!\n");
		Uninstall_FromTitle(tid);	
		}
	}
	printf("\tPress any key...\n\n");
		wait_anyKey();
}


/* Double-caching menu code was made in an attempt to improve performance 
  In the end, my input lag was caused by something entirely different, 
  but I didn't really want to undo the whole menu */


void printMenuList(void){
	//Shove the headers out
	printf(page_header[0]);
	printf(page_header[1]);
	
	if (menu_index == 0){
		// If we're on the main index, just shove out each row
		int i;
		for (i = 0; i < NUM_TYPES; i++)
			printf(page_contents[i]);
		
	} else if (num_titles) {
		// If we're on a page with titles, print all columns and rows
		int i;		
		for (i = 0; i < ROWS; i++){
			int j;
			for (j = 0; j < COLUMNS; j++)
				printf(page_contents[i+(j*ROWS)]);
			putchar('\n');
		}
		
	} else{
		// Or a blank page
		printf("\n\tNo titles to display\n");
	}
	
}

void updateSelected(int delta){
	if (delta == 0){
		// Set cursor location to 0
		selected = 0;
	} else {
		// Remove the cursor from the last selected item
		page_contents[selected][1] = ' ';
		page_contents[selected][2] = ' ';
		// Set new cursor location
		selected += delta;
	}
	
	// Add the cursor to the now-selected item
	page_contents[selected][1] = '-';
	page_contents[selected][2] = '>';
	
}

void updatePage(void){
	if (menu_index == 0){
		// On the main index...
		// Set our max-tracking variables to the number of items on this menu
		max_selected = num_titles = NUM_TYPES;
		
		// Fill headers and page contents
		strcpy(page_header[0], "\tTitles:\n");
		page_header[1][0] = '\0';
		strcpy(page_contents[0], "    00000001 - System Titles\n");
		strcpy(page_contents[1], "    00010000 - Disc Game Titles (and saves)\n");
		strcpy(page_contents[2], "    00010001 - Installed Channel Titles\n");
		strcpy(page_contents[3], "    00010002 - System Channel Titles\n");
		strcpy(page_contents[4], "    00010004 - Games that use Channels (Channel+Save)\n");
		strcpy(page_contents[5], "    00010005 - Downloadable Game Content\n");
		strcpy(page_contents[6], "    00010008 - Hidden Channels\n");
	
	} else {
		// For every other page....
		
		// Fill the headers
		sprintf(page_header[0], "\tTitles:%08x:\n", types[menu_index-1]);
		strcpy(page_header[1], "\tPage: ");
		if (num_titles) {
			// If there's any contents...
			int i;
			
			// Figure out where our page is starting
			int page_offset = page * ITEMS_PER_PAGE;
			
			// And the highest we can select
			if ((page+1) * ITEMS_PER_PAGE <= num_titles)
				max_selected = ITEMS_PER_PAGE;
			else
				max_selected = num_titles % ITEMS_PER_PAGE;
			
			// Fill the "Page" header with our number of pages
			for (i = 0; i <= (num_titles / ITEMS_PER_PAGE); i++)
				if(i == page)
					sprintf(page_header[1], "%s(%d) ", page_header[1], i+1);
				else
					sprintf(page_header[1], "%s %d   ", page_header[1], i+1);
			strcat(page_header[1], "\n");
				
			i = 0;
			// Fill the cached page contents with each title entry
			while (i < max_selected){
				char temp[15];
				sprintf(temp,"(%s)", titleText(titles[page_offset+i]));
				sprintf(page_contents[i], "    %08x %-10s", titles[page_offset+i], temp);
				i++;
			}
			// And fill the rest of the slots with whitespace
			while (i < ITEMS_PER_PAGE){
				strcpy(page_contents[i], "                       ");
				i++;
			}
			
			// And reset our cursor to position 0
			
		} else {
			max_selected = 0;
		}
	}
	updateSelected(0);
	
}

void updateTitleList(void){
	// Sanity check to make sure we're on a title page
	if (menu_index > 0){
		s32 ret;
		
		// Get count of titles of our requested type
		ret = getTitles_TypeCount(types[menu_index-1], &num_titles);
		if (ret < 0){
			printf("\tError! Can't get count of titles! (ret = %d)\n", ret);
			exit(1);
		}
		
		// Die if we can't handle this many
		if (num_titles > MAX_TITLES){
			printf("\tError! Too many titles! (%u)\n", num_titles);
			exit(1);
		}
		
		// Get titles of our requested type
		ret = getTitles_Type(types[menu_index-1], titles, num_titles);
		if (ret < 0){
			printf("\tError! Can't get list of titles! (ret = %d)\n", ret);
			exit(1);
		}
	}
	// Now generate the page
	updatePage();
}



void parseCommand(u32 pressed){
	
	// B: Load Index menu
	if (pressed & WPAD_BUTTON_B){
		menu_index=0;
		updatePage();
	}
	
	// Plus/Minus: Next page, last page.
	if (pressed & WPAD_BUTTON_PLUS) {
		if ((page+1) * ITEMS_PER_PAGE < num_titles) {
			page++;
			updatePage();
		}
	} else if (pressed & WPAD_BUTTON_MINUS){
		if (page > 0) {
			page--;
			updatePage();
		}
	} 
	
	// Directional controls
	if (pressed & WPAD_BUTTON_UP){
		if (selected > 0) 
			updateSelected(-1);
	} else if (pressed & WPAD_BUTTON_DOWN){
		if (selected+1 < max_selected) 
			updateSelected(1);
	}
		
	if (pressed & WPAD_BUTTON_LEFT) {
		if (selected >= ROWS)
			updateSelected(-ROWS);
	} else if (pressed & WPAD_BUTTON_RIGHT){
		if (selected+ROWS < max_selected)
			updateSelected(ROWS);
	}
		
	// Uninstall selected or enter menu
	if (pressed & WPAD_BUTTON_A){
		if (menu_index == 0) {
			menu_index = selected+1;
		}
		else if (selected < max_selected) {
			uninstallChecked(types[menu_index-1], titles[(page*ITEMS_PER_PAGE)+selected]);
		}
		updateTitleList();
	}
}

// Make sure we're getting the correct system menu region
void checkRegion(void){
	s32 sysmenu_region, conf_region;
	
	conf_region = CONF_GetRegion();
	
	printf("\n\tChecking system region...\n");
	
	system_region = get_sysmenu_region();
	switch(system_region){
		case 'E':
			sysmenu_region = CONF_REGION_US;
			break;
		case 'J':
			sysmenu_region = CONF_REGION_JP;
			break;
		case 'P':
			sysmenu_region = CONF_REGION_EU;
			break;
		default:
			printf("\tRegion detection failure! %d\n", system_region);
			printf("\tConf region: %d", conf_region);
			wait_anyKey();
			return;
			break;
	}
	printf("\tRegion properly detected as %c\n", system_region);

	if (sysmenu_region != conf_region){
		printf("\tYour System Menu and SYSCONF regions differ. (semi brick?)\n");
		printf("\tSysmenu: %d  Sysconf: %d\n", sysmenu_region, conf_region);
		printf("\tProceed with caution! (Using Sysmenu region)\n");
		wait_anyKey();
	}
}

int main(void) {
	// IOS_ReloadIOS(21);
	basicInit();	
	miscInit();
	checkRegion();
	updatePage();
	
	
	/* Main loop - Menu basics very loosely adapted from Waninkoko's WAD Manager 1.0 source */
	
	u32 pressed = 0;
	
	// Wait to see init messages
	//wait_anyKey();
	
	do{
		if (pressed)
			parseCommand(pressed);

		/* Wait for vertical sync */
		VIDEO_WaitVSync();
	
		/* Clear screen */
		printf("\x1b[2J\n\n");
	
		/* Print welcome message */
		printf("\t-------------------------------\n");
		printf("\t AnyTitle Deleter 1.0\tby tona\n");
		printf("\t-------------------------------\n");
	
		printf("\t+-- Controls -----------------------------------------------------------+\n");
		printf("\t (DPAD) Browse | (+/-) Change Page | (A) Select | (B) Back | (HOME) Exit\n");
		printf("\t+-----------------------------------------------------------------------+\n\n");
	
		/* Print item list */
		printMenuList();
	
		printf("\t+-----------------------------------------------------------------------+\n");
		
		/* Controls */
		pressed = wait_anyKey();
		
	} while(!(pressed & WPAD_BUTTON_HOME));
	
	/* Outro */
	printf("\tThanks for playing!\n");
	
	miscDeInit();
	
	return 0;
}
