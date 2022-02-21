#include <eris/v810.h>
#include <eris/king.h>
#include <eris/tetsu.h>
#include <eris/romfont.h>
#include <eris/timer.h>
#include <eris/cd.h>
#include <eris/low/pad.h>
#include <eris/low/scsi.h>

#include <math.h>

#include "main.h"
#include "tinyfont.h"

#include "sin32tab.h"

#ifdef _8BPP
	unsigned char framebuffer[SCREEN_SIZE_IN_PIXELS];
#else
	unsigned short framebuffer[SCREEN_SIZE_IN_PIXELS];
#endif

static unsigned short myPal[256];


static int nframe = 0;

volatile int __attribute__ ((zda)) zda_timer_count = 0;

/* Declare this "noinline" to ensure that my_timer_irq() is not a leaf. */
__attribute__ ((noinline)) void increment_zda_timer_count (void)
{
	zda_timer_count++;
}

/* Simple test interrupt_handler that is not a leaf. */
/* Because it is not a leaf function, it will use the full IRQ preamble. */
__attribute__ ((interrupt)) void my_timer_irq (void)
{
	eris_timer_ack_irq();

	increment_zda_timer_count();
}


static void initTimer()
{
	// The PC-FX firmware leaves a lot of hardware actively generating
	// IRQs when a program starts, and it is only because the V810 has
	// interrupts-disabled that the firmware IRQ handlers are not run.
	//
	// You *must* mask/disable/reset the existing IRQ sources and init
	// new handlers before enabling the V810's interrupts!

	// Disable all interrupts before changing handlers.
	irq_set_mask(0x7F);

	// Replace firmware IRQ handlers for the Timer and HuC6270-A.
	//
	// This liberis function uses the V810's hardware IRQ numbering,
	// see FXGA_GA and FXGABOAD documents for more info ...
	irq_set_raw_handler(0x9, my_timer_irq);

	// Enable Timer interrupt.
	//
	// d6=Timer
	// d5=External
	// d4=KeyPad
	// d3=HuC6270-A
	// d2=HuC6272
	// d1=HuC6270-B
	// d0=HuC6273
	irq_set_mask(0x3F);

	// Reset and start the Timer.
	eris_timer_init();
	eris_timer_set_period(23864); /* approx 1/60th of a second */
	eris_timer_start(1);

	// Hmmm ... this needs to be cleared here for some reason ... there's
	// probably a bug to find somewhere!
	zda_timer_count = 0;

	// Allow all IRQs.
	//
	// This liberis function uses the V810's hardware IRQ numbering,
	// see FXGA_GA and FXGABOAD documents for more info ...
	irq_set_level(8);

	// Enable V810 CPU's interrupt handling.
	irq_enable();
}

/*static void vsync()
{
	while(eris_tetsu_get_raster() !=200) {};
}
*/

uint16 RGB2YUV(int r, int g, int b)
{
	uint16 yuv;

	int Y = (19595 * r + 38469 * g + 7471 * b) >> 16;
	int U = ((32243 * (b - Y)) >> 16) + 128;
	int V = ((57475 * (r - Y)) >> 16) + 128;

	if (Y <   0) Y =   0;
	if (Y > 255) Y = 255;
	if (U <   0) U =   0;
	if (U > 255) U = 255;
	if (V <   0) V =   0;
	if (V > 255) V = 255;

	yuv = (uint16)((Y<<8) | ((U>>4)<<4) | (V>>4));

	return yuv;
}

void setPal(int c, int r, int g, int b, uint16* pal)
{
	CLAMP(r, 0, 255)
	CLAMP(g, 0, 255)
	CLAMP(b, 0, 255)

	pal[c] = RGB2YUV(r,g,b);
}

void setPalGradient(int c0, int c1, int r0, int g0, int b0, int r1, int g1, int b1, uint16* pal)
{
	int i;
	const int dc = (c1 - c0);
	const int dr = ((r1 - r0) << 16) / dc;
	const int dg = ((g1 - g0) << 16) / dc;
	const int db = ((b1 - b0) << 16) / dc;

	r0 <<= 16;
	g0 <<= 16;
	b0 <<= 16;

	for (i = c0; i <= c1; i++)
	{
		setPal(i, r0>>16, g0>>16, b0>>16, pal);

		r0 += dr;
		g0 += dg;
		b0 += db;
	}
}


static void initPal()
{
	int i;
	for (i=0; i<4; ++i) {
		uint16 *palPtr = &myPal[i*64];
		setPalGradient(0,15, 0,0,0, 255,127,31, palPtr);
		setPalGradient(16,31, 255,127,31, 63,191,255, palPtr);
		setPalGradient(32,47, 63,191,255, 191,31,63, palPtr);
		setPalGradient(48,63, 191,31,63, 0,0,0, palPtr);
	}
}

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

static void testPlasma(int t)
{
	int x;
	int y = 200; //SCREEN_HEIGHT;
	unsigned char *fsy = (unsigned char*)fsin1_32;

	eris_king_set_kram_write(SCREEN_SIZE_IN_PIXELS * 4, 1);

	while(y!=0) {
		const unsigned char yp = ((fsy[(y + 3*t) & (NUM_SINES-1)] + fsy[(3*y + 2*t) & (NUM_SINES-1)]) >> 1);
		const unsigned int ysin = (yp << 24) | (yp << 16) | (yp << 8) | yp;
		unsigned int *fs1 = (unsigned int*)fsin1_32;

		x = SCREEN_WIDTH >> 2;
		while(x!=0) {
			const unsigned int c = *fs1++ + ysin;
			const unsigned short c0 = (c << 8) | ((c>>8) & 0x00FF);
			const unsigned short c1 = ((c >> 8) & 0xFF00) | (c>>24);
			eris_king_kram_write(c0);
			eris_king_kram_write(c1);
			--x;
		}
		--y;
	}
}

static int getFps()
{
	static int fps = 0;
	static int prev_sec = 0;
	static int prev_nframe = 0;

	const int curr_sec = zda_timer_count / 60;
	if (curr_sec != prev_sec) {
		fps = nframe - prev_nframe;
		prev_sec = curr_sec;
		prev_nframe = nframe;
	}
	return fps;
}

int main()
{
	//int t0, t1;

	initPal();
	initDisplay();
	initTimer();

	initTinyFonts();
	
	for(;;) {
		testPlasma(nframe++);

		drawNumber(16,216, getFps());
	}

	return 0;
}
