/* 
 * Generic mode timing module.
 */
#include <stdlib.h>

#include "timing.h"		/* Types. */

#include "driver.h"		/* for __svgalib_monitortype (remove me) */

/* Standard mode timings. */

/* HDG: set this to use Xorg's builtin default timings for 640x480 @ 72Hz
   instead of svgalib's old default timings. svgalib's timings
   cause my monitor to go in powersaving mode. */
#define USE_XORG_DEFAULT_TIMINGS

MonitorModeTiming __svgalib_standard_timings[] =
{
#define S __svgalib_standard_timings
/* 320x200 @ 70 Hz, 31.5 kHz hsync */
    {12588, 320, 336, 384, 400, 200, 204, 206, 225, DOUBLESCAN, S + 1},
/* 320x200 @ 83 Hz, 37.5 kHz hsync */
    {13333, 320, 336, 384, 400, 200, 204, 206, 225, DOUBLESCAN, S + 2},
/* 320x240 @ 60 Hz, 31.5 kHz hsync */
    {12588, 320, 336, 384, 400, 240, 245, 247, 263, DOUBLESCAN, S + 3},
/* 320x240 @ 72Hz, 38.5 kHz hsync */
    {15000, 320, 336, 384, 400, 240, 244, 246, 261, DOUBLESCAN, S + 4},
/* 320x400 @ 70 Hz, 31.5 kHz hsync */
    {12588, 320, 336, 384, 400, 400, 408, 412, 450, 0, S + 5},
/* 320x400 @ 83 Hz, 37.5 kHz hsync */
    {13333, 320, 336, 384, 400, 400, 408, 412, 450, 0, S + 6},
/* 320x480 @ 60 Hz, 31.5 kHz hsync */
    {12588, 320, 336, 384, 400, 480, 490, 494, 526, 0, S + 7},
/* 320x480 @ 72Hz, 38.5 kHz hsync */
    {15000, 320, 336, 384, 400, 480, 488, 492, 522, 0, S + 8},
/* 400x300 @ 56 Hz, 35.2 kHz hsync, 4:3 aspect ratio */
    {18000, 400, 416, 448, 512, 300, 301, 302, 312, DOUBLESCAN, S+9},
/* 400x300 @ 60 Hz, 37.8 kHz hsync */
    {20000, 400, 416, 480, 528, 300, 301, 303, 314, DOUBLESCAN, S+10},
/* 400x300 @ 72 Hz, 48.0 kHz hsync*/
    {25000, 400, 424, 488, 520, 300, 319, 322, 333, DOUBLESCAN, S+11},
/* 400x600 @ 56 Hz, 35.2 kHz hsync, 4:3 aspect ratio */
    {18000, 400, 416, 448, 512, 600, 602, 604, 624, 0, S+12},
/* 400x600 @ 60 Hz, 37.8 kHz hsync */
    {20000, 400, 416, 480, 528, 600, 602, 606, 628, 0, S+13},
/* 400x600 @ 72 Hz, 48.0 kHz hsync*/
    {25000, 400, 424, 488, 520, 600, 639, 644, 666, 0, S+14},
/* 512x384 @ 67Hz */
    {19600, 512, 522, 598, 646, 384, 418, 426, 454, 0, S+15 },
/* 512x384 @ 86Hz */
    {25175, 512, 522, 598, 646, 384, 418, 426, 454,0, S+16},
/* 512x480 @ 55Hz */
    {19600, 512, 522, 598, 646, 480, 500, 510, 550, 0, S+17},
/* 512x480 @ 71Hz */
    {25175, 512, 522, 598, 646, 480, 500, 510, 550,0, S+18},
/* 640x400 at 70 Hz, 31.5 kHz hsync */
    {25175, 640, 664, 760, 800, 400, 409, 411, 450, 0, S + 19},
/* 640x480 at 60 Hz, 31.5 kHz hsync */
    {25175, 640, 664, 760, 800, 480, 491, 493, 525, 0, S + 20},
/* 640x480 at 72 Hz, 36.5 kHz hsync */
#ifdef USE_XORG_DEFAULT_TIMINGS
    {31500, 640, 664, 704, 832, 480, 489, 491, 520, 0, S + 21},
#else
    {31500, 640, 680, 720, 864, 480, 488, 491, 521, 0, S + 21},
#endif
/* 800x600 at 56 Hz, 35.15 kHz hsync */
    {36000, 800, 824, 896, 1024, 600, 601, 603, 625, 0, S + 22},
/* 800x600 at 60 Hz, 37.8 kHz hsync */
    {40000, 800, 840, 968, 1056, 600, 601, 605, 628, PHSYNC | PVSYNC, S + 23},
/* 800x600 at 72 Hz, 48.0 kHz hsync */
    {50000, 800, 856, 976, 1040, 600, 637, 643, 666, PHSYNC | PVSYNC, S + 24},
/* 960x720 @ 70Hz */
    {66000, 960, 984, 1112, 1248, 720, 723, 729, 756, NHSYNC | NVSYNC, S+25},
/* 960x720* interlaced, 35.5 kHz hsync */
    {40000, 960, 984, 1192, 1216, 720, 728, 784, 817, INTERLACED, S + 26},
/* 1024x768 at 87 Hz interlaced, 35.5 kHz hsync */
    {44900, 1024, 1048, 1208, 1264, 768, 776, 784, 817, INTERLACED, S + 27},
/* 1024x768 at 100 Hz, 40.9 kHz hsync */
    {55000, 1024, 1048, 1208, 1264, 768, 776, 784, 817, INTERLACED, S + 28},
/* 1024x768 at 60 Hz, 48.4 kHz hsync */
    {65000, 1024, 1032, 1176, 1344, 768, 771, 777, 806, NHSYNC | NVSYNC, S + 29},
/* 1024x768 at 70 Hz, 56.6 kHz hsync */
    {75000, 1024, 1048, 1184, 1328, 768, 771, 777, 806, NHSYNC | NVSYNC, S + 30},
/* 1152x864 at 59.3Hz */
    {85000, 1152, 1214, 1326, 1600, 864, 870, 885, 895, 0, S+31},
/* 1280x1024 at 87 Hz interlaced, 51 kHz hsync */
    {80000, 1280, 1296, 1512, 1568, 1024, 1025, 1037, 1165, INTERLACED, S + 32},
/* 1024x768 at 76 Hz, 62.5 kHz hsync */
    {85000, 1024, 1032, 1152, 1360, 768, 784, 787, 823, 0, S + 33},
/* 1280x1024 at 60 Hz, 64.3 kHz hsync */
    {110000, 1280, 1328, 1512, 1712, 1024, 1025, 1028, 1054, 0, S + 34},
/* 1280x1024 at 74 Hz, 78.9 kHz hsync */
    {135000, 1280, 1312, 1456, 1712, 1024, 1027, 1030, 1064, 0, S + 35},
/* 1600x1200 at 60Hz */
    {162000, 1600, 1668, 1860, 2168, 1200, 1201, 1204, 1250, 0, S + 36},
/* 1600x1200 at 68Hz */
    {188500, 1600, 1792, 1856, 2208, 1200, 1202, 1205, 1256, 0, S + 37},
/* 1600x1200 at 75 Hz */
    {198000, 1600, 1616, 1776, 2112, 1200, 1201, 1204, 1250, 0, S + 38},
/* 720x540 at 56 Hz, 35.15 kHz hsync */
    {32400, 720, 744, 808, 920, 540, 541, 543, 563, 0, S + 39},
/* 720x540 at 60 Hz, 37.8 kHz hsync */
    {36000, 720, 760, 872, 952, 540, 541, 545, 565, 0, S + 40},
/* 720x540 at 72 Hz, 48.0 kHz hsync */
    {45000, 720, 768, 880, 936, 540, 552, 558, 599, 0, S + 41},
/* 1072x600 at 57 Hz interlaced, 35.5 kHz hsync */
    {44900, 1072, 1096, 1208, 1264, 600, 602, 604, 625, 0, S + 42},
/* 1072x600 at 65 Hz, 40.9 kHz hsync */
    {55000, 1072, 1096, 1208, 1264, 600, 602, 604, 625, 0, S + 43},
/* 1072x600 at 78 Hz, 48.4 kHz hsync */
    {65000, 1072, 1088, 1184, 1344, 600, 603, 607, 625, NHSYNC | NVSYNC, S + 44},
/* 1072x600 at 90 Hz, 56.6 kHz hsync */
    {75000, 1072, 1096, 1200, 1328, 768, 603, 607, 625, NHSYNC | NVSYNC, S + 45},
/* 1072x600 at 100 Hz, 62.5 kHz hsync */
    {85000, 1072, 1088, 1160, 1360, 768, 603, 607, 625, 0, NULL},
#undef S
};

