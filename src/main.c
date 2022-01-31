#include <eris/v810.h>
#include <eris/king.h>
#include <eris/tetsu.h>
#include <eris/romfont.h>
#include <eris/cd.h>
#include <eris/low/pad.h>
#include <eris/low/scsi.h>

#include "main.h"
#include "tinyfont.h"


#ifdef _8BPP
	unsigned char framebuffer[SCREEN_SIZE_IN_PIXELS];
#else
	unsigned short framebuffer[SCREEN_SIZE_IN_PIXELS];
#endif

unsigned short myPal[256];

static void initDisplay()
{
	int i;
	u16 microprog[16];

	eris_king_init();
	eris_tetsu_init();

	eris_tetsu_set_priorities(0, 0, 1, 0, 0, 0, 0);
	eris_tetsu_set_king_palette(0, 0, 0, 0);
	eris_tetsu_set_rainbow_palette(0);

	eris_king_set_bg_prio(KING_BGPRIO_0, KING_BGPRIO_HIDE, KING_BGPRIO_HIDE, KING_BGPRIO_HIDE, 0);
	#ifdef _8BPP
		eris_king_set_bg_mode(KING_BGMODE_256_PAL, KING_BGPRIO_HIDE, KING_BGPRIO_HIDE, KING_BGPRIO_HIDE);
	#else
		eris_king_set_bg_mode(KING_BGMODE_64K, KING_BGPRIO_HIDE, KING_BGPRIO_HIDE, KING_BGPRIO_HIDE);
	#endif
	eris_king_set_kram_pages(0, 0, 0, 0);

	for(i = 0; i < 16; i++) {
		microprog[i] = KING_CODE_NOP;
	}

	#ifdef _8BPP
		microprog[0] = KING_CODE_BG0_CG_0;
		microprog[1] = KING_CODE_BG0_CG_1;
		microprog[2] = KING_CODE_BG0_CG_2;
		microprog[3] = KING_CODE_BG0_CG_3;
	#else
		microprog[0] = KING_CODE_BG0_CG_0;
		microprog[1] = KING_CODE_BG0_CG_1;
		microprog[2] = KING_CODE_BG0_CG_2;
		microprog[3] = KING_CODE_BG0_CG_3;
		microprog[4] = KING_CODE_BG0_CG_4;
		microprog[5] = KING_CODE_BG0_CG_5;
		microprog[6] = KING_CODE_BG0_CG_6;
		microprog[7] = KING_CODE_BG0_CG_7;	
	#endif

	eris_king_disable_microprogram();
	eris_king_write_microprogram(microprog, 0, 16);
	eris_king_enable_microprogram();

	#ifdef _8BPP
	eris_tetsu_set_rainbow_palette(0);
	for(i = 0; i < 256; i++) {
		eris_tetsu_set_palette(i, myPal[i]);
	}
	#endif

	eris_tetsu_set_video_mode(TETSU_LINES_262, 0, TETSU_DOTCLOCK_5MHz, TETSU_COLORS_16, TETSU_COLORS_16, 0, 0, 1, 0, 0, 0, 0);

	eris_king_set_bat_cg_addr(KING_BG0, 0, 0);
	eris_king_set_bat_cg_addr(KING_BG0SUB, 0, 0);
	eris_king_set_scroll(KING_BG0, 0, 0);
	eris_king_set_bg_size(KING_BG0, KING_BGSIZE_256, KING_BGSIZE_256, KING_BGSIZE_256, KING_BGSIZE_256);

	eris_king_set_kram_read(0, 1);
	eris_king_set_kram_write(0, 1);

	for(i = 0; i < SCREEN_SIZE_IN_PIXELS; ++i) {
		eris_king_kram_write(0);
	}
	eris_king_set_kram_write(0, 1);
}

void bufDisplay()
{
	int i;
	//unsigned int *dst = (unsigned int*)framebuffer;

	eris_king_set_kram_read(0, 1);
	eris_king_set_kram_write(0, 1);

	#ifdef _8BPP
	for(i = 0; i < SCREEN_SIZE_IN_PIXELS; i+=2) {
		eris_king_kram_write(framebuffer[i] << 8 | framebuffer[i+1]);
	}
	#else
	for(i = 0; i < SCREEN_SIZE_IN_PIXELS; ++i) {
		eris_king_kram_write(framebuffer[i]);
	}
	#endif

	/*#ifdef _8BPP
	for(i = 0; i < SCREEN_SIZE_IN_PIXELS; i+=4) {
		const unsigned int c = *dst++;
		const unsigned short c0 = (c << 8) | ((c>>8) & 0x00FF);
		const unsigned short c1 = ((c >> 8) & 0xFF00) | (c>>24);
		eris_king_kram_write(c0);
		eris_king_kram_write(c1);
	}
	#else
	for(i = 0; i < SCREEN_SIZE_IN_PIXELS; i+=2) {
		const unsigned int c = *dst++;
		const unsigned short c0 = (c << 8) | ((c>>8) & 0x00FF);
		const unsigned short c1 = ((c >> 8) & 0xFF00) | (c>>24);
		eris_king_kram_write(c0);
		eris_king_kram_write(c1);
	}
	#endif*/

	eris_king_set_kram_write(0, 1);
}

static void testDisplay(int t)
{
	int i;
	//int x,y;
	//unsigned int *dst = (unsigned int*)framebuffer;

	/*eris_king_set_kram_write(0, 1);
	for(i = 0; i < SCREEN_SIZE_IN_PIXELS; ++i) {
		eris_king_kram_write((unsigned short)(i+t));
	};*/

	for(i = 0; i < SCREEN_SIZE_IN_PIXELS; ++i) {
		framebuffer[i] = i+t;
	}

	/*i = 0;
	for (y=0; y<SCREEN_HEIGHT; ++y) {
		for (x=0; x<SCREEN_WIDTH; ++x) {
			framebuffer[i++] = x^y;
		}
	}*/
	/*for (y=0; y<SCREEN_HEIGHT; ++y) {
		framebuffer[y*SCREEN_WIDTH + y] = 0xFFFF;
	}*/

	bufDisplay();
}

static void initPal()
{
	int i;
	for (i=0; i<256; ++i) {
		myPal[i] = (i<<8) | 255;
	}
}

/*static void vsync()
{
	while(eris_tetsu_get_raster() !=200) {};
}*/

int main()
{
	static int t = 0;

	initPal();
	initDisplay();

	initTinyFonts();

	for(;;) {
		testDisplay(t++);
		drawNumber(4,231-4, t);
	}

	return 0;
}