#define NUMBER_OF_STANDARD_MODES \
	(sizeof(__svgalib_standard_timings) / sizeof(__svgalib_standard_timings[0]))

static MonitorModeTiming *user_timings = NULL;
static MonitorModeTiming *current_timing, *force_timing = NULL, new_timing;
static void GTF_calcTimings(double hPixels,double vLines,double freq,
        int type,int wantMargins,int wantInterlace, int wantDblscan, 
        MonitorModeTiming *mmt);
/*
 * SYNC_ALLOWANCE is in percent
 * 1% corresponds to a 315 Hz deviation at 31.5 kHz, 1 Hz at 100 Hz
 */
#define SYNC_ALLOWANCE 1

#define INRANGE(x,y) \
    ((x) > __svgalib_##y.min * (1.0f - SYNC_ALLOWANCE / 100.0f) && \
     (x) < __svgalib_##y.max * (1.0f + SYNC_ALLOWANCE / 100.0f))

/*
 * Check monitor spec.
 */
static int timing_within_monitor_spec(MonitorModeTiming * mmtp)
{
    float hsf;			/* Horz. sync freq in Hz */
    float vsf;			/* Vert. sync freq in Hz */

    hsf = mmtp->pixelClock * 1000.0f / mmtp->HTotal;
    vsf = hsf / mmtp->VTotal;
    if ((mmtp->flags & INTERLACED))
	vsf *= 2.0f;
    if ((mmtp->flags & DOUBLESCAN))
	vsf /= 2.0f;

    DPRINTF("hsf = %f (in:%d), vsf = %f (in:%d)\n",
	   hsf / 1000, (int) INRANGE(hsf, horizsync),
	   vsf, (int) INRANGE(vsf, vertrefresh));

    return INRANGE(hsf, horizsync) && INRANGE(vsf, vertrefresh);
}

void __svgalib_addusertiming(MonitorModeTiming * mmtp)
{
    MonitorModeTiming *newmmt;

    if (!(newmmt = malloc(sizeof(*newmmt))))
	return;
    *newmmt = *mmtp;
    if(newmmt->VSyncStart<newmmt->VDisplay+1)newmmt->VSyncStart=newmmt->VDisplay+1;
    if(newmmt->VSyncEnd<newmmt->VSyncStart+1)newmmt->VSyncEnd=newmmt->VSyncStart+1;
    newmmt->next = user_timings;
    user_timings = newmmt;
}

/*
 * The __svgalib_getmodetiming function looks up a mode in the standard mode
 * timings, choosing the mode with the highest dot clock that matches
 * the requested svgalib mode, and is supported by the hardware
 * (card limits, and monitor type). cardlimits points to a structure
 * of type CardSpecs that describes the dot clocks the card supports
 * at different depths. Returns non-zero if no mode is found.
 */

/*
 * findclock is an auxilliary function that checks if a close enough
 * pixel clock is provided by the card. Returns clock number if
 * succesful (a special number if a programmable clock must be used), -1
 * otherwise.
 */

/*
 * Clock allowance in 1/1000ths. 10 (1%) corresponds to a 250 kHz
 * deviation at 25 MHz, 1 MHz at 100 MHz
 */
#define CLOCK_ALLOWANCE 10

#define PROGRAMMABLE_CLOCK_MAGIC_NUMBER 0x1234

static int findclock(int clock, CardSpecs * cardspecs)
{
    int i;
    /* Find a clock that is close enough. */
    for (i = 0; i < cardspecs->nClocks; i++) {
	int diff;
	diff = cardspecs->clocks[i] - clock;
	if (diff < 0)
	    diff = -diff;
	if (diff * 1000 / clock < CLOCK_ALLOWANCE)
	    return i;
    }
    /* Try programmable clocks if available. */
    if (cardspecs->flags & CLOCK_PROGRAMMABLE) {
	int diff;
	diff = cardspecs->matchProgrammableClock(clock) - clock;
	if (diff < 0)
	    diff = -diff;
	if (diff * 1000 / clock < CLOCK_ALLOWANCE)
	    return PROGRAMMABLE_CLOCK_MAGIC_NUMBER;
    }
    /* No close enough clock found. */
    return -1;
}

static MonitorModeTiming *search_mode(MonitorModeTiming * timings,
				      int maxclock,
				      ModeInfo * modeinfo,
				      CardSpecs * cardspecs)
{
    int bestclock = 0;
    MonitorModeTiming *besttiming = NULL, *t;

    /*
     * bestclock is the highest pixel clock found for the resolution
     * in the mode timings, within the spec of the card and
     * monitor.
     * besttiming holds a pointer to timing with this clock.
     */

    /* Search the timings for the best matching mode. */
    for (t = timings; t; t = t->next)
	if (t->HDisplay == modeinfo->width
	    && t->VDisplay == modeinfo->height
            && ( (!(t->flags&INTERLACED)) || (!(cardspecs->flags&NO_INTERLACE)) )
	    && timing_within_monitor_spec(t)
	    && t->pixelClock <= maxclock
	    && t->pixelClock > bestclock
	    && cardspecs->mapHorizontalCrtc(modeinfo->bitsPerPixel,
					    t->pixelClock,
					    t->HTotal)
	    <= cardspecs->maxHorizontalCrtc
	/* Find the clock (possibly scaled by mapClock). */
	    && findclock(cardspecs->mapClock(modeinfo->bitsPerPixel,
					 t->pixelClock), cardspecs) != -1
	    ) {
	    bestclock = t->pixelClock;
	    besttiming = t;
	}
    return besttiming;
}

int __svgalib_getmodetiming(ModeTiming * modetiming, ModeInfo * modeinfo,
		  CardSpecs * cardspecs)
{
    int maxclock, desiredclock;
    MonitorModeTiming *besttiming=NULL;

    if(force_timing){
       if(timing_within_monitor_spec(force_timing) && 
          force_timing->HDisplay == modeinfo->width && 
          force_timing->VDisplay == modeinfo->height)
       {
            besttiming=force_timing;
       };
    };

    /* Get the maximum pixel clock for the depth of the requested mode. */
    if (modeinfo->bitsPerPixel == 4)
	maxclock = cardspecs->maxPixelClock4bpp;
    else if (modeinfo->bitsPerPixel == 8)
	maxclock = cardspecs->maxPixelClock8bpp;
    else if (modeinfo->bitsPerPixel == 16) {
	if ((cardspecs->flags & NO_RGB16_565)
	    && modeinfo->greenWeight == 6)
	    return 1;		/* No 5-6-5 RGB. */
	maxclock = cardspecs->maxPixelClock16bpp;
    } else if (modeinfo->bitsPerPixel == 24)
	maxclock = cardspecs->maxPixelClock24bpp;
    else if (modeinfo->bitsPerPixel == 32)
	maxclock = cardspecs->maxPixelClock32bpp;
    else
	maxclock = 0;

    /*
     * Check user defined timings first.
     * If there is no match within these, check the standard timings.
     */
    if(!besttiming)
        besttiming = search_mode(user_timings, maxclock, modeinfo, cardspecs);
    if (!besttiming) {
	besttiming = search_mode(__svgalib_standard_timings, maxclock, modeinfo, cardspecs);
	if (!besttiming)
	    return 1;
    }
    /*
     * Copy the selected timings into the result, which may
     * be adjusted for the chipset.
     */

    modetiming->flags = besttiming->flags;
    modetiming->pixelClock = besttiming->pixelClock;	/* Formal clock. */

    /*
     * We know a close enough clock is available; the following is the
     * exact clock that fits the mode. This is probably different
     * from the best matching clock that will be programmed.
     */
    desiredclock = cardspecs->mapClock(modeinfo->bitsPerPixel,
				       besttiming->pixelClock);

    /* Fill in the best-matching clock that will be programmed. */
    modetiming->selectedClockNo = findclock(desiredclock, cardspecs);
    if (modetiming->selectedClockNo == PROGRAMMABLE_CLOCK_MAGIC_NUMBER) {
	modetiming->programmedClock =
	    cardspecs->matchProgrammableClock(desiredclock);
	modetiming->flags |= USEPROGRCLOCK;
    } else
	modetiming->programmedClock = cardspecs->clocks[
					    modetiming->selectedClockNo];
    modetiming->HDisplay = besttiming->HDisplay;
    modetiming->HSyncStart = besttiming->HSyncStart;
    modetiming->HSyncEnd = besttiming->HSyncEnd;
    modetiming->HTotal = besttiming->HTotal;
    if (cardspecs->mapHorizontalCrtc(modeinfo->bitsPerPixel,
				     modetiming->programmedClock,
				     besttiming->HTotal)
	!= besttiming->HTotal) {
	/* Horizontal CRTC timings are scaled in some way. */
	modetiming->CrtcHDisplay =
	    cardspecs->mapHorizontalCrtc(modeinfo->bitsPerPixel,
					 modetiming->programmedClock,
					 besttiming->HDisplay);
	modetiming->CrtcHSyncStart =
	    cardspecs->mapHorizontalCrtc(modeinfo->bitsPerPixel,
					 modetiming->programmedClock,
					 besttiming->HSyncStart);
	modetiming->CrtcHSyncEnd =
	    cardspecs->mapHorizontalCrtc(modeinfo->bitsPerPixel,
					 modetiming->programmedClock,
					 besttiming->HSyncEnd);
	modetiming->CrtcHTotal =
	    cardspecs->mapHorizontalCrtc(modeinfo->bitsPerPixel,
					 modetiming->programmedClock,
					 besttiming->HTotal);
	modetiming->flags |= HADJUSTED;
    } else {
	modetiming->CrtcHDisplay = besttiming->HDisplay;
	modetiming->CrtcHSyncStart = besttiming->HSyncStart;
	modetiming->CrtcHSyncEnd = besttiming->HSyncEnd;
	modetiming->CrtcHTotal = besttiming->HTotal;
    }
    modetiming->VDisplay = besttiming->VDisplay;
    modetiming->VSyncStart = besttiming->VSyncStart;
    modetiming->VSyncEnd = besttiming->VSyncEnd;
    modetiming->VTotal = besttiming->VTotal;
    if (modetiming->flags & DOUBLESCAN){
	modetiming->VDisplay <<= 1;
	modetiming->VSyncStart <<= 1;
	modetiming->VSyncEnd <<= 1;
	modetiming->VTotal <<= 1;
    }
    modetiming->CrtcVDisplay = modetiming->VDisplay;
    modetiming->CrtcVSyncStart = modetiming->VSyncStart;
    modetiming->CrtcVSyncEnd = modetiming->VSyncEnd;
    modetiming->CrtcVTotal = modetiming->VTotal;
    if (((modetiming->flags & INTERLACED)
	 && (cardspecs->flags & INTERLACE_DIVIDE_VERT))
	|| (modetiming->VTotal >= 1024
	    && (cardspecs->flags & GREATER_1024_DIVIDE_VERT))) {
	/*
	 * Card requires vertical CRTC timing to be halved for
	 * interlaced modes, or for all modes with vertical
	 * timing >= 1024.
	 */
	modetiming->CrtcVDisplay /= 2;
	modetiming->CrtcVSyncStart /= 2;
	modetiming->CrtcVSyncEnd /= 2;
	modetiming->CrtcVTotal /= 2;
	modetiming->flags |= VADJUSTED;
    }
    current_timing=besttiming;
    return 0;			/* Succesful. */
}

int vga_getcurrenttiming(int *pixelClock,
       			 int *HDisplay,
                         int *HSyncStart,
                         int *HSyncEnd,
                         int *HTotal,
                         int *VDisplay,
                         int *VSyncStart,
                         int *VSyncEnd,
                         int *VTotal,
                         int *flags)
{
   if(current_timing){
      *pixelClock=current_timing->pixelClock;
      *HDisplay=current_timing->HDisplay;
      *HSyncStart=current_timing->HSyncStart;
      *HSyncEnd=current_timing->HSyncEnd;
      *HTotal=current_timing->HTotal;
      *VDisplay=current_timing->VDisplay;
      *VSyncStart=current_timing->VSyncStart;
      *VSyncEnd=current_timing->VSyncEnd;
      *VTotal=current_timing->VTotal;
      *flags=current_timing->flags;
      return 0;
   }
   return 1;
};

int vga_changetiming(int pixelClock,
   		     int HDisplay,
   		     int HSyncStart,
                     int HSyncEnd,
                     int HTotal,
                     int VDisplay,
                     int VSyncStart,
                     int VSyncEnd,
                     int VTotal,
                     int flags) {
  if(current_timing){
     new_timing=*current_timing;
     new_timing.pixelClock+=pixelClock;
     new_timing.HDisplay+=HDisplay;
     new_timing.HSyncStart+=HSyncStart;
     new_timing.HSyncEnd+=HSyncEnd;
     new_timing.HTotal+=HTotal;
     new_timing.VDisplay+=VDisplay;
     new_timing.VSyncStart+=VSyncStart;
     new_timing.VSyncEnd+=VSyncEnd;
     new_timing.VTotal+=VTotal;
     force_timing=&new_timing;
     vga_setmode(CM|0x8000);
     force_timing=NULL;
  };
  
  return 1;
};

static int find_up_timing(int x, int y, int *bestx, int *besty, MonitorModeTiming **bestmodetiming)
{
      MonitorModeTiming *t;
      int bestclock=0;
      int mode_ar;

      *bestmodetiming=NULL;
      *bestx=*besty=4096;
      for (t = user_timings; t; t = t->next) {
	     if ((mode_ar=1000*t->VDisplay/t->HDisplay)<=765
                && mode_ar>=735
                && t->HDisplay >= x
                && t->VDisplay >= y 
                && timing_within_monitor_spec(t)
                && t->HDisplay <= *bestx
                && t->VDisplay <= *besty
                && t->pixelClock>=bestclock
                ) {
                   bestclock = t->pixelClock;
                   *bestx=t->HDisplay;
                   *besty=t->VDisplay;
                   *bestmodetiming = t;
                   };
      };
      for (t = __svgalib_standard_timings; t; t = t->next) {
	     if (t->HDisplay >= x
                && t->VDisplay >= y
                && timing_within_monitor_spec(t)
                && t->HDisplay <= *bestx
                && t->VDisplay <= *besty
                && t->pixelClock>=bestclock
                ) {
                   bestclock = t->pixelClock;
                   *bestx=t->HDisplay;
                   *besty=t->VDisplay;
                   *bestmodetiming = t;
                   };
      };
      return *bestmodetiming!=NULL;
};

static int find_down_timing(int x, int y, int *bestx, int *besty, MonitorModeTiming **bestmodetiming)
{
      MonitorModeTiming *t;
      int bestclock=0;
      int mode_ar;

      *bestmodetiming=NULL;
      *bestx=*besty=0;
      for (t = user_timings; t; t = t->next) {
	     if ((mode_ar=1000*t->VDisplay/t->HDisplay)<=765
                && mode_ar>=735
                && t->HDisplay <= x
                && t->VDisplay <= y 
                && timing_within_monitor_spec(t)
                && t->HDisplay >= *bestx
                && t->VDisplay >= *besty
                && t->pixelClock>=bestclock
                ) {
                   bestclock = t->pixelClock;
                   *bestx=t->HDisplay;
                   *besty=t->VDisplay;
                   *bestmodetiming = t;
                   };
      };
      for (t = __svgalib_standard_timings; t; t = t->next) {
	     if (t->HDisplay <= x
                && t->VDisplay <= y
                && timing_within_monitor_spec(t)
                && t->HDisplay >= *bestx
                && t->VDisplay >= *besty
                && t->pixelClock>=bestclock
                ) {
                   bestclock = t->pixelClock;
                   *bestx=t->HDisplay;
                   *besty=t->VDisplay;
                   *bestmodetiming = t;
                   };
      };
      return *bestmodetiming!=NULL;
};

int vga_guesstiming(int x, int y, int clue, int arg)
{
/* This functions tries to add timings that fit a specific mode,
   by changing the timings of a similar mode
   
currently only works for x:y = 4:3, clue means:
0- scale down timing of a higher res mode
1- scale up timings of a lower res mode
*/

   MonitorModeTiming mmt, *bestmodetiming = NULL ;
   int bestx, besty, flag, mx, my /*, bestclock */ ;

   int aspect_ratio=1000*y/x;
   switch(clue) {
      case 0: /* 0,1 only 4:3 ratio, find close mode, and up/down scale timing */
      case 1:
          if((aspect_ratio>765)||(aspect_ratio<735))return 0;
          if(clue==0)find_up_timing(x,y,&bestx,&besty,&bestmodetiming);
          if(clue==1)find_down_timing(x,y,&bestx,&besty,&bestmodetiming);
          if(bestmodetiming){
             
             mmt=*bestmodetiming;
             
             mmt.pixelClock=(mmt.pixelClock*x)/bestx;
             mmt.HDisplay=x;
             mmt.VDisplay=y;
             mmt.HSyncStart=(mmt.HSyncStart*x)/bestx;
             mmt.HSyncEnd=(mmt.HSyncEnd*x)/bestx;
             mmt.HTotal=(mmt.HTotal*x)/bestx;
             mmt.VSyncStart=(mmt.VSyncStart*x)/bestx;
             mmt.VSyncEnd=(mmt.VSyncEnd*x)/bestx;
             mmt.VTotal=(mmt.VTotal*x)/bestx;
             __svgalib_addusertiming(&mmt);
             return 1;
          };
          break;
      case 2: /* Use GTF, caller provides all parameters. */
          flag = arg>>16;
          GTF_calcTimings(x , y, arg&0xffff, flag&3, flag&4, flag&8, flag&16, &mmt); 
          __svgalib_addusertiming(&mmt);
          return 1;
      
          
      case 256: /* 256,257: find a 4:3 mode with y close to requested, and */
      case 257: /* up/down scale timing */
          mx=y*4/3;
          if((clue&1)==0)find_up_timing(mx,y,&bestx,&besty,&bestmodetiming);
          if((clue&1)==1)find_down_timing(mx,y,&bestx,&besty,&bestmodetiming);
          if(bestmodetiming){
             
             mmt=*bestmodetiming;
             
             mmt.pixelClock=(mmt.pixelClock*x)/bestx;
             mmt.HDisplay=x;
             mmt.HSyncStart=(mmt.HSyncStart*x)/bestx;
             mmt.HSyncEnd=(mmt.HSyncEnd*x)/bestx;
             mmt.HTotal=(mmt.HTotal*x)/bestx;
             mmt.VDisplay=y;
             mmt.VSyncStart=(mmt.VSyncStart*mx)/bestx;
             mmt.VSyncEnd=(mmt.VSyncEnd*mx)/bestx;
             mmt.VTotal=(mmt.VTotal*mx)/bestx;
             __svgalib_addusertiming(&mmt);
             return 1;
          };
          break;
      case 258: /* 258,259: find a 4:3 mode with x close to requested, and */
      case 259: /* up/down scale timing */
          my=(x*3)>>2;
          if((clue&1)==0)find_up_timing(x,my,&bestx,&besty,&bestmodetiming);
          if((clue&1)==1)find_down_timing(x,my,&bestx,&besty,&bestmodetiming);
          if(bestmodetiming){
             
             mmt=*bestmodetiming;
             
             mmt.pixelClock=(mmt.pixelClock*x)/bestx;
             mmt.HDisplay=x;
             mmt.HSyncStart=(mmt.HSyncStart*x)/bestx;
             mmt.HSyncEnd=(mmt.HSyncEnd*x)/bestx;
             mmt.HTotal=(mmt.HTotal*x)/bestx;
             mmt.VDisplay=y;
             mmt.VSyncStart=(mmt.VSyncStart*y)/besty;
             mmt.VSyncEnd=(mmt.VSyncEnd*y)/besty;
             mmt.VTotal=(mmt.VTotal*y)/besty;
             __svgalib_addusertiming(&mmt);
             return 1;
          };
          break;
   };
   return 0;
};

/* Everything from here to the end of the file is copyright by 
scitechsoft. See their original program in utils/gtf subdirectory */
   
typedef struct {
	double	margin;			/* Margin size as percentage of display		*/
	double	cellGran;		/* Character cell granularity			*/
	double	minPorch;		/* Minimum front porch in lines/chars		*/
	double	vSyncRqd;		/* Width of V sync in lines			*/
	double	hSync;			/* Width of H sync as percent of total		*/
	double	minVSyncBP;		/* Minimum vertical sync + back porch (us)	*/
	double	m;			/* Blanking formula gradient			*/
	double	c;			/* Blanking formula offset			*/
	double  k;			/* Blanking formula scaling factor		*/
	double  j;			/* Blanking formula scaling factor weight	*/
} GTF_constants;

static GTF_constants GC = {
	1.8,				/* Margin size as percentage of display		*/
	8,				/* Character cell granularity			*/
	1,				/* Minimum front porch in lines/chars		*/
	3,				/* Width of V sync in lines			*/
	8,				/* Width of H sync as percent of total		*/
	550,				/* Minimum vertical sync + back porch (us)	*/
	600,				/* Blanking formula gradient			*/
	40,				/* Blanking formula offset			*/
	128,				/* Blanking formula scaling factor		*/
	20,				/* Blanking formula scaling factor weight	*/
};

static double round(double v)
{
   	double u=v;
        int j;
        
        if(u<0) u=-u;
        u=u+0.5;
        j=u;
        if(v<0) j=-j;

	return j;
}

static double sqrt(double u)
{
   	double v,w;
        int i;
        v=0;
        
        if(u==0) return 0;
        if(u<0) u=-u;
        w=u;
        if(u<1) w=1;
        for(i=0;i<50;i++){
              w=w/2;
              if(v*v==u)break;
              if(v*v<u)v=v+w; 
              if(v*v>u)v=v-w; 
        };
        
   	return v;
}
static void GetInternalConstants(GTF_constants *c)
{
	c->margin = GC.margin;
	c->cellGran = round(GC.cellGran);
	c->minPorch = round(GC.minPorch);
	c->vSyncRqd = round(GC.vSyncRqd);
	c->hSync = GC.hSync;
	c->minVSyncBP = GC.minVSyncBP;
	if (GC.k == 0)
		c->k = 0.001;
	else
		c->k = GC.k;
	c->m = (c->k / 256) * GC.m;
	c->c = (GC.c - GC.j) * (c->k / 256) + GC.j;
	c->j = GC.j;
}

static void GTF_calcTimings(double hPixels,double vLines,double freq,
	int type,int wantMargins,int wantInterlace, int wantDblscan, 
        MonitorModeTiming *mmt)
{
	double  		interlace,vFieldRate,hPeriod=0;
	double  		topMarginLines,botMarginLines;
	double  		leftMarginPixels,rightMarginPixels;
	double			hPeriodEst=0,vSyncBP=0,vBackPorch=0;
	double			vTotalLines=0,vFieldRateEst;
	double			hTotalPixels,hTotalActivePixels,hBlankPixels;
	double			idealDutyCycle=0,hSyncWidth,hSyncBP,hBackPorch;
	double			idealHPeriod;
	double			vFreq,hFreq,dotClock;
	GTF_constants   c;

	/* Get rounded GTF constants used for internal calculations */
	GetInternalConstants(&c);

	/* Move input parameters into appropriate variables */
	vFreq = hFreq = dotClock = freq;

	/* Round pixels to character cell granularity */
	hPixels = round(hPixels / c.cellGran) * c.cellGran;

	/* For interlaced mode halve the vertical parameters, and double
	 * the required field refresh rate.
	 */
         
        if(wantDblscan) vLines = vLines * 2;
	if (wantInterlace) {
		vLines = round(vLines / 2);
		vFieldRate = vFreq * 2;
		dotClock = dotClock * 2;
		interlace = 0.5;
		}
	else {
		vFieldRate = vFreq;
		interlace = 0;
		}

	/* Determine the lines for margins */
	if (wantMargins) {
		topMarginLines = round(c.margin / 100 * vLines);
		botMarginLines = round(c.margin / 100 * vLines);
		}
	else {
		topMarginLines = 0;
		botMarginLines = 0;
		}

	if (type != GTF_lockPF) {
		if (type == GTF_lockVF) {
			/* Estimate the horizontal period */
			hPeriodEst = ((1/vFieldRate) - (c.minVSyncBP/1000000)) /
				(vLines + (2*topMarginLines) + c.minPorch + interlace) * 1000000;

			/* Find the number of lines in vSync + back porch */
			vSyncBP = round(c.minVSyncBP / hPeriodEst);
			}
		else if (type == GTF_lockHF) {
			/* Find the number of lines in vSync + back porch */
			vSyncBP = round((c.minVSyncBP * hFreq) / 1000);
			}

		/* Find the number of lines in the V back porch alone */
		vBackPorch = vSyncBP - c.vSyncRqd;

		/* Find the total number of lines in the vertical period */
		vTotalLines = vLines + topMarginLines + botMarginLines + vSyncBP
			+ interlace + c.minPorch;

		if (type == GTF_lockVF) {
			/* Estimate the vertical frequency */
			vFieldRateEst = 1000000 / (hPeriodEst * vTotalLines);

			/* Find the actual horizontal period */
			hPeriod = (hPeriodEst * vFieldRateEst) / vFieldRate;

			/* Find the actual vertical field frequency */
			vFieldRate = 1000000 / (hPeriod * vTotalLines);
			}
		else if (type == GTF_lockHF) {
			/* Find the actual vertical field frequency */
			vFieldRate = (hFreq / vTotalLines) * 1000;
			}
		}

	/* Find the number of pixels in the left and right margins */
	if (wantMargins) {
		leftMarginPixels = round(hPixels * c.margin) / (100 * c.cellGran);
		rightMarginPixels = round(hPixels * c.margin) / (100 * c.cellGran);
		}
	else {
		leftMarginPixels = 0;
		rightMarginPixels = 0;
		}

	/* Find the total number of active pixels in image + margins */
	hTotalActivePixels = hPixels + leftMarginPixels + rightMarginPixels;

	if (type == GTF_lockVF) {
		/* Find the ideal blanking duty cycle */
		idealDutyCycle = c.c - ((c.m * hPeriod) / 1000);
		}
	else if (type == GTF_lockHF) {
		/* Find the ideal blanking duty cycle */
		idealDutyCycle = c.c - (c.m / hFreq);
		}
	else if (type == GTF_lockPF) {
		/* Find ideal horizontal period from blanking duty cycle formula */
		idealHPeriod = (((c.c - 100) + (sqrt(((100-c.c)*(100-c.c)) +
			(0.4 * c.m * (hTotalActivePixels + rightMarginPixels +
			leftMarginPixels) / dotClock)))) / (2 * c.m)) * 1000;

		/* Find the ideal blanking duty cycle */
		idealDutyCycle = c.c - ((c.m * idealHPeriod) / 1000);
		}

	/* Find the number of pixels in blanking time */
	hBlankPixels = round((hTotalActivePixels * idealDutyCycle) /
		((100 - idealDutyCycle) * 2 * c.cellGran)) * (2 * c.cellGran);

	/* Find the total number of pixels */
	hTotalPixels = hTotalActivePixels + hBlankPixels;

	/* Find the horizontal back porch */
	hBackPorch = round((hBlankPixels / 2) / c.cellGran) * c.cellGran;

	/* Find the horizontal sync width */
	hSyncWidth = round(((c.hSync/100) * hTotalPixels) / c.cellGran) * c.cellGran;

	/* Find the horizontal sync + back porch */
	hSyncBP = hBackPorch + hSyncWidth;

	if (type == GTF_lockPF) {
		/* Find the horizontal frequency */
		hFreq = (dotClock / hTotalPixels) * 1000;

		/* Find the horizontal period */
		hPeriod = 1000 / hFreq;

		/* Find the number of lines in vSync + back porch */
		vSyncBP = round((c.minVSyncBP * hFreq) / 1000);

		/* Find the number of lines in the V back porch alone */
		vBackPorch = vSyncBP - c.vSyncRqd;

		/* Find the total number of lines in the vertical period */
		vTotalLines = vLines + topMarginLines + botMarginLines + vSyncBP
			+ interlace + c.minPorch;

		/* Find the actual vertical field frequency */
		vFieldRate = (hFreq / vTotalLines) * 1000;
		}
	else {
		if (type == GTF_lockVF) {
			/* Find the horizontal frequency */
			hFreq = 1000 / hPeriod;
			}
		else if (type == GTF_lockHF) {
			/* Find the horizontal frequency */
			hPeriod = 1000 / hFreq;
			}

		/* Find the pixel clock frequency */
		dotClock = hTotalPixels / hPeriod;
		}

	/* Find the vertical frame frequency */
	if (wantInterlace) {
		vFreq = vFieldRate / 2;
		}
	else
		vFreq = vFieldRate;

	mmt->pixelClock = dotClock;

	/* Determine the vertical timing parameters */
	mmt->HTotal = hTotalPixels;
	mmt->HDisplay = hTotalActivePixels;
	mmt->HSyncStart = mmt->HTotal - hSyncBP;
	mmt->HSyncEnd = mmt->HTotal - hBackPorch;

	/* Determine the vertical timing parameters */
	mmt->VTotal = vTotalLines;
	mmt->VDisplay = vLines;
	mmt->VSyncStart = mmt->VTotal - vSyncBP;
	mmt->VSyncEnd = mmt->VTotal - vBackPorch;

        if(wantDblscan) {
	    mmt->VTotal >>= 1;
	    mmt->VDisplay >>= 1 ;
	    mmt->VSyncStart >>= 1 ;
	    mmt->VSyncEnd >>= 1 ;
        };

        if(wantInterlace) {
	    mmt->VTotal <<= 1;
	    mmt->VDisplay <<= 1 ;
	    mmt->VSyncStart <<= 1 ;
	    mmt->VSyncEnd <<= 1 ;
        };

        mmt->flags = NHSYNC | PVSYNC | ((wantInterlace) ? INTERLACED : 0) 
                            | ((wantDblscan) ? DOUBLESCAN : 0);
}

