/* MOLEFEST V1.05
 * (C)1998 Robin Burrows <rburrows@bigfoot.com>
 * Source code release 23/12/2001
 *
 * DISCLAIMER:
 * This source code is provided 'as is' with no warranty or guarantees,
 * you use it at your own risk, and by doing so agree that the author is
 * in no way liable or responsible for any damage or loss of time, data,
 * or anything else that may occur.
 *
 * AGREEMENT:
 * You may freely use, alter, compile, distribute and study this sourcecode,
 * with only two restrictions:
 * 1) You may NOT distribute a derived version without crediting me,
 * Robin Burrows as the writer of the original source.
 * 2) You may NOT charge money for this source, or executables compiled
 * from it or an altered version (in addition, see the SEAL licence
 * agreement in the SEAL SDK which says that only free software may be
 * made with SEAL unless it is licenced).
 *
 * COMPILING:
 * You will need the files from molefest.zip (in particular moledata.dat)
 * Set the defines below as appropriate, then: for DJGPP:
 * gcc molefest.c -O2 -Wall -o molefest.exe -lalleg -lm
 * Windows: See documentation for WinAllegro
 * SEAL: See documentation and examples for SEAL SDK
 * To compile my executables I used DJGPP 2.01 for DOS, mingw32 for Windows
 *
 * All Copyrights and trademarks are property of their respective owners
 * DJGPP:    DJ Delorie http://www.delorie.com/djgpp/
 * Allegro:  Shawn Hargreaves http://www.talula.demon.co.uk/allegro/
 * JGMod:    Jeffery Guan Foo Wah http://surf.to/jgmod
 * SEAL:     Carlos Hasan (no URL given, try any good game development site)
 *
 * Molefest: http://www.geocities.com/SiliconValley/Vista/7336/robcstf.htm
 */

/* If you are using WinAllegro to make a DirectX version, uncomment this: */
//#define __WIN32__

/* If you comment out SOUNDON, you must also comment out MUSICON and USESEAL */
//#define SOUNDON 1
/* If you want to use have JGMOD, uncomment the following line */
//#define MUSICON 1
/* If you want to use SEAL (Win32/DirectX only) uncomment next line */
//#define USESEAL 1

/* Standard includes... */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <allegro.h>
#ifdef __WIN32__
/* For WinAllegro, this wrapper file by George Foot is needed */
//#include "wrapper.h"
#include <winalleg.h>

#endif

#define random rand
#define srandom srand

/* Here are the two music systems, JGMod for DOS builds, SEAL for Windows */
/* Use one, or none, but not both */
#ifdef MUSICON
#include <jgmod.h>
void SEALPlaySnd (SAMPLE *, int, int, int, int);
#endif
#ifdef USESEAL
#include "midasdll.h"
void SEALResumeMod (int);
void SEALOpen (void);
void SEALClose (void);
int SEALStartMusic (int, int);
void SEALStopMusic (void);
void SEALPlaySnd (SAMPLE *, int, int, int, int);
#endif

/* This is produced by the Allegro grabber for moledata.dat. You can view
 * moledata.dat with the Allegro grabber */
#include "moledata.h"

/* Although I have the screen size and depth defined here, I think it's very
 * unlikely you can change it without altering other parts of the source */
#define SCRWIDTH 640
#define SCRHEIGHT 480
#define SCRDEPTH 8

/* Some defines for the 3D starfield on the intro */
#define STARDEPTH 2048
#define STARDIST 512
#define MAXSTARS 1000
#define NUMSTARS 1000
#define STARINC 10
#define STARSPEED 16

/* These say how many pixels from the left and top of the screen the play grid
 * is, and how wide and tall it is (in blocks), again, you are unlikely to just
 * be able to change these */
#define GXOFF 32
#define GYOFF 20
#define BLX 9
#define BLY 7

/* Some data structures for various objects in the game... */
typedef struct {
float xpos, ypos, zpos;
int oldx, oldy, oldx2, oldy2;
int colour;
} STAR;

typedef struct {
int oldx, oldy;
float x, y, destx, desty;
int angle, letnumber;
} LETTER;

typedef struct {
int x, y, state, colour;
} OPTPANEL;

typedef struct {
int hiscore, pad1;
char level, difficulty, pad2, pad3;
char name[20];
} HIENTRY;

/* The Molefest global variables */
HIENTRY highscores[10];
int vsyncon = 0;
volatile int fps = 0;
volatile int fpscounter = 0;
volatile int fpsdelay = 0;
volatile int fpstimer = 0;
STAR * mystars;
BITMAP * display1;	/* First video memory page */
BITMAP * display2;	/* Second video memory page */
BITMAP * dbuffer;		/* Video memory page to draw to (ie. not currently shown) */
BITMAP * pieces[8];	/* Pieces are: Blank, horiz, vert, cross, r, mirror r, mirror L, L created in InitGame() */
BITMAP * digbm, * rockbm; /* digbm is the block the mole is currently digging through, rockbm is the rock */
DATAFILE * mydata;
int scroffset, nextshunt, score, level, endgame, crosses, crossnote, blockstogo;
int puttimer, putx, puty, moletimer, molex, moley, moledelay, moledelaydef;
int moleangle, moleaframe, moleinx, moleiny, moleto, moleprog;
unsigned char putblock;
int drawbx, drawby, drawtimes;

char topmessage[60];	/* Current message at top of game screen */
int messagedur;
volatile int redraw;	/* messagedur is how long (in frames) message is displayed for.
				 * redraw is used by winallegro for redrawing when DirectX is restored */

#ifdef MUSICON
JGMOD *myxm;
#endif

/* Prefs */
int gammalevel;
int sfxvol, modvol;
int difficulty;
int msespd;
int fliptype;

/* My silly playgrid system. bit 7 = place block?, bits 6,5,4 = current 'piece', 3,2,1,0 = background */
unsigned char thegrid[BLY][BLX];
unsigned char crossgrid[BLY][BLX];
/* nextblock is the next 6 blocks to appear, shown in topleft of game screen */
unsigned char nextblock[6];
/* so blocks are never 'shut out' (ie. a certain block is not ignored) */
unsigned char givenblock[8];

/* Level names (well now you know... :) */
char * leveltxt[] = {
NULL,
"1: The Garden",
"2: Farmer's Field",
"3: The Lawn",
"4: Badlands",
"5: Flower Beds",
"6: Alien World",
"7: Its...",
"8: Mole Finale",
"9: Cricket Turf",
"10: Mole's Den",
"11: Mole 2000",
"12: Dial-a-Mole",
"Mole in a suitcase",
"14: It's a hole",
"15: Salamander",
"16: Central Cavern",
"17: Misnomer",
"18: The GPF",
"19: Bistro 88",
"20: Mole Heaven"
};

PALETTE mypal;

/* Default highscore entries */
HIENTRY defhighs[] = {
{ 619600, 0, 12, 1, 0, 0, "Alltime best" },
{ 90000, 0, 13, 2, 0, 0, "Super Mole" },
{ 80000, 0, 12, 2, 0, 0, "Great Mole" },
{ 70000, 0, 11, 2, 0, 0, "Notable Mole" },
{ 60000, 0, 10, 1, 0, 0, "Competent Mole" },
{ 50000, 0, 9, 1, 0, 0, "Average Mole" },
{ 40000, 0, 8, 1, 0, 0, "Also-ran Mole" },
{ 30000, 0, 7, 1, 0, 0, "Cabbage Mole" },
{ 20000, 0, 6, 0, 0, 0, "No-hoper Mole" },
{ 10000, 0, 5, 0, 0, 0, "Dead Mole" }
};

/* Default positions and speeds for spinning letters on intro */
LETTER defletters[] = {
{ 0, 0, SCRWIDTH, SCRHEIGHT/2-0+4, 62, SCRHEIGHT/2-32+4, 255, -1 },
{ 0, 0, -100, SCRHEIGHT/2-100+32, 184, SCRHEIGHT/2-32+32, 255, -1 },
{ 0, 0, SCRWIDTH, SCRHEIGHT/2-100, 244, SCRHEIGHT/2-32, 255, -1 },
{ 0, 0, -100, SCRHEIGHT/2-0+32, 286, SCRHEIGHT/2-32+32, 255, -1 },
{ 0, 0, SCRWIDTH, SCRHEIGHT/2-0+2, 348, SCRHEIGHT/2-32+2, 255, -1 },
{ 0, 0, -100, SCRHEIGHT/2-100+32, 398, SCRHEIGHT/2-32+32, 255, -1 },
{ 0, 0, SCRWIDTH, SCRHEIGHT/2-100+32, 462, SCRHEIGHT/2-32+32, 255, -1 },
{ 0, 0, -100, SCRHEIGHT/2-0+14, 518, SCRHEIGHT/2-32+14, 255, -1 },
};

/* For when the blue main menu panel appears */
char flipnumbers[] = {
0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 6, 6, 7, 7, 8,
9, 10, 10, 11, 11, 12, 12, 13, 13, 13, 14, 14, 14, 15, 15, 15, 15, 16, 16, 16, 16, 16, 16,
15, 15, 15, 15, 14, 14, 14, 13, 13, 13, 12, 12, 11, 11, 10, 10, 9,
8, 7, 7, 6, 6, 5, 5, 4, 4, 4, 3, 3, 3, 2, 2, 2, 1, 1, 1, 1, 0, 0,
0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 6, 6, 7, 7, 8,
9, 10, 10, 11, 11, 12, 12, 13, 13, 13, 14, 14, 14, 15, 15, 15, 15, 16, 16, 16, 16, 16, 16, -1 };

/* Currently selected item 'glows' */
char glownumbers[] = {
0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 8, 7, 6, 5, 4, 3, 2, 1, -1 };


void WinFlip (void)
{
	if (fliptype == 0) {
		if (scroffset) { show_video_bitmap (display2); dbuffer = display1; }
		else { show_video_bitmap (display1); dbuffer = display2; }
	}
	if (fliptype == 3) {
		blit (dbuffer, screen, 0, 0, 0, 0, SCRWIDTH, SCRHEIGHT);
		if (dbuffer == display1) dbuffer = display2;
		else dbuffer = display1;
	}
}

/* Keep the game speed correct on all systems... */
void fpsinterrupt (void)
{
	fpstimer--; if (fpstimer<0)
	{
		fps = fpscounter; fpscounter = 0;
		fpstimer = 59;
	}
	fpsdelay ++;
}
END_OF_FUNCTION (fpsinterrupt);

/* My own palette setter to allow for brightness, thanks to Brian Bacon, was it? */
void setmypal(PALETTE srcplt)
{
PALETTE gamma_pal;      // palette used for gamma correction
int i, j;
float amount;

	switch (gammalevel) {
		case 0:
			amount = 0.25;
			break;
		case 1:
			amount = 0.50;
			break;
		case 2:
			amount = 0.75;
			break;
		case 3:
			amount = 1.00;
			break;
		case 4:
			amount = 1.25;
			break;
		case 5:
			amount = 1.50;
			break;
		case 6:
			amount = 1.75;
			break;
		case 7:
			amount = 2.00;
			break;
		default:
			amount = 1.00;
			break;
	}

	for (i = 0; i < 256; i++)
	{
		j = (int) (((float) srcplt[i].r) * amount);
		if (j<0) j = 0; if (j>63) j = 63;
		gamma_pal[i].r = j;
		j = (int) (((float) srcplt[i].g) * amount);
		if (j<0) j = 0; if (j>63) j = 63;
		gamma_pal[i].g = j;
		j = (int) (((float) srcplt[i].b) * amount);
		if (j<0) j = 0; if (j>63) j = 63;
		gamma_pal[i].b = j;
	}

	set_palette(gamma_pal);
}

/* Simple functions to save and load highscores and preferences... */
void ReadHighScores (void)
{
FILE * fpt;
int verinfo, i;

	for (i=0;i<10;i++) memcpy (&highscores[i], &defhighs[i], sizeof (HIENTRY));
	fpt = fopen ("molehisc.bin", "rb");
	if (fpt != NULL) {
		fread (&verinfo, sizeof(int), 1, fpt);
		if (verinfo != 1) { fclose (fpt); return; }
		for (i=0;i<10;i++)
			if (fread (&highscores[i], sizeof (HIENTRY), 1, fpt) != 1)
				{ fclose (fpt); return; }
		fclose (fpt);
	}
}

void SaveHighScores (void)
{
FILE * fpt;
int verinfo, i;

	fpt = fopen ("molehisc.bin", "wb");
	if (fpt != NULL) {
		verinfo = 1;
		fwrite (&verinfo, sizeof(int), 1, fpt);
		for (i=0;i<10;i++)
			if (fwrite (&highscores[i], sizeof (HIENTRY), 1, fpt) != 1)
				{ fclose (fpt); return; }
		fclose (fpt);
	}
}

void ReadPrefs (void)
{
FILE * fpt;
int verinfo;

	gammalevel = 3; sfxvol = 255; modvol = 223; difficulty = 1; msespd = 2;

	fpt = fopen ("molepref.bin", "rb");
	if (fpt != NULL) {
		fread (&verinfo, sizeof(int), 1, fpt);
		if (verinfo > 2 || verinfo < 1) { fclose (fpt); return; }
		fread (&sfxvol, sizeof(int), 1, fpt);
		fread (&modvol, sizeof(int), 1, fpt);
		fread (&gammalevel, sizeof(int), 1, fpt);
		fread (&difficulty, sizeof(int), 1, fpt);
		if (verinfo > 1) fread (&msespd, sizeof(int), 1, fpt);
		fclose (fpt);
	}
}

void SavePrefs (void)
{
FILE * fpt;
int verinfo;

	fpt = fopen ("molepref.bin", "wb");
	if (fpt != NULL) {
		verinfo = 2;
		fwrite (&verinfo, sizeof(int), 1, fpt);
		fwrite (&sfxvol, sizeof(int), 1, fpt);
		fwrite (&modvol, sizeof(int), 1, fpt);
		fwrite (&gammalevel, sizeof(int), 1, fpt);
		fwrite (&difficulty, sizeof(int), 1, fpt);
		fwrite (&msespd, sizeof(int), 1, fpt);
		fclose (fpt);
	}
}

/* Strangely, the Allegro fade routines didn't work for me, so I wrote these */
void myfadeout (void)
{
int i;
int stillthere = 1;

	while (stillthere) {
		if (fpsdelay) {
			fpsdelay = 0;
			stillthere = 0;
			for (i=0;i<256;i++) {
				if (mypal[i].r>0) { mypal[i].r--; stillthere = 1; }
				if (mypal[i].g>0) { mypal[i].g--; stillthere = 1; }
				if (mypal[i].b>0) { mypal[i].b--; stillthere = 1; }
			}
			for (i=0;i<256;i++) {
				if (mypal[i].r>0) { mypal[i].r--; stillthere = 1; }
				if (mypal[i].g>0) { mypal[i].g--; stillthere = 1; }
				if (mypal[i].b>0) { mypal[i].b--; stillthere = 1; }
			}
			setmypal (mypal);
		}
	}
}

void myfadein (void)
{
int i;
int notthere = 1;
PALETTE temppal;

	memset (temppal, 0, sizeof(PALETTE));
	while (notthere) {
		if (fpsdelay) {
			fpsdelay = 0;
			notthere = 0;
			for (i=0;i<256;i++) {
				if (mypal[i].r>temppal[i].r) { temppal[i].r++; notthere = 1; }
				if (mypal[i].g>temppal[i].g) { temppal[i].g++; notthere = 1; }
				if (mypal[i].b>temppal[i].b) { temppal[i].b++; notthere = 1; }
			}
			for (i=0;i<256;i++) {
				if (mypal[i].r>temppal[i].r) { temppal[i].r++; notthere = 1; }
				if (mypal[i].g>temppal[i].g) { temppal[i].g++; notthere = 1; }
				if (mypal[i].b>temppal[i].b) { temppal[i].b++; notthere = 1; }
			}
			setmypal (temppal);
		}
	}
}

/* This is a bit messy, but you can see the result in the game */
void DrawHighScores (int pos)
{
int i, j;
char tempnum[256];

	for (j=0;j<2;j++) {
	acquire_bitmap(dbuffer);
	textout_centre_ex (dbuffer, mydata[TAHOMA].dat, "Magnificent Mole Manoeuvrers", SCRWIDTH/2, 10, 255, -1);
	textout_ex (dbuffer, mydata[TAHOMA].dat, "Rank        Name         Score  Level Difficulty",
		SCRWIDTH/2-230, SCRHEIGHT/2-180, 253, -1);
	release_bitmap(dbuffer);
	for (i=0;i<10;i++)
	{
		sprintf (tempnum, "%d", i+1);
		acquire_bitmap(dbuffer);
		textout_ex (dbuffer, mydata[TAHOMA].dat, tempnum, (SCRWIDTH/2)-220, (SCRHEIGHT/2)-140+(i*26), 63-(i*2), -1);
		textout_ex (dbuffer, mydata[TAHOMA].dat, highscores[i].name,
			(SCRWIDTH/2)-180, (SCRHEIGHT/2)-140+(i*26), 159-(i*2), -1);
		release_bitmap(dbuffer);
		sprintf (tempnum, "%d", highscores[i].hiscore);
		acquire_bitmap(dbuffer);
		textout_ex (dbuffer, mydata[TAHOMA].dat, tempnum, (SCRWIDTH/2)+60, (SCRHEIGHT/2)-140+(i*26), 31-(i*2), -1);
		release_bitmap(dbuffer);
		sprintf (tempnum, "%d", highscores[i].level);
		acquire_bitmap(dbuffer);
		textout_ex (dbuffer, mydata[TAHOMA].dat, tempnum, (SCRWIDTH/2)+140, (SCRHEIGHT/2)-140+(i*26), 95-(i*2), -1);
		switch (highscores[i].difficulty) {
			case 0:
			textout_ex (dbuffer, mydata[TAHOMA].dat, " EASY", (SCRWIDTH/2)+170, (SCRHEIGHT/2)-140+(i*26), 223-(i*2), -1);
			break;
			case 1:
			textout_ex (dbuffer, mydata[TAHOMA].dat, "MEDIUM", (SCRWIDTH/2)+170, (SCRHEIGHT/2)-140+(i*26), 223-(i*2), -1);
			break;
			default:
			textout_ex (dbuffer, mydata[TAHOMA].dat, " HARD", (SCRWIDTH/2)+170, (SCRHEIGHT/2)-140+(i*26), 223-(i*2), -1);
			break;
		}
		release_bitmap(dbuffer);
	}

	if (score && score<highscores[9].hiscore)
		{
			sprintf (tempnum, "Your score was %d, better luck next game", score);
			acquire_bitmap(dbuffer);
			textout_centre_ex (dbuffer, mydata[TAHOMA].dat, tempnum, SCRWIDTH/2, SCRHEIGHT/2+200, 254, -1);
			release_bitmap(dbuffer);
		}
	else if (score && (pos<10)) {
		acquire_bitmap(dbuffer);
		textout_centre_ex (dbuffer, mydata[TAHOMA].dat, "Wow, great score, enter your name in the top 10",
			SCRWIDTH/2, SCRHEIGHT/2+200, 254, -1);
		rect (dbuffer, SCRWIDTH/2-184, SCRHEIGHT/2-142+(pos*26), 
			SCRWIDTH/2+50, SCRHEIGHT/2-116+(pos*26), 253);
		release_bitmap(dbuffer);
		}
		if (vsyncon==1) vsync ();
#ifndef __WIN32__
		scroll_screen (0, scroffset);
		if (scroffset) dbuffer = display1; else dbuffer = display2;
#else
		WinFlip ();
#endif
		scroffset = SCRHEIGHT-scroffset;
		if (vsyncon==2) vsync ();
	}
}

void GoHighScore (void)
{
int i, j, pos;
volatile int timedelay;

	while (key[KEY_ESC]); clear_keybuf();
	for (i=0;i<32;i++) { mypal[i].r = (i*2); mypal[i].g = (i*2); mypal[i].b = 0; };
	for (i=0;i<32;i++) { mypal[i+32].r = (i*2); mypal[i+32].g = 0; mypal[i+32].b = 0; };
	for (i=0;i<32;i++) { mypal[i+64].r = 0; mypal[i+64].g = (i*2); mypal[i+64].b = 0; };
	for (i=0;i<32;i++) { mypal[i+96].r = 0; mypal[i+96].g = 0; mypal[i+96].b = (i*2); };
	for (i=0;i<32;i++) { mypal[i+128].r = (i*2); mypal[i+128].g = i; mypal[i+128].b = 0; };
	for (i=0;i<32;i++) { mypal[i+160].r = (i*2); mypal[i+160].g = (i*2); mypal[i+160].b = 0; };
	for (i=0;i<32;i++) { mypal[i+192].r = 0; mypal[i+192].g = i; mypal[i+192].b = (i*2); };
	mypal[253].r = 60, mypal[253].g = 60; mypal[253].b = 40;
	mypal[254].r = 20, mypal[254].g = 60; mypal[254].b = 20;
	mypal[255].r = 60, mypal[255].g = 60; mypal[255].b = 60;

	for (pos=0;pos<10;pos++) if (highscores[pos].hiscore<score) break;
	j = 9; while (j>pos) { highscores[j] = highscores[j-1]; j--; }
	if (pos<10) { highscores[pos].name[0] = 0; highscores[pos].hiscore = score;
		highscores[pos].level = level; highscores[pos].difficulty = difficulty; }

	DrawHighScores (pos);

#ifdef SOUNDON
	if (sfxvol && (pos<10)) {
		SEALPlaySnd (mydata[APPLAUSE].dat, sfxvol, 128, 800, TRUE);
		SEALPlaySnd (mydata[APPLAUSE].dat, sfxvol, 128, 1000, TRUE);
	}
#endif

	myfadein ();
	clear_keybuf();
	if (pos<10) i = 0; else i = 20; fpsdelay = 0; while (1) {
		if (fpsdelay) { timedelay = fpsdelay; fpsdelay = 0;
		if (vsyncon==1) vsync ();
#ifndef __WIN32__
		scroll_screen (0, scroffset);
		if (scroffset) dbuffer = display1; else dbuffer = display2;
#else
		WinFlip ();
#endif
		scroffset = SCRHEIGHT-scroffset;
		if (vsyncon==2) vsync ();
		if (i<18) {
			j = readkey();
			if ((j>>8) == KEY_ENTER) { i = 20;
				acquire_bitmap(display1);
				rect (display1, SCRWIDTH/2-184, SCRHEIGHT/2-142+(pos*26), 
					SCRWIDTH/2+50, SCRHEIGHT/2-116+(pos*26), 0);
				release_bitmap(display1);
				acquire_bitmap(display2);
				rect (display2, SCRWIDTH/2-184, SCRHEIGHT/2-142+(pos*26), 
					SCRWIDTH/2+50, SCRHEIGHT/2-116+(pos*26), 0);
				release_bitmap(display2);
			}
			if (((j>>8) == KEY_BACKSPACE) && (i>0)) { i--;
			highscores[pos].name[i] = 0;
			acquire_bitmap(display1);
			rectfill (display1, SCRWIDTH/2-183, SCRHEIGHT/2-141+(pos*26), 
				SCRWIDTH/2+49, SCRHEIGHT/2-117+(pos*26), 0);
			textout_ex (display1, mydata[TAHOMA].dat, highscores[pos].name,
				(SCRWIDTH/2)-180, (SCRHEIGHT/2)-140+(pos*26), 255, 0);
			release_bitmap(display1);
			acquire_bitmap(display2);
			rectfill (display2, SCRWIDTH/2-183, SCRHEIGHT/2-141+(pos*26), 
				SCRWIDTH/2+49, SCRHEIGHT/2-117+(pos*26), 0);
			textout_ex (display2, mydata[TAHOMA].dat, highscores[pos].name,
				(SCRWIDTH/2)-180, (SCRHEIGHT/2)-140+(pos*26), 255, 0);
			release_bitmap(display2);
			}
			if (((j>>8)==KEY_SPACE) || ((j&0xFF)>=' ')) {
			highscores[pos].name[i] = j; i++;
			highscores[pos].name[i] = 0;
			acquire_bitmap(display1);
			textout_ex (display1, mydata[TAHOMA].dat, highscores[pos].name,
				(SCRWIDTH/2)-180, (SCRHEIGHT/2)-140+(pos*26), 255, 0);
			release_bitmap(display1);
			acquire_bitmap(display2);
			textout_ex (display2, mydata[TAHOMA].dat, highscores[pos].name,
				(SCRWIDTH/2)-180, (SCRHEIGHT/2)-140+(pos*26), 255, 0);
			release_bitmap(display2);
			}
			while (key[KEY_ENTER]); while (key[KEY_SPACE]);
		}
	}
#ifdef __WIN32__
	if (redraw) { DrawHighScores (pos); redraw = 0; }
#endif
	if (key[KEY_ESC] || key[KEY_SPACE] || key[KEY_ENTER]) break;
	if (mouse_b & 1) break;
	}

#ifdef SOUNDON
//	stop_sample (mydata[APPLAUSE].dat);
#endif
	myfadeout ();
	acquire_bitmap(display1);
	clear (display1);
	release_bitmap(display1);
	acquire_bitmap(display2);
	clear (display2);
	release_bitmap(display2);
}

/* Assign random values to starfield */
STAR * InitStars (void)
{
int i;
STAR * thestars, * basestars;

	thestars = malloc (sizeof(STAR) * MAXSTARS);
	if (thestars == NULL) return NULL;

	basestars = thestars;
	for (i=0;i<MAXSTARS;i++)
	{
		thestars->xpos = (random ()%(SCRWIDTH*2))-(SCRWIDTH);
		thestars->ypos = (random ()%(SCRHEIGHT*2))-(SCRHEIGHT);
		thestars->zpos = ((random ()%(STARDEPTH-2))+2);
		thestars->colour = 64+(32*(i%4));
		thestars++;
	}
	return basestars;
}

/* Erase, move and draw stars */
void DoStars (STAR * mystars, int timegap)
{
int i, j, x, y;

	acquire_bitmap(dbuffer);
	for (i=0;i<NUMSTARS;i++)
	{
		putpixel (dbuffer, mystars->oldx2, mystars->oldy2, 0);

/* Calculate amount moved since last time */
		j = timegap; while (j)
		{
			mystars->zpos -= STARSPEED;
			if (mystars->zpos<=0) mystars->zpos += STARDEPTH;
			j--;
		}

		x = SCRWIDTH/2 + (STARDIST*mystars->xpos/mystars->zpos);
		y = SCRHEIGHT/2 + (STARDIST*mystars->ypos/mystars->zpos);
		putpixel (dbuffer, x, y,
			32-((int)(mystars->zpos/64.0))+mystars->colour);
		mystars->oldx2 = mystars->oldx;
		mystars->oldy2 = mystars->oldy;
		mystars->oldx = x;
		mystars->oldy = y;
		mystars++;
	}
	release_bitmap(dbuffer);
}

/* The attractor is the first bit people see, and spins the letters on */
int attract (void)
{
int i, j, timedelay;
LETTER letter[8];
OPTPANEL panel[4];
int framecounter;

	if (mydata==NULL) mydata = load_datafile ("data/moledata.dat");
	if (mydata==NULL) return -1;

	mystars = InitStars ();
	if (mystars==NULL) { unload_datafile (mydata); return -1; }

	memcpy (mypal, mydata[TFPAL].dat, sizeof(PALETTE));
	for (i=64;i<96;i++)
	{ mypal[i].r = (i-64)*2; mypal[i].g = (i-64)*2; mypal[i].b = (i-64)*2; }
	for (i=96;i<128;i++)
	{ mypal[i].r = (i-96)*2; mypal[i].g = 0; mypal[i].b = 0; }
	for (i=128;i<160;i++)
	{ mypal[i].r = 0; mypal[i].g = (i-128)*2; mypal[i].b = 0; }
	for (i=160;i<192;i++)
	{ mypal[i].r = 0; mypal[i].g = 0; mypal[i].b = (i-160)*2; }
	mypal[192].r = 63; mypal[192].g = 60; mypal[192].b = 10;
	mypal[193].r = 60; mypal[193].g = 10; mypal[193].b = 10;
	acquire_bitmap(display1);
	clear (display1);
	release_bitmap(display1);
	acquire_bitmap(display2);
	clear (display2);
	release_bitmap(display2);
	setmypal (mypal);
	show_mouse (NULL);

#ifdef MUSICON
	if (myxm==NULL && modvol) {
		myxm = load_mod ("data/k_pbox.xm");
		if (myxm!=NULL) play_mod (myxm, TRUE); 
		set_mod_volume (modvol);
	}
#endif
#ifdef USESEAL
	SEALStartMusic (0, modvol);
#endif

	for (i=0;i<8;i++) letter[i] = defletters[i];
	for (i=0;i<4;i++) panel[i].state = -1;
	fpsdelay = 0; fpstimer = 0; fpscounter = 0; fps = 0;
	framecounter = -100; clear_keybuf(); while (1)
	{
		if (fpsdelay) { timedelay = fpsdelay; fpsdelay = 0;
		if (vsyncon==1) vsync();
#ifndef __WIN32__
		scroll_screen (0, scroffset);
		if (scroffset) dbuffer = display1; else dbuffer = display2;
#else
		WinFlip ();
#endif
		scroffset = SCRHEIGHT-scroffset;
		if (vsyncon==2) vsync ();
		
		if (framecounter==0)  letter[0].letnumber = MLETTER;
		if (framecounter==20) letter[1].letnumber = OLETTER;
		if (framecounter==40) letter[2].letnumber = LLETTER;
		if (framecounter==60) letter[3].letnumber = ELETTER;
		if (framecounter==80) letter[4].letnumber = FLETTER;
		if (framecounter==100)
		{
			letter[5].letnumber = ELETTER2;
			letter[0].desty = 4;
		}
		if (framecounter==110) letter[1].desty = 32;
		if (framecounter==120)
		{
			letter[6].letnumber = SLETTER;
			letter[2].desty = 0;
		}
		if (framecounter==130) letter[3].desty = 32;
		if (framecounter==140)
		{
			letter[7].letnumber = TLETTER;
			letter[4].desty = 2;
		}
		if (framecounter==150) letter[5].desty = 32;
		if (framecounter==160) letter[6].desty = 32;
		if (framecounter==170) letter[7].desty = 14;
		if (framecounter==180)
		{
			panel[0].x = SCRWIDTH/2-100;
			panel[0].y = SCRHEIGHT/2-40;
			panel[0].state = 0;
			panel[0].colour = 170;
		}
		if (framecounter==190)
		{
			panel[1].x = SCRWIDTH/2-100;
			panel[1].y = SCRHEIGHT/2;
			panel[1].state = 0;
			panel[1].colour = 170;
		}
		if (framecounter==200)
		{
			panel[2].x = SCRWIDTH/2-100;
			panel[2].y = SCRHEIGHT/2+40;
			panel[2].state = 0;
			panel[2].colour = 170;
		}
		if (framecounter==210)
		{
			panel[3].x = SCRWIDTH/2-100;
			panel[3].y = SCRHEIGHT/2+100;
			panel[3].state = 0;
			panel[3].colour = 170;
		}

		for (i=0;i<8;i++)
		{
			acquire_bitmap(dbuffer);
			if (letter[i].letnumber!=-1)
			{
				rectfill (dbuffer, letter[i].oldx, letter[i].oldy,
					letter[i].oldx + ((RLE_SPRITE *)mydata[letter[i].letnumber].dat)->w + 60,
					letter[i].oldy + ((RLE_SPRITE *)mydata[letter[i].letnumber].dat)->h + 60, 0);
				letter[i].oldx = letter[i].x-30;
				letter[i].oldy = letter[i].y-30;
			}
			release_bitmap(dbuffer);
		}
		for (i=0;i<4;i++)
		{
			acquire_bitmap(dbuffer);
			if (panel[i].state>-1)
			{
				rectfill (dbuffer, panel[i].x, panel[i].y-16, panel[i].x+199, panel[i].y+31, 0);
				j = timedelay; while (j) { if (flipnumbers[panel[i].state+1]!=-1) panel[i].state++; j--; }
			}
			release_bitmap(dbuffer);
		}
		DoStars (mystars, timedelay);
		if (key[KEY_ESC] || mouse_b&1 || keypressed()) {
/* If people are impatient, they can interrupt the attractor and go to the main menu */
		for (i=0;i<8;i++)
		{
			if (letter[i].letnumber!=-1)
			{
				if (scroffset) {
				acquire_bitmap(display1);
				rectfill (display1, letter[i].oldx, letter[i].oldy,
					letter[i].x + ((RLE_SPRITE *)mydata[letter[i].letnumber].dat)->w + 60,
					letter[i].y + ((RLE_SPRITE *)mydata[letter[i].letnumber].dat)->h + 60, 0);
				release_bitmap(display1);
				} else {
				acquire_bitmap(display2);
				rectfill (display2, letter[i].oldx, letter[i].oldy,
					letter[i].x + ((RLE_SPRITE *)mydata[letter[i].letnumber].dat)->w + 60,
					letter[i].y + ((RLE_SPRITE *)mydata[letter[i].letnumber].dat)->h + 60, 0);
				release_bitmap(display2);
				}
			}
//			release_bitmap(dbuffer);
		} while (key[KEY_ESC]); while (mouse_b&1); break; }
		for (i=0;i<4;i++)
		{
			acquire_bitmap(dbuffer);
			if (panel[i].state>-1)
			{
				rectfill (dbuffer, panel[i].x, panel[i].y-flipnumbers[panel[i].state],
					panel[i].x+199, panel[i].y+flipnumbers[panel[i].state],
					panel[i].colour+flipnumbers[panel[i].state]);
			}
			release_bitmap(dbuffer);
		}
		for (i=0;i<8;i++)
		{
			acquire_bitmap(dbuffer);
			if (letter[i].letnumber!=-1)
			{
				j = timedelay; while (j) {
				letter[i].x -= (letter[i].x - letter[i].destx)/8;
				letter[i].y -= (letter[i].y - letter[i].desty)/8;
				if (letter[i].angle != 0)
				{ if (letter[i].angle >= 8) letter[i].angle -= (letter[i].angle)/8;
					else letter[i].angle--; }
				j--; }
				if (letter[i].angle) rotate_sprite (dbuffer, mydata[letter[i].letnumber].dat,
					letter[i].x, letter[i].y, (letter[i].angle)<<16);
				else draw_sprite (dbuffer, mydata[letter[i].letnumber].dat,
					letter[i].x, letter[i].y);
			}
			release_bitmap(dbuffer);
		}
		acquire_bitmap(dbuffer);
		if (framecounter>200) {
			textout_ex (dbuffer, mydata[TAHOMA].dat, "By", SCRWIDTH-180, 110, 192, -1);
			textout_ex (dbuffer, mydata[TAHOMA].dat, "Robin Burrows", SCRWIDTH-150, 110, 193, -1); }
		if (framecounter>220)
			textout_ex (dbuffer, font, "Written with DJGPP", SCRWIDTH/2-(24*8), SCRHEIGHT-20, 90, -1);
		if (framecounter>240)
			textout_ex (dbuffer, font, "http://www.delorie.com/djgpp/", SCRWIDTH/2-(24*8)+(21*8), SCRHEIGHT-20, 156, -1);
		if (framecounter>260)
			textout_ex (dbuffer, font, "and Allegro 4.4", SCRWIDTH/2-(27*8), SCRHEIGHT-10, 90, -1);
		if (framecounter>280)
			textout_ex (dbuffer, font, "http://alleg.sourceforge.net", SCRWIDTH/2-(27*8)+(20*8), SCRHEIGHT-10, 156, -1);
		release_bitmap(dbuffer);
#ifdef __WIN32__
		if (redraw) {
			acquire_bitmap(display1);
			clear (display1);
			release_bitmap(display1);
			acquire_bitmap(display2);
			clear (display2);
			release_bitmap(display2);
			redraw = 0;
		}
#endif
		framecounter++; }
		if (panel[3].state>-1) if (flipnumbers[panel[3].state+1]==-1) break;
	}
#ifndef __WIN32__
	position_mouse (SCRWIDTH/2+120, SCRHEIGHT/2+60);
#endif
	return 0;
}

/* mainmenu follows the attractor and just wait 'til an option is selected */
int mainmenu (void)
{
int i, timedelay, option, oldoption, mymousex, mymousey, oldx, oldy, oldx2, oldy2, glowcount, menucounter;
int keyu, keyd, keyF4;
int keyt[5]; 

	keyt[1] = SCRHEIGHT/2-40;
	keyt[2] = SCRHEIGHT/2;
	keyt[3] = SCRHEIGHT/2+42;
	keyt[4] = SCRHEIGHT/2+102;
	keyu = 0; keyd = 0; keyF4 = 0;

	fpsdelay = 0; fpstimer = 0; fpscounter = 0; fps = 0;
	mymousex = mouse_x; mymousey = mouse_y;
	oldx = mymousex; oldy = mymousey; option = 0; oldoption = 0;
	oldx2 = oldx; oldy2 = oldy; glowcount = 0; menucounter = 0;
	while (mouse_b & 1); while (1)
	{
		if (fpsdelay) { timedelay = fpsdelay; fpsdelay = 0;
		if (vsyncon==1) vsync();
#ifndef __WIN32__
		scroll_screen (0, scroffset);
		if (scroffset) dbuffer = display1; else dbuffer = display2;
#else
		WinFlip ();
#endif
		scroffset = SCRHEIGHT-scroffset;
		if (vsyncon==2) vsync ();

		oldx2 = oldx; oldy2 = oldy;
		oldx = mymousex; oldy = mymousey;
		mymousex = mouse_x; mymousey = mouse_y;
		acquire_bitmap(dbuffer);
		rectfill (dbuffer, oldx2, oldy2, oldx2+15, oldy2+21, 0);
		release_bitmap(dbuffer);
		DoStars (mystars, timedelay);
		acquire_bitmap(dbuffer);
		draw_sprite (dbuffer, mydata[MLETTER].dat, 62, 4);
		draw_sprite (dbuffer, mydata[OLETTER].dat, 183, 32);
		draw_sprite (dbuffer, mydata[LLETTER].dat, 244, 0);
		draw_sprite (dbuffer, mydata[ELETTER].dat, 285, 32);
		draw_sprite (dbuffer, mydata[FLETTER].dat, 348, 2);
		draw_sprite (dbuffer, mydata[ELETTER2].dat, 397, 32);
		draw_sprite (dbuffer, mydata[SLETTER].dat, 462, 32);
		draw_sprite (dbuffer, mydata[TLETTER].dat, 517, 14);
		release_bitmap(dbuffer);
		oldoption = option; option = 0; if (mymousex>((SCRWIDTH/2)-100) && mymousex<((SCRWIDTH/2)+100))
		{
			if (mymousey>((SCRHEIGHT/2)-56) && mymousey<((SCRHEIGHT/2)-24)) option = 1;
			if (mymousey>((SCRHEIGHT/2)-16) && mymousey<((SCRHEIGHT/2)+16)) option = 2;
			if (mymousey>((SCRHEIGHT/2)+24) && mymousey<((SCRHEIGHT/2)+56)) option = 3;
			if (mymousey>((SCRHEIGHT/2)+84) && mymousey<((SCRHEIGHT/2)+116)) option = 4;
#ifdef SOUNDON
		if (sfxvol)
			if (option != oldoption && option)
				SEALPlaySnd (mydata[CLICK2].dat, sfxvol, 128, 1000, FALSE);
#endif
		}

		if (option==0 && key[KEY_UP] && !keyu) { position_mouse (SCRWIDTH/2+80, SCRHEIGHT/2-40); keyu = 1; }
		if (option==0 && key[KEY_DOWN] && !keyd) { position_mouse (SCRWIDTH/2+80, SCRHEIGHT/2-40); keyd = 1; }
		if (option!=0 && key[KEY_UP] && !keyu)
		{
			option--; if (option<1) option = 4;
			position_mouse (SCRWIDTH/2+80, keyt[option]);
			keyu = 1;
#ifdef SOUNDON
	if (sfxvol)
			SEALPlaySnd (mydata[CLICK2].dat, sfxvol, 128, 1000, FALSE);
#endif
		}
		if (option!=0 && key[KEY_DOWN] && !keyd)
		{
			option++; if (option>4) option = 1;
			position_mouse (SCRWIDTH/2+80, keyt[option]);
			keyd = 1;
#ifdef SOUNDON
	if (sfxvol)
			SEALPlaySnd (mydata[CLICK2].dat, sfxvol, 128, 1000, FALSE);
#endif
		}
		if (key[KEY_F4] && !keyF4)
		{
			vsyncon++; if (vsyncon==3) vsyncon = 0;
			keyF4 = 1;
#ifdef SOUNDON
	if (sfxvol)
			SEALPlaySnd (mydata[CHING1].dat, sfxvol, 128, 1000, FALSE);
#endif
		}
		if (!key[KEY_UP]) keyu = 0;
		if (!key[KEY_DOWN]) keyd = 0;
		if (!key[KEY_F4]) keyF4 = 0;
		acquire_bitmap(dbuffer);
		if (option==1) for (i=0;i<33;i++) hline (dbuffer, SCRWIDTH/2-100, SCRHEIGHT/2-56+i, SCRWIDTH/2+99, 143+(i/2));
		else for (i=0;i<33;i++) hline (dbuffer, SCRWIDTH/2-100, SCRHEIGHT/2-56+i, SCRWIDTH/2+99, 175+(i/2));
		if (option==2) for (i=0;i<33;i++) hline (dbuffer, SCRWIDTH/2-100, SCRHEIGHT/2-16+i, SCRWIDTH/2+99, 143+(i/2));
		else for (i=0;i<33;i++) hline (dbuffer, SCRWIDTH/2-100, SCRHEIGHT/2-16+i, SCRWIDTH/2+99, 175+(i/2));
		if (option==3) for (i=0;i<33;i++) hline (dbuffer, SCRWIDTH/2-100, SCRHEIGHT/2+24+i, SCRWIDTH/2+99, 143+(i/2));
		else for (i=0;i<33;i++) hline (dbuffer, SCRWIDTH/2-100, SCRHEIGHT/2+24+i, SCRWIDTH/2+99, 175+(i/2));
		if (option==4) for (i=0;i<33;i++) hline (dbuffer, SCRWIDTH/2-100, SCRHEIGHT/2+84+i, SCRWIDTH/2+99, 143+(i/2));
		else for (i=0;i<33;i++) hline (dbuffer, SCRWIDTH/2-100, SCRHEIGHT/2+84+i, SCRWIDTH/2+99, 175+(i/2));

		glowcount++; if (glownumbers[glowcount]==-1) glowcount = 0;
		textout_ex (dbuffer, mydata[TAHOMA].dat, "Start game", SCRWIDTH/2-46, SCRHEIGHT/2-50, 0, -1);
		textout_ex (dbuffer, mydata[TAHOMA].dat, "Options", SCRWIDTH/2-32, SCRHEIGHT/2-10, 0, -1);
		textout_ex (dbuffer, mydata[TAHOMA].dat, "High scores", SCRWIDTH/2-46, SCRHEIGHT/2+30, 0, -1);
		textout_ex (dbuffer, mydata[TAHOMA].dat, "Exit", SCRWIDTH/2-12, SCRHEIGHT/2+90, 0, -1);
		if (option==1) textout_ex (dbuffer, mydata[TAHOMA].dat, "Start game", SCRWIDTH/2-48, SCRHEIGHT/2-52, 84+glownumbers[glowcount], -1);
		else textout_ex (dbuffer, mydata[TAHOMA].dat, "Start game", SCRWIDTH/2-48, SCRHEIGHT/2-52, 82, -1);
		if (option==2) textout_ex (dbuffer, mydata[TAHOMA].dat, "Options", SCRWIDTH/2-34, SCRHEIGHT/2-12, 84+glownumbers[glowcount], -1);
		else textout_ex (dbuffer, mydata[TAHOMA].dat, "Options", SCRWIDTH/2-34, SCRHEIGHT/2-12, 82, -1);
		if (option==3) textout_ex (dbuffer, mydata[TAHOMA].dat, "High scores", SCRWIDTH/2-48, SCRHEIGHT/2+28, 84+glownumbers[glowcount], -1);
		else textout_ex (dbuffer, mydata[TAHOMA].dat, "High scores", SCRWIDTH/2-48, SCRHEIGHT/2+28, 82, -1);
		if (option==4) textout_ex (dbuffer, mydata[TAHOMA].dat, "Exit", SCRWIDTH/2-14, SCRHEIGHT/2+88, 84+glownumbers[glowcount], -1);
		else textout_ex (dbuffer, mydata[TAHOMA].dat, "Exit", SCRWIDTH/2-14, SCRHEIGHT/2+88, 82, -1);
		textout_ex (dbuffer, mydata[TAHOMA].dat, "By", SCRWIDTH-180, 110, 192, -1);
		textout_ex (dbuffer, mydata[TAHOMA].dat, "Robin Burrows", SCRWIDTH-150, 110, 193, -1);
		textout_ex (dbuffer, font, "Written with DJGPP", SCRWIDTH/2-(24*8), SCRHEIGHT-20, 90, -1);
		textout_ex (dbuffer, font, "http://www.delorie.com/djgpp/", SCRWIDTH/2-(24*8)+(21*8), SCRHEIGHT-20, 156, -1);
		textout_ex (dbuffer, font, "and Allegro 4.4", SCRWIDTH/2-(27*8), SCRHEIGHT-10, 90, -1);
		textout_ex (dbuffer, font, "http://alleg.sourceforge.net", SCRWIDTH/2-(27*8)+(20*8), SCRHEIGHT-10, 156, -1);
		draw_rle_sprite (dbuffer, mydata[MENUMOUSE].dat, mymousex, mymousey);
		release_bitmap(dbuffer);
#ifdef __WIN32__
		if (redraw) {
			acquire_bitmap(display1);
			clear (display1);
			release_bitmap(display1);
			acquire_bitmap(display2);
			clear (display2);
			release_bitmap(display2);
			redraw = 0;
		}
#endif
		menucounter+=timedelay;
		if (mymousex != oldx2 || mymousey != oldy2) menucounter = 0;
		}
		if (key[KEY_ESC]) { option = 4; break; }
		if ((key[KEY_ENTER] || key[KEY_SPACE]) && option) {
#ifdef SOUNDON
		if (sfxvol)
			SEALPlaySnd (mydata[CLICK1].dat, sfxvol, 128, 1000, FALSE);
#endif
		break; }
		if ((mouse_b & 1) && option) {
#ifdef SOUNDON
		if (sfxvol)
			SEALPlaySnd (mydata[CLICK1].dat, sfxvol, 128, 1000, FALSE);
#endif
		break; }
		if (menucounter>3600) { option = 0; break; }
	}
#ifdef MUSICON
	if (myxm!=NULL && option != 0) { stop_mod (); destroy_mod (myxm); myxm = NULL; }
#endif
#ifdef USESEAL
	if (option != 0) SEALStopMusic ();
#endif
	return option;
}

/* Change brightness, sound, music volume, and game difficulty */
int ChangeOptions (void)
{
int i, timedelay, option, oldoption, mymousex, mymousey, oldx, oldy, oldx2, oldy2, glowcount, menucounter;
int keyu, keyd, keyl, keyr, mousefire;
int keyt[7]; 

	keyt[1] = SCRHEIGHT/2-70;
	keyt[2] = SCRHEIGHT/2-30;
	keyt[3] = SCRHEIGHT/2+10;
	keyt[4] = SCRHEIGHT/2+50;
	keyt[5] = SCRHEIGHT/2+110;
	keyt[6] = SCRHEIGHT/2-110;
	keyu = 0; keyd = 0; keyl = 0; keyr = 0; mousefire = 0;

#ifdef MUSICON
	if (myxm==NULL) {
		myxm = load_mod ("data/lurid.xm");
		if (myxm!=NULL) play_mod (myxm, TRUE); 
		set_mod_volume (modvol);
	}
#endif
#ifdef USESEAL
	SEALStartMusic (1, modvol);
#endif

	memcpy (mypal, mydata[TFPAL].dat, sizeof(PALETTE));
	for (i=64;i<96;i++)
	{ mypal[i].r = (i-64)*2; mypal[i].g = (i-64)*2; mypal[i].b = (i-64)*2; }
	for (i=96;i<128;i++)
	{ mypal[i].r = (i-96)*2; mypal[i].g = 0; mypal[i].b = 0; }
	for (i=128;i<160;i++)
	{ mypal[i].r = 0; mypal[i].g = (i-128)*2; mypal[i].b = 0; }
	for (i=160;i<192;i++)
	{ mypal[i].r = 0; mypal[i].g = 0; mypal[i].b = (i-160)*2; }
	mypal[192].r = 63; mypal[192].g = 60; mypal[192].b = 10;
	mypal[193].r = 60; mypal[193].g = 10; mypal[193].b = 10;
	setmypal (mypal);

	fpsdelay = 0; fpstimer = 0; fpscounter = 0; fps = 0;
	mymousex = mouse_x; mymousey = mouse_y;
	oldx = mymousex; oldy = mymousey; option = 0; oldoption = 0;
	oldx2 = oldx; oldy2 = oldy; glowcount = 0; menucounter = 0;
	while (mouse_b & 1); while (1)
	{
		if (fpsdelay) { timedelay = fpsdelay; fpsdelay = 0;
		if (vsyncon==1) vsync ();
#ifndef __WIN32__
		scroll_screen (0, scroffset);
		if (scroffset) dbuffer = display1; else dbuffer = display2;
#else
		WinFlip ();
#endif
		scroffset = SCRHEIGHT-scroffset;
		if (vsyncon==2) vsync ();

		oldx2 = oldx; oldy2 = oldy;
		oldx = mymousex; oldy = mymousey;
		mymousex = mouse_x; mymousey = mouse_y;
		acquire_bitmap(dbuffer);
		rectfill (dbuffer, oldx2, oldy2, oldx2+15, oldy2+21, 0);
		release_bitmap(dbuffer);
		DoStars (mystars, timedelay);
		oldoption = option; option = 0; if (mymousex>((SCRWIDTH/2)-100) && mymousex<((SCRWIDTH/2)+100))
		{
			if (mymousey>((SCRHEIGHT/2)-116) && mymousey<((SCRHEIGHT/2)-84)) option = 6;
			if (mymousey>((SCRHEIGHT/2)-76) && mymousey<((SCRHEIGHT/2)-44)) option = 1;
			if (mymousey>((SCRHEIGHT/2)-36) && mymousey<((SCRHEIGHT/2)-4)) option = 2;
			if (mymousey>((SCRHEIGHT/2)+4) && mymousey<((SCRHEIGHT/2)+36)) option = 3;
			if (mymousey>((SCRHEIGHT/2)+44) && mymousey<((SCRHEIGHT/2)+76)) option = 4;
			if (mymousey>((SCRHEIGHT/2)+104) && mymousey<((SCRHEIGHT/2)+146)) option = 5;
#ifdef SOUNDON
		if (sfxvol)
			if (option != oldoption && option)
				SEALPlaySnd (mydata[CLICK2].dat, sfxvol, 128, 1000, FALSE);
#endif
		}

		if (option==0 && key[KEY_UP] && !keyu) { position_mouse (SCRWIDTH/2+80, SCRHEIGHT/2-60); keyu = 1; }
		if (option==0 && key[KEY_DOWN] && !keyd) { position_mouse (SCRWIDTH/2+80, SCRHEIGHT/2-60); keyd = 1; }
		if (option!=0 && key[KEY_UP] && !keyu)
		{
			option--; if (option<1) option = 6;
			position_mouse (SCRWIDTH/2+80, keyt[option]);
			keyu = 1;
#ifdef SOUNDON
	if (sfxvol)
			SEALPlaySnd (mydata[CLICK2].dat, sfxvol, 128, 1000, FALSE);
#endif
		}
		if (option!=0 && key[KEY_DOWN] && !keyd)
		{
			option++; if (option>6) option = 1;
			position_mouse (SCRWIDTH/2+80, keyt[option]);
			keyd = 1;
#ifdef SOUNDON
	if (sfxvol)
			SEALPlaySnd (mydata[CLICK2].dat, sfxvol, 128, 1000, FALSE);
#endif
		}

		if (option==6 && (mouse_b & 1) && !mousefire)
		{
			msespd++; if (msespd == 6) msespd = 0; set_mouse_speed(msespd, msespd);
			mousefire = 1;
#ifdef SOUNDON
	if (sfxvol)
			SEALPlaySnd (mydata[CLICK2].dat, sfxvol, 128, 1000, FALSE);
#endif
		}

		if (option==6 && key[KEY_RIGHT] && !keyr)
		{
			msespd++; if (msespd == 6) msespd = 0; set_mouse_speed(msespd, msespd);
			keyr = 1;
#ifdef SOUNDON
	if (sfxvol)
			SEALPlaySnd (mydata[CLICK2].dat, sfxvol, 128, 1000, FALSE);
#endif
		}

		if (option==6 && key[KEY_LEFT] && !keyl)
		{
			msespd--; if (msespd == -1) msespd = 5; set_mouse_speed(msespd, msespd);
			keyl = 1;
#ifdef SOUNDON
	if (sfxvol)
			SEALPlaySnd (mydata[CLICK2].dat, sfxvol, 128, 1000, FALSE);
#endif
		}

		if (option==1 && (mouse_b & 1) && !mousefire)
		{
			gammalevel++; if (gammalevel == 8) gammalevel = 0; setmypal(mypal);
			mousefire = 1;
#ifdef SOUNDON
	if (sfxvol)
			SEALPlaySnd (mydata[CLICK2].dat, sfxvol, 128, 1000, FALSE);
#endif
		}

		if (option==1 && key[KEY_RIGHT] && !keyr)
		{
			gammalevel++; if (gammalevel == 8) gammalevel = 0; setmypal(mypal);
			keyr = 1;
#ifdef SOUNDON
	if (sfxvol)
			SEALPlaySnd (mydata[CLICK2].dat, sfxvol, 128, 1000, FALSE);
#endif
		}

		if (option==1 && key[KEY_LEFT] && !keyl)
		{
			gammalevel--; if (gammalevel <0) gammalevel = 7; setmypal(mypal);
			keyl = 1;
#ifdef SOUNDON
	if (sfxvol)
			SEALPlaySnd (mydata[CLICK2].dat, sfxvol, 128, 1000, FALSE);
#endif
		}

		if (option==2 && key[KEY_RIGHT] && !keyr)
		{
			if (sfxvol) sfxvol += 32; else sfxvol = 31;
			if (sfxvol>255) sfxvol = 0;
			keyr = 1;
#ifdef SOUNDON
	if (sfxvol)
			SEALPlaySnd (mydata[CLICK2].dat, sfxvol, 128, 1000, FALSE);
#endif
		}
		if (option==2 && key[KEY_LEFT] && !keyl)
		{
			if (sfxvol == 31) sfxvol = 0; else sfxvol -= 32;
			if (sfxvol<0) sfxvol = 255;
			keyl = 1;
#ifdef SOUNDON
	if (sfxvol)
			SEALPlaySnd (mydata[CLICK2].dat, sfxvol, 128, 1000, FALSE);
#endif
		}
		if (option==2 && (mouse_b & 1) && !mousefire)
		{
			if (sfxvol) sfxvol += 32; else sfxvol = 31;
			if (sfxvol>255) sfxvol = 0;
			mousefire = 1;
#ifdef SOUNDON
	if (sfxvol)
			SEALPlaySnd (mydata[CLICK2].dat, sfxvol, 128, 1000, FALSE);
#endif
		}
		if (option==3 && key[KEY_RIGHT] && !keyr)
		{
			if (modvol) modvol += 32; else modvol = 31;
			if (modvol>255) modvol = 0;
#ifdef MUSICON
			set_mod_volume (modvol);
#endif
#ifdef USESEAL
			SEALResumeMod (modvol);
#endif
			keyr = 1;
#ifdef SOUNDON
	if (sfxvol)
			SEALPlaySnd (mydata[CLICK2].dat, sfxvol, 128, 1000, FALSE);
#endif
		}
		if (option==3 && key[KEY_LEFT] && !keyl)
		{
			if (modvol == 31) modvol = 0; else modvol -= 32;
			if (modvol<0) modvol = 255;
#ifdef MUSICON
			set_mod_volume (modvol);
#endif
#ifdef USESEAL
			SEALResumeMod (modvol);
#endif
			keyl = 1;
#ifdef SOUNDON
	if (sfxvol)
			SEALPlaySnd (mydata[CLICK2].dat, sfxvol, 128, 1000, FALSE);
#endif
		}
		if (option==3 && (mouse_b & 1) && !mousefire)
		{
			if (modvol) modvol += 32; else modvol = 31;
			if (modvol>255) modvol = 0;
#ifdef MUSICON
			set_mod_volume (modvol);
#endif
#ifdef USESEAL
			SEALResumeMod (modvol);
#endif
			mousefire = 1;
#ifdef SOUNDON
	if (sfxvol)
			SEALPlaySnd (mydata[CLICK2].dat, sfxvol, 128, 1000, FALSE);
#endif
		}
		if (option==4 && key[KEY_RIGHT] && !keyr)
		{
			difficulty++; if (difficulty==3) difficulty = 0;
			keyr = 1;
#ifdef SOUNDON
	if (sfxvol)
			SEALPlaySnd (mydata[CLICK2].dat, sfxvol, 128, 1000, FALSE);
#endif
		}
		if (option==4 && (mouse_b & 1) && !mousefire)
		{
			difficulty++; if (difficulty==3) difficulty = 0;
			mousefire = 1;
#ifdef SOUNDON
	if (sfxvol)
			SEALPlaySnd (mydata[CLICK2].dat, sfxvol, 128, 1000, FALSE);
#endif
		}
		if (option==4 && key[KEY_LEFT] && !keyl)
		{
			difficulty--; if (difficulty<0) difficulty = 2;
			keyl = 1;
#ifdef SOUNDON
	if (sfxvol)
			SEALPlaySnd (mydata[CLICK2].dat, sfxvol, 128, 1000, FALSE);
#endif
		}

		if (!key[KEY_UP]) keyu = 0;
		if (!key[KEY_DOWN]) keyd = 0;
		if (!key[KEY_LEFT]) keyl = 0;
		if (!key[KEY_RIGHT]) keyr = 0;
		if (!(mouse_b & 1)) mousefire = 0;
		acquire_bitmap(dbuffer);
		if (option==6) for (i=0;i<33;i++) hline (dbuffer, SCRWIDTH/2-100, SCRHEIGHT/2-116+i, SCRWIDTH/2+99, 143+(i/2));
		else for (i=0;i<33;i++) hline (dbuffer, SCRWIDTH/2-100, SCRHEIGHT/2-116+i, SCRWIDTH/2+99, 175+(i/2));
		if (option==1) for (i=0;i<33;i++) hline (dbuffer, SCRWIDTH/2-100, SCRHEIGHT/2-76+i, SCRWIDTH/2+99, 143+(i/2));
		else for (i=0;i<33;i++) hline (dbuffer, SCRWIDTH/2-100, SCRHEIGHT/2-76+i, SCRWIDTH/2+99, 175+(i/2));
		if (option==2) for (i=0;i<33;i++) hline (dbuffer, SCRWIDTH/2-100, SCRHEIGHT/2-36+i, SCRWIDTH/2+99, 143+(i/2));
		else for (i=0;i<33;i++) hline (dbuffer, SCRWIDTH/2-100, SCRHEIGHT/2-36+i, SCRWIDTH/2+99, 175+(i/2));
		if (option==3) for (i=0;i<33;i++) hline (dbuffer, SCRWIDTH/2-100, SCRHEIGHT/2+4+i, SCRWIDTH/2+99, 143+(i/2));
		else for (i=0;i<33;i++) hline (dbuffer, SCRWIDTH/2-100, SCRHEIGHT/2+4+i, SCRWIDTH/2+99, 175+(i/2));
		if (option==4) for (i=0;i<33;i++) hline (dbuffer, SCRWIDTH/2-100, SCRHEIGHT/2+44+i, SCRWIDTH/2+99, 143+(i/2));
		else for (i=0;i<33;i++) hline (dbuffer, SCRWIDTH/2-100, SCRHEIGHT/2+44+i, SCRWIDTH/2+99, 175+(i/2));
		if (option==5) for (i=0;i<33;i++) hline (dbuffer, SCRWIDTH/2-100, SCRHEIGHT/2+104+i, SCRWIDTH/2+99, 143+(i/2));
		else for (i=0;i<33;i++) hline (dbuffer, SCRWIDTH/2-100, SCRHEIGHT/2+104+i, SCRWIDTH/2+99, 175+(i/2));

		glowcount++; if (glownumbers[glowcount]==-1) glowcount = 0;
		textout_ex (dbuffer, mydata[TAHOMA].dat, "Mouse Speed", SCRWIDTH/2-90, SCRHEIGHT/2-110, 0, -1);
		textout_ex (dbuffer, mydata[TAHOMA].dat, "Brightness", SCRWIDTH/2-90, SCRHEIGHT/2-70, 0, -1);
		textout_ex (dbuffer, mydata[TAHOMA].dat, "Effects Vol", SCRWIDTH/2-90, SCRHEIGHT/2-30, 0, -1);
		textout_ex (dbuffer, mydata[TAHOMA].dat, "Music Vol", SCRWIDTH/2-90, SCRHEIGHT/2+10, 0, -1);
		textout_ex (dbuffer, mydata[TAHOMA].dat, "Difficulty", SCRWIDTH/2-90, SCRHEIGHT/2+50, 0, -1);
		textout_ex (dbuffer, mydata[TAHOMA].dat, "Exit", SCRWIDTH/2-12, SCRHEIGHT/2+110, 0, -1);
		if (option==6) textout_ex (dbuffer, mydata[TAHOMA].dat, "Mouse Speed", SCRWIDTH/2-92, SCRHEIGHT/2-112, 84+glownumbers[glowcount], -1);
		else textout_ex (dbuffer, mydata[TAHOMA].dat, "Mouse Speed", SCRWIDTH/2-92, SCRHEIGHT/2-112, 82, -1);
		if (option==1) textout_ex (dbuffer, mydata[TAHOMA].dat, "Brightness", SCRWIDTH/2-92, SCRHEIGHT/2-72, 84+glownumbers[glowcount], -1);
		else textout_ex (dbuffer, mydata[TAHOMA].dat, "Brightness", SCRWIDTH/2-92, SCRHEIGHT/2-72, 82, -1);
		if (option==2) textout_ex (dbuffer, mydata[TAHOMA].dat, "Effects Vol", SCRWIDTH/2-92, SCRHEIGHT/2-32, 84+glownumbers[glowcount], -1);
		else textout_ex (dbuffer, mydata[TAHOMA].dat, "Effects Vol", SCRWIDTH/2-92, SCRHEIGHT/2-32, 82, -1);
		if (option==3) textout_ex (dbuffer, mydata[TAHOMA].dat, "Music Vol", SCRWIDTH/2-92, SCRHEIGHT/2+8, 84+glownumbers[glowcount], -1);
		else textout_ex (dbuffer, mydata[TAHOMA].dat, "Music Vol", SCRWIDTH/2-92, SCRHEIGHT/2+8, 82, -1);
		if (option==4) textout_ex (dbuffer, mydata[TAHOMA].dat, "Difficulty", SCRWIDTH/2-92, SCRHEIGHT/2+48, 84+glownumbers[glowcount], -1);
		else textout_ex (dbuffer, mydata[TAHOMA].dat, "Difficulty", SCRWIDTH/2-92, SCRHEIGHT/2+48, 82, -1);
		if (option==5) textout_ex (dbuffer, mydata[TAHOMA].dat, "Exit", SCRWIDTH/2-14, SCRHEIGHT/2+108, 84+glownumbers[glowcount], -1);
		else textout_ex (dbuffer, mydata[TAHOMA].dat, "Exit", SCRWIDTH/2-14, SCRHEIGHT/2+108, 82, -1);
		if (fliptype == 0) textout_ex (dbuffer, mydata[TAHOMA].dat, "pageflip mode (default)", SCRWIDTH/2-92, SCRHEIGHT/2+148, 82, -1);
		if (fliptype == 3) textout_ex (dbuffer, mydata[TAHOMA].dat, "double buffer (safe)", SCRWIDTH/2-92, SCRHEIGHT/2+148, 82, -1);

		for (i=0;i<msespd;i++) rectfill (dbuffer, SCRWIDTH/2+(i*10)+42, SCRHEIGHT/2-110,
			SCRWIDTH/2+(i*10)+49, SCRHEIGHT/2-90, 192);
		for (i=0;i<gammalevel;i++) rectfill (dbuffer, SCRWIDTH/2+(i*10)+12, SCRHEIGHT/2-70,
			SCRWIDTH/2+(i*10)+19, SCRHEIGHT/2-50, 192);
		for (i=0;i<((sfxvol+1)/32);i++) rectfill (dbuffer, SCRWIDTH/2+(i*10)+12, SCRHEIGHT/2-30,
			SCRWIDTH/2+(i*10)+19, SCRHEIGHT/2-10, 192);
		for (i=0;i<((modvol+1)/32);i++) rectfill (dbuffer, SCRWIDTH/2+(i*10)+12, SCRHEIGHT/2+10,
			SCRWIDTH/2+(i*10)+19, SCRHEIGHT/2+30, 192);

		if (difficulty == 0) {
			textout_ex (dbuffer, mydata[TAHOMA].dat, "easy", SCRWIDTH/2+14, SCRHEIGHT/2+50, 0, -1);
			textout_ex (dbuffer, mydata[TAHOMA].dat, "easy", SCRWIDTH/2+12, SCRHEIGHT/2+48, 192, -1);
		} else if (difficulty == 1) {
			textout_ex (dbuffer, mydata[TAHOMA].dat, "medium", SCRWIDTH/2+14, SCRHEIGHT/2+50, 0, -1);
			textout_ex (dbuffer, mydata[TAHOMA].dat, "medium", SCRWIDTH/2+12, SCRHEIGHT/2+48, 192, -1);
		} else {
			textout_ex (dbuffer, mydata[TAHOMA].dat, "hard", SCRWIDTH/2+14, SCRHEIGHT/2+50, 0, -1);
			textout_ex (dbuffer, mydata[TAHOMA].dat, "hard", SCRWIDTH/2+12, SCRHEIGHT/2+48, 192, -1);
		}

		draw_rle_sprite (dbuffer, mydata[MENUMOUSE].dat, mymousex, mymousey);
		release_bitmap(dbuffer);
#ifdef __WIN32__
		if (redraw) {
			acquire_bitmap(display1);
			clear (display1);
			release_bitmap(display1);
			acquire_bitmap(display2);
			clear (display2);
			release_bitmap(display2);
			redraw = 0;
		}
#endif
		}
		if (key[KEY_ESC]) { option = 5; break; }
		if ((key[KEY_ENTER] || key[KEY_SPACE]) && (option==5)) {
#ifdef SOUNDON
		if (sfxvol)
			SEALPlaySnd (mydata[CLICK1].dat, sfxvol, 128, 1000, FALSE);
#endif
		break; }
		if ((mouse_b & 1) && option == 5) {
#ifdef SOUNDON
		if (sfxvol)
			SEALPlaySnd (mydata[CLICK1].dat, sfxvol, 128, 1000, FALSE);
#endif
		break; }
	}
#ifdef MUSICON
	if (myxm!=NULL && option != 0) { stop_mod (); destroy_mod (myxm); myxm = NULL; }
#endif
#ifdef USESEAL
	SEALStopMusic ();
#endif

	return option;
}

/**********************************************************/
/* The following routines are all used in the game proper */
/**********************************************************/

/* This draws a sort of 2D wireframe spinning thing to indicate you are
 * replacing a block */
void PutTimeDraw (void)
{
int mycol;

	if (!puttimer) return;
	puttimer --;
	if (puttimer<2) return;
	if (puttimer==4) thegrid[puty][putx] |= putblock;
	mycol = makecol (255, 0, 0);
	line (dbuffer, putx*64+GXOFF+(puttimer%64), puty*64+GYOFF,
		putx*64+GXOFF+63, puty*64+GYOFF+(puttimer%64), mycol);
	line (dbuffer, putx*64+GXOFF+63, puty*64+GYOFF+(puttimer%64),
		putx*64+GXOFF+63-(puttimer%64), puty*64+GYOFF+63, mycol);
	line (dbuffer, putx*64+GXOFF+63-(puttimer%64), puty*64+GYOFF+63,
		putx*64+GXOFF, puty*64+GYOFF+63-(puttimer%64), mycol);
	line (dbuffer, putx*64+GXOFF, puty*64+GYOFF+63-(puttimer%64),
		putx*64+GXOFF+(puttimer%64), puty*64+GYOFF, mycol);
	line (dbuffer, putx*64+GXOFF+(puttimer%64), puty*64+GYOFF,
		putx*64+GXOFF+63-(puttimer%64), puty*64+GYOFF+63, mycol);
	line (dbuffer, putx*64+GXOFF+63, puty*64+GYOFF+(puttimer%64),
		putx*64+GXOFF, puty*64+GYOFF+63-(puttimer%64), mycol);
}

/* If you click to place a block, this happens: */
void SetGridTile (int x, int y)
{
int i;

	if (x<GXOFF || x>=(GXOFF+(BLX*64)) || y<GYOFF || y>=(GYOFF+(BLY*64))) return;
	x -= GXOFF; y -= GYOFF;
	x /= 64; y /= 64;
/* Need a 'no' sound here */
	if (thegrid[y][x]&0x80) {
#ifdef SOUNDON
	if (sfxvol)
	SEALPlaySnd (mydata[CHING1].dat, sfxvol, 128, 1000, FALSE);
#endif
	return; }
	if (thegrid[y][x]&0xF0) { puttimer = 150+(difficulty*50); score -= 25; putx = x; puty = y;
		putblock = nextblock[0]<<4; thegrid[y][x] &= 0x0F;
		strcpy (topmessage, "Alter block -25"); messagedur = 300;
	}
	else if (thegrid[y][x]) { thegrid[y][x] &= 0xF; thegrid[y][x] |= nextblock[0]<<4; }
	nextshunt = 32;
#ifdef SOUNDON
	if (sfxvol)
		SEALPlaySnd (mydata[CLICK1].dat, sfxvol, 128, 1000, FALSE);
#endif
	for (i=0;i<5;i++) nextblock[i] = nextblock[i+1];
	for (i=1;i<8;i++) { givenblock[i]--;
		if (givenblock[i] ==0) { givenblock[i] = 6; nextblock[5] = i; return; } }
	nextblock[5] = (random()%7)+1;
}

/* returns the block at the specified coordinates */
int GetGridTile (int x, int y)
{
	if (x<GXOFF || x>=(GXOFF+(BLX*64)) || y<GYOFF || y>=(GYOFF+(BLY*64))) return 0;
	x -= GXOFF; y -= GYOFF;
	x /= 64; y /= 64;
	return thegrid[y][x];
}

/* Draws the correct block at the specified grid reference */
void DrawGridTile (BITMAP * dest, int i, int j)
{
int k, l, m;
BITMAP * tempbm, * tempbm2, * tempbm3;

	tempbm = create_bitmap (64, 64);
	if (tempbm == NULL) return;

	tempbm3 = ((BITMAP *) mydata[GRASS01].dat);
	k = thegrid[j][i] & 0xF; if (!k) { destroy_bitmap (tempbm); return; }
	switch (k)
	{
		case 0:
			rectfill (tempbm, 0, 0, 63, 63, 0);
			break;
		case 1:
			tempbm3 = ((BITMAP *) mydata[GRASS01].dat);
			blit (tempbm3, tempbm, 0, 0, 0, 0, 64, 64);
			break;
		case 2:
			tempbm3 = ((BITMAP *) mydata[GRASS02].dat);
			blit (tempbm3, tempbm, 0, 0, 0, 0, 64, 64);
			break;
		case 3:
			tempbm3 = ((BITMAP *) mydata[GRASS03].dat);
			blit (tempbm3, tempbm, 0, 0, 0, 0, 64, 64);
			break;
		case 4:
			tempbm3 = ((BITMAP *) mydata[GRASS04].dat);
			blit (tempbm3, tempbm, 0, 0, 0, 0, 64, 64);
			break;
		case 5:
			tempbm3 = ((BITMAP *) mydata[GRASS05].dat);
			blit (tempbm3, tempbm, 0, 0, 0, 0, 64, 64);
			break;
		case 6:
			tempbm3 = ((BITMAP *) mydata[GRASS06].dat);
			blit (tempbm3, tempbm, 0, 0, 0, 0, 64, 64);
			break;
		case 12:
			tempbm3 = ((BITMAP *) mydata[DIRT01].dat);
			blit (tempbm3, tempbm, 0, 0, 0, 0, 64, 64);
			break;
		case 13:
			tempbm3 = rockbm;
			blit (tempbm3, tempbm, 0, 0, 0, 0, 64, 64);
			break;
	}
	if (thegrid[j][i]&0x80) {
	k = (thegrid[j][i]&0x70) >> 4; if (k>0 && k<8) {
		tempbm2 = pieces[k];
		for (l=0;l<64;l++) for (m=0;m<64;m++)
			if (tempbm2->line[l][m])
				tempbm->line[l][m] = ((BITMAP *) mydata[DIRT01].dat)->line[l][m];
	}
	if (k==3 && crossgrid[j][i]) {
	if (crossgrid[j][i] == 1)
		{ blit (tempbm3, tempbm, 20, 0, 20, 0, 24, 20);
		blit (tempbm3, tempbm, 20, 44, 20, 44, 24, 20);
		tempbm2 = pieces[k];
		for (l=0;l<64;l++) for (m=0;m<64;m++)
			if (l<20 || l>43) tempbm->line[l][m] |= tempbm2->line[l][m];
	}
	if (crossgrid[j][i] == 2)
		{ blit (tempbm3, tempbm, 0, 20, 0, 20, 20, 24);
		blit (tempbm3, tempbm, 44, 20, 44, 20, 20, 24);
		tempbm2 = pieces[k];
		for (l=0;l<64;l++) for (m=0;m<64;m++)
			if (m<20 || m>43) tempbm->line[l][m] |= tempbm2->line[l][m];
	} }
	} else {
	k = (thegrid[j][i]&0x70) >> 4; if (k>0 && k<8) {
		tempbm2 = pieces[k];
		for (l=0;l<64;l++) for (m=0;m<64;m++)
			tempbm->line[l][m] |= tempbm2->line[l][m];
	} }
	blit (tempbm, dest, 0, 0, i*64+GXOFF, j*64+GYOFF, 64, 64);
	destroy_bitmap (tempbm);
}

/* Draws the block the mouse is over, including red/black piece and white/black rectangle */
void DrawMouseTile (BITMAP * dest, int x, int y)
{
unsigned char mycol;
int l, m;
BITMAP * tempbm, * tempbm2;

	if (!GetGridTile (x, y)) return;
	if (x>=GXOFF && x<(GXOFF+(BLX*64)) && y>=GYOFF && y<(GYOFF+(BLY*64)))
	{
		tempbm = create_bitmap (64, 64);
		if (tempbm == NULL) return;

//	DrawGridTile (dest, x, y);
	clear (tempbm);

	tempbm2 = pieces[nextblock[0]];
	if (puttimer) mycol = (unsigned char) makecol (10, 10, 10);
	else mycol = (unsigned char) makecol (255, 0, 0);
	for (l=0;l<64;l++) for (m=0;m<64;m++)
		if (tempbm2->line[l][m]) tempbm->line[l][m] = mycol;

	if (puttimer || (GetGridTile(x, y)&0x80)) rect (tempbm, 0, 0, 63, 63, makecol (10, 10, 10));
	else rect (tempbm, 0, 0, 63, 63, makecol (255, 255, 255));
	masked_blit (tempbm, dest, 0, 0, ((x-GXOFF)/64)*64+GXOFF, ((y-GYOFF)/64)*64+GYOFF, 64, 64);
	destroy_bitmap (tempbm);
	}
}

/* Draws the entire gamegrid, for example at game start */
void DrawGrid (BITMAP * dest)
{
int i, j;

/*	tempbm = create_bitmap (64, 64);
	if (tempbm == NULL) return;
*/	for (j=0;j<BLY;j++)
	{
		for (i=0;i<BLX;i++)
		{
/*			k = thegrid[j][i] & 0xF; switch (k)
			{
				case 0:
					rectfill (tempbm, 0, 0, 63, 63, 0);
					break;
				case 1:
					blit (mydata[GRASS01].dat, tempbm, 0, 0, 0, 0, 64, 64);
					break;
				case 2:
					blit (mydata[DIRT01].dat, tempbm, 0, 0, 0, 0, 64, 64);
					break;
			}
			k = thegrid[j][i] >> 4; if (k>0 && k<8) {
				tempbm2 = pieces[k];
				for (l=0;l<64;l++) for (m=0;m<64;m++)
					tempbm->line[l][m] |= tempbm2->line[l][m];
			}
			blit (tempbm, dest, 0, 0, i*64+GXOFF, j*64+GYOFF, 64, 64);
*/
			DrawGridTile (dest, i, j);
		}
	}
//	destroy_bitmap (tempbm);
}

/* Helpful messages at top of screen */
void DrawTopMessage (BITMAP * dest)
{
	rectfill (dest, SCRWIDTH/2-100, 0, SCRWIDTH/2+99, 18, 0);
	textout_centre_ex (dest, mydata[TAHOMA].dat, topmessage, SCRWIDTH/2, 0, makecol (255, 255, 255), -1);
}

/* Why am I commenting these? */
void DrawScore (BITMAP * dest)
{
char scoretext[10];

	rectfill (dest, SCRWIDTH-122, SCRHEIGHT-25, SCRWIDTH-69, SCRHEIGHT-11, makecol (41, 4, 4));
	sprintf (scoretext, "%6d", score);
	textout_ex (dest, font, scoretext, SCRWIDTH-118, SCRHEIGHT-21, makecol (255, 255, 255), -1);
}

void DrawNextPanel (BITMAP * dest)
{
int i, j, k;
BITMAP * tempbm;
char togotext[30];

	blit (mydata[PANEL1].dat, dest, 0, 0, 0, 0, 128, 48);
	i = blockstogo; if (i<0) i = 0;
	sprintf (togotext, "Left: %d", i);
	textout_ex (dest, font, togotext, 8, 37, makecol (255, 255, 255), -1);
	tempbm = create_bitmap (32, 32);
	if (tempbm==NULL) return;

	for (i=0;i<4;i++)
	{
		for (j=0;j<32;j++)
			for (k=0;k<32;k++) tempbm->line[j][k] = pieces[nextblock[i]]->line[j*2][k*2];
		rect (tempbm, 0, 0, 31, 31, makecol (128, 128, 128));
		masked_blit (tempbm, dest, 0, 0, (3-i)*32-nextshunt, 3, 32, 32);
	}
	destroy_bitmap (tempbm);
}

/* This is a special case, where a block is in transition */
void DrawDigbm (BITMAP * dest)
{
	if (moleinx<0 || moleinx>=BLX || moleiny<0 || moleiny>=BLY) return;
	masked_blit (digbm, dbuffer, 0, 0, moleinx*64+GXOFF, moleiny*64+GYOFF, 64, 64);
}

/* Callback for do_line to smoothly put the trail behind the mole */
void DigbmCurve (BITMAP *passbm, int x, int y, int d)
{
BITMAP * tempbm;

	if (getpixel (passbm, x, y)) {
		tempbm = (BITMAP *) mydata[DIRT01].dat;
		digbm->line[y][x] = tempbm->line[y][x];
	}
}

/* The trail of earth needs to be drawn under the mole */
void UpdateDigbm (void)
{
int i, j;
BITMAP * tempbm, * tempbm2;

	if (moleinx<0 || moleinx>=BLX || moleiny<0 || moleiny>=BLY) return;

	tempbm = pieces[(thegrid[moleiny][moleinx]&0x70)>>4];
	tempbm2 = ((BITMAP *) mydata[DIRT01].dat);

	if (!moleprog) {
		switch (thegrid[moleiny][moleinx]&0x0F)
		{
			case 1:
				blit (mydata[GRASS01].dat, digbm, 0, 0, 0, 0, 64, 64);
				break;
			case 2:
				blit (mydata[GRASS02].dat, digbm, 0, 0, 0, 0, 64, 64);
				break;
			case 3:
				blit (mydata[GRASS03].dat, digbm, 0, 0, 0, 0, 64, 64);
				break;
			case 4:
				blit (mydata[GRASS04].dat, digbm, 0, 0, 0, 0, 64, 64);
				break;
			case 5:
				blit (mydata[GRASS05].dat, digbm, 0, 0, 0, 0, 64, 64);
				break;
			case 6:
				blit (mydata[GRASS06].dat, digbm, 0, 0, 0, 0, 64, 64);
				break;
		}
		for (j=0;j<64;j++) for (i=0;i<64;i++)
			if (tempbm->line[j][i]) digbm->line[j][i] |= tempbm->line[j][i];

		if (crossnote && ((thegrid[moleiny][moleinx]&0x70)==0x30))
		{
			if ((moleangle>=32 && moleangle<96) || (moleangle>=160 && moleangle<224))
				{ for (i=20;i<44;i++) for (j=0;j<64;j++) digbm->line[j][i] = tempbm2->line[j][i]; }
			else { for (i=20;i<44;i++) for (j=0;j<64;j++) digbm->line[i][j] = tempbm2->line[i][j]; }
		}
	}

	switch ((thegrid[moleiny][moleinx]&0x70)>>4)
	{
		case 1:
			if (moleangle>=0 && moleangle<128)
				for (i=20;i<44;i++) digbm->line[i][moleprog] = tempbm2->line[i][moleprog];
			else
				for (i=20;i<44;i++) digbm->line[i][63-moleprog] = tempbm2->line[i][63-moleprog];
			break;
		case 2:
			if (moleangle>=64 && moleangle<192)
				for (i=20;i<44;i++) digbm->line[moleprog][i] = tempbm2->line[moleprog][i];
			else
				for (i=20;i<44;i++) digbm->line[63-moleprog][i] = tempbm2->line[63-moleprog][i];
			break;
		case 3:
			if (moleangle>=32 && moleangle<96)
				for (i=20;i<44;i++) digbm->line[i][moleprog] = tempbm2->line[i][moleprog];
			else if (moleangle>=96 && moleangle<160)
				for (i=20;i<44;i++) digbm->line[moleprog][i] = tempbm2->line[moleprog][i];
			else if (moleangle>=160 && moleangle<224)
				for (i=20;i<44;i++) digbm->line[i][63-moleprog] = tempbm2->line[i][63-moleprog];
			else
				for (i=20;i<44;i++) digbm->line[63-moleprog][i] = tempbm2->line[63-moleprog][i];
			break;
		case 4:
			if (moleangle>=0 && moleangle<64) {
				if (moleprog<32) {
					do_line (tempbm, 63, 63, 0, 63-(moleprog*2), 0, DigbmCurve);
					do_line (tempbm, 63, 63, 0, 62-(moleprog*2), 0, DigbmCurve); }
				else {
					do_line (tempbm, 63, 63, ((moleprog-32)*2), 0, 0, DigbmCurve);
					do_line (tempbm, 63, 63, ((moleprog-32)*2)+1, 0, 0, DigbmCurve); }
			} else {
				if (moleprog<32) {
					do_line (tempbm, 63, 63, 63-(moleprog*2), 0, 0, DigbmCurve);
					do_line (tempbm, 63, 63, 62-(moleprog*2), 0, 0, DigbmCurve); }
				else {
					do_line (tempbm, 63, 63, 0, ((moleprog-32)*2), 0, DigbmCurve);
					do_line (tempbm, 63, 63, 0, ((moleprog-32)*2)+1, 0, DigbmCurve); }
			}
			break;
		case 5:
			if (moleangle>=64 && moleangle<128) {
				if (moleprog<32) {
					do_line (tempbm, 0, 63, (moleprog*2), 0, 0, DigbmCurve);
					do_line (tempbm, 0, 63, (moleprog*2)+1, 0, 0, DigbmCurve); }
				else {
					do_line (tempbm, 0, 63, 63, ((moleprog-32)*2), 0, DigbmCurve);
					do_line (tempbm, 0, 63, 63, ((moleprog-32)*2)+1, 0, DigbmCurve); }
			} else {
				if (moleprog<32) {
					do_line (tempbm, 0, 63, 63, 63-(moleprog*2), 0, DigbmCurve);
					do_line (tempbm, 0, 63, 63, 62-(moleprog*2), 0, DigbmCurve); }
				else {
					do_line (tempbm, 0, 63, 63-((moleprog-32)*2), 0, 0, DigbmCurve);
					do_line (tempbm, 0, 63, 62-((moleprog-32)*2), 0, 0, DigbmCurve); }
			}
			break;
		case 6:
			if (moleangle>=128 && moleangle<192) {
				if (moleprog<32) {
					do_line (tempbm, 0, 0, 63, (moleprog*2), 0, DigbmCurve);
					do_line (tempbm, 0, 0, 63, (moleprog*2)+1, 0, DigbmCurve); }
				else {
					do_line (tempbm, 0, 0, 63-((moleprog-32)*2), 63, 0, DigbmCurve);
					do_line (tempbm, 0, 0, 62-((moleprog-32)*2)+1, 63, 0, DigbmCurve); }
			} else {
				if (moleprog<32) {
					do_line (tempbm, 0, 0, (moleprog*2), 63, 0, DigbmCurve);
					do_line (tempbm, 0, 0, (moleprog*2)+1, 63, 0, DigbmCurve); }
				else {
					do_line (tempbm, 0, 0, 63, 63-((moleprog-32)*2), 0, DigbmCurve);
					do_line (tempbm, 0, 0, 63, 62-((moleprog-32)*2), 0, DigbmCurve); }
			}
			break;
		case 7:
			if (moleangle>=192 && moleangle<256) {
				if (moleprog<32) {
					do_line (tempbm, 63, 0, 63-(moleprog*2), 63, 0, DigbmCurve);
					do_line (tempbm, 63, 0, 62-(moleprog*2), 63, 0, DigbmCurve); }
				else {
					do_line (tempbm, 63, 0, 0, 63-((moleprog-32)*2), 0, DigbmCurve);
					do_line (tempbm, 63, 0, 0, 62-((moleprog-32)*2), 0, DigbmCurve); }
			} else {
				if (moleprog<32) {
					do_line (tempbm, 63, 0, 0, (moleprog*2), 0, DigbmCurve);
					do_line (tempbm, 63, 0, 0, (moleprog*2)+1, 0, DigbmCurve); }
				else {
					do_line (tempbm, 63, 0, ((moleprog-32)*2), 63, 0, DigbmCurve);
					do_line (tempbm, 63, 0, ((moleprog-32)*2)+1, 63, 0, DigbmCurve); }
			}
			break;
	}
}

/* The mole has reached the end of the line */
void MoleCrash (void)
{
	drawtimes = 0; moleinx = 4; moleiny = 4;
	if (!blockstogo) { moletimer = 10000; blockstogo--; return; }
#ifdef SOUNDON
	if (sfxvol)
	SEALPlaySnd (mydata[CHING1].dat, sfxvol, 128, 100, FALSE);
#endif
	endgame = 1;
}

/* Move the mole according to the current speed, piece and direction */
void MoveMole (void)
{
	if (moleinx<0 || moleinx>=BLX || moleiny<0 || moleiny>=BLY) return;

	if (moletimer) { moletimer--; return; }
	if (moledelay>1) { moledelay-=2; return; }
	moleprog++;
	if (moleprog>63) {
		moleprog = 0; if (blockstogo) moledelay = moledelaydef; else moledelay = 1;
		drawbx = moleinx; drawby = moleiny; drawtimes = 2;
		crossgrid[moleiny][moleinx]+=2;
		if ((moleangle>=32 && moleangle<96) || (moleangle>=160 && moleangle<224))
			crossgrid[moleiny][moleinx]--;
		crossnote = 0;
/* 'moleto' indicates where the mole is going, 0 is up, 1 is right, 2 is down, 3 is left */
		if (moleto==1)
		{
			moleiny--; moleangle = 0;
			if (moleiny<0) MoleCrash(); else
			switch ((thegrid[moleiny][moleinx]&0x70)>>4)
			{
				case 0:
				case 1:
				case 6:
				case 7:
					MoleCrash ();
					break;
				case 2:
					moleto = 1;
					score += 150;
					break;
				case 3:
					if (thegrid[moleiny][moleinx]&0x80) crossnote = 1;
					moleto = 1;
					score += 150;
					break;
				case 4:
					moleto = 2;
					score += 150;
					break;
				case 5:
					moleto = 4;
					score += 150;
					break;
					
			}
		}
		else if (moleto==2)
		{
			moleinx++; moleangle = 64;
			if (moleinx>=BLX) MoleCrash(); else
			switch ((thegrid[moleiny][moleinx]&0x70)>>4)
			{
				case 0:
				case 2:
				case 4:
				case 7:
					MoleCrash ();
					break;
				case 1:
					moleto = 2;
					score += 150;
					break;
				case 3:
					if (thegrid[moleiny][moleinx]&0x80) crossnote = 1;
					moleto = 2;
					score += 150;
					break;
				case 5:
					moleto = 3;
					score += 150;
					break;
				case 6:
					moleto = 1;
					score += 150;
					break;
					
			}
		}
		else if (moleto==3)
		{
			moleiny++; moleangle = 128;
			if (moleiny>=BLY) MoleCrash(); else
			switch ((thegrid[moleiny][moleinx]&0x70)>>4)
			{
				case 0:
				case 1:
				case 4:
				case 5:
					MoleCrash ();
					break;
				case 2:
					moleto = 3;
					score += 150;
					break;
				case 3:
					if (thegrid[moleiny][moleinx]&0x80) crossnote = 1;
					moleto = 3;
					score += 150;
					break;
				case 6:
					moleto = 4;
					score += 150;
					break;
				case 7:
					moleto = 2;
					score += 150;
					break;
					
			}
		}
		else if (moleto==4)
		{
			moleinx--; moleangle = 192;
			if (moleinx<0) MoleCrash(); else
			switch ((thegrid[moleiny][moleinx]&0x70)>>4)
			{
				case 0:
				case 2:
				case 5:
				case 6:
					MoleCrash ();
					break;
				case 1:
					moleto = 4;
					score += 150;
					break;
				case 3:
					if (thegrid[moleiny][moleinx]&0x80) crossnote = 1;
					moleto = 4;
					score += 150;
					break;
				case 4:
					moleto = 3;
					score += 150;
					break;
				case 7:
					moleto = 1;
					score += 150;
					break;
					
			}
		}
		if ((thegrid[moleiny][moleinx]&0xF0)==0xb0)
		{
			crosses++;
			if (crosses == 5)
			{ strcpy (topmessage, "FIVE crosses +10000"); messagedur = 300; score += 10000;
#ifdef SOUNDON
	if (sfxvol)
			SEALPlaySnd (mydata[APPLAUSE].dat, sfxvol, 128, 1000, FALSE);
#endif
			}
			else if (crosses == 10)
			{ strcpy (topmessage, "TEN crosses +50000"); messagedur = 300; score += 50000;
#ifdef SOUNDON
	if (sfxvol)
					SEALPlaySnd (mydata[APPLAUSE].dat, sfxvol, 128, 1000, FALSE);
#endif
			} else { strcpy (topmessage, "Cross bonus +2000"); messagedur = 300; score += 2000; }
		}
		thegrid[moleiny][moleinx] |= 0x80;
		score += difficulty*100;
		if (blockstogo>0) blockstogo--; else score += 150;
		return;
	}
/* This is pretty poor, it works out the direction from the moleangle */
	switch ((thegrid[moleiny][moleinx]&0x70)>>4)
	{
		case 4:
			if (moleangle>=0 && moleangle<64) moleangle++;
			else moleangle--;
			break;
		case 5:
			if (moleangle>=64 && moleangle<128) moleangle++;
			else moleangle--;
			break;
		case 6:
			if (moleangle>=128 && moleangle<192) moleangle++;
			else moleangle--;
			break;
		case 7:
			if (moleangle>=192 && moleangle<256) moleangle++;
			else moleangle--;
			break;
	}
	moleangle &= 0xFF;
	if (blockstogo) moledelay = moledelaydef; else moledelay = 1;
}

/* This is pretty awful too, works out the pixel position of the mole from the
 * grid position and progress of the mole... */
void CalcMolePos (void)
{
	if (moleinx<0 || moleinx>=BLX || moleiny<0 || moleiny>=BLY) return;

	molex = moleinx*64+GXOFF+18;
	moley = moleiny*64+GYOFF+10;

	switch ((thegrid[moleiny][moleinx]&0x70)>>4)
	{
		case 0:
			if (moleto==1) moley -= moleprog/2;
			if (moleto==2) molex += moleprog/2;
			if (moleto==3) moley += moleprog/2;
			if (moleto==4) molex -= moleprog/2;
			break;
		case 1:
			if (moleangle>=0 && moleangle<128) { molex -= 32; molex += moleprog; }
			else { molex += 32; molex -= moleprog; }
			break;
		case 2:
			if (moleangle>=64 && moleangle<192) { moley -= 32; moley += moleprog; }
			else { moley += 32; moley -= moleprog; }
			break;
		case 3:
			if (moleangle>=32 && moleangle<96) { molex -= 32; molex += moleprog; }
			else if (moleangle>=96 && moleangle<160) { moley -= 32; moley += moleprog; }
			else if (moleangle>=160 && moleangle<224) { molex += 32; molex -= moleprog; }
			else { moley += 32; moley -= moleprog; }
			break;
		case 4:
			if (moleangle>=0 && moleangle<64)
			{
				moley += 32;
				molex += 32 -fixtoi ((32*fixcos(itofix(moleangle))));
				moley -= fixtoi ((32*fixsin(itofix(moleangle))));
			} else {
				molex += 64; moley += 32;
				molex -= 32 -fixtoi ((32*fixcos(itofix(moleangle))));
				moley += fixtoi ((32*fixsin(itofix(moleangle))));
			}
			break;
		case 5:
			if (moleangle>=64 && moleangle<128)
			{
				molex -= 64; moley += 32;
				molex += 32 -fixtoi ((32*fixcos(itofix(moleangle))));
				moley -= fixtoi ((32*fixsin(itofix(moleangle))));
			} else {
				moley += 32;
				molex -= 32 -fixtoi ((32*fixcos(itofix(moleangle))));
				moley += fixtoi ((32*fixsin(itofix(moleangle))));
			}
			break;
		case 6:
			if (moleangle>=128 && moleangle<192)
			{
				moley -= 32; molex -= 64;
				molex += 32 -fixtoi ((32*fixcos(itofix(moleangle))));
				moley -= fixtoi ((32*fixsin(itofix(moleangle))));
			} else {
				moley -= 32;
				molex -= 32 -fixtoi ((32*fixcos(itofix(moleangle))));
				moley += fixtoi ((32*fixsin(itofix(moleangle))));
			}
			break;
		case 7:
		default:
			if (moleangle>=192 && moleangle<256)
			{
				moley -= 32;
				molex += 32 -fixtoi ((32*fixcos(itofix(moleangle))));
				moley -= fixtoi ((32*fixsin(itofix(moleangle))));
			} else {
				molex += 64; moley -= 32;
				molex -= 32 -fixtoi ((32*fixcos(itofix(moleangle))));
				moley += fixtoi ((32*fixsin(itofix(moleangle))));
			}
			break;
	}
}

/* Actually inititialises each level */
int InitGame (void)
{
int i, j;
BITMAP * tempbm;

/* Create all the pieces (save drawing them by hand!) */
	for (i=0;i<8;i++) {
		if ((pieces[i] = create_bitmap (64, 64)) == NULL) return -1;
		clear (pieces[i]);
	}
	rectfill (pieces[1], 0, 20, 63, 43, 128);
	rectfill (pieces[2], 20, 0, 43, 63, 128);
	rectfill (pieces[3], 0, 20, 63, 43, 128);
	rectfill (pieces[3], 20, 0, 43, 63, 128);
	circlefill (pieces[4], 63, 63, 43, 128);
	circlefill (pieces[4], 63, 63, 19, 0);
	circlefill (pieces[5], 0, 63, 43, 128);
	circlefill (pieces[5], 0, 63, 19, 0);
	circlefill (pieces[6], 0, 0, 43, 128);
	circlefill (pieces[6], 0, 0, 19, 0);
	circlefill (pieces[7], 63, 0, 43, 128);
	circlefill (pieces[7], 63, 0, 19, 0);

/* Do strange things to palette. Colours 0-127 are game colours, 128-255 are either
 * darker or lighter depending on the level. Transparency can then be done with
 * colour&=0x7F and colour|=0x80 */
	memcpy (mypal, mydata[GAMEPAL].dat, sizeof(PALETTE));
	if (level%2) for (i=0;i<128;i++) {
		mypal[i+128].r = (mypal[i].r>>1) + (mypal[i].r>>2);
		mypal[i+128].g = (mypal[i].g>>1) + (mypal[i].g>>2);
		mypal[i+128].b = (mypal[i].b>>1) + (mypal[i].b>>2);
	} else for (i=0;i<128;i++) {
		mypal[i+128].r = (mypal[i].r) + (mypal[i].r);
		if (mypal[i+128].r>63) mypal[i+128].r = 63;
		mypal[i+128].g = (mypal[i].g) + (mypal[i].g);
		if (mypal[i+128].g>63) mypal[i+128].g = 63;
		mypal[i+128].b = (mypal[i].b) + (mypal[i].b);
		if (mypal[i+128].b>63) mypal[i+128].b = 63;
	}

/* Custom orange colour for next blocks */
	mypal[128].r = 54; mypal[128].g = 20; mypal[128].b = 0;
/* Fill grid in with background blocks for that level */
	for (j=0;j<BLY;j++) for (i=0;i<BLX;i++) thegrid[j][i] = ((level-1)%6)+1;
/* The grid is a strange shape, get rid of 4 blocks */
	thegrid[0][0] = 0; thegrid[0][1] = 0;
	thegrid[BLY-1][BLX-2] = 0; thegrid[BLY-1][BLX-1] = 0;
/* Level design, easy eh? */
	switch (level)
	{
		case 1:
			thegrid[2][6] = 0x8D; thegrid[3][2] = 0x21;
			thegrid[4][2] = 0x8C; moleinx = 2; moleiny = 4; moleto = 1; moleangle = 0;
			break;
		case 2:
			thegrid[1][6] = 0x32; thegrid[5][2] = 0x32; thegrid[5][4] = 0x8D;
			thegrid[2][4] = 0x8C; moleinx = 4; moleiny = 2; moleto = 3; moleangle = 128;
			break;
		case 3:
			thegrid[3][3] = 0x8D; thegrid[2][4] = 0x8D; thegrid[4][4] = 0x8D;
			thegrid[3][7] = 0x8C; moleinx = 7; moleiny = 3; moleto = 4; moleangle = 192;
			break;
		case 4:
			thegrid[1][3] = 0x34;
			thegrid[0][5] = 0x8D; thegrid[1][5] = 0x8D; thegrid[6][3] = 0x8D; thegrid[5][3] = 0x8D;
			thegrid[0][2] = 0x8C; moleinx = 2; moleiny = 0; moleto = 2; moleangle = 64;
			break;
		case 5:
			thegrid[3][5] = 0x35; thegrid[3][4] = 0x35; thegrid[2][5] = 0x35; thegrid[2][4] = 0x35;
			thegrid[3][6] = 0x8C; moleinx = 6; moleiny = 3; moleto = 4; moleangle = 192;
			break;
		case 6:
			thegrid[2][3] = 0x8D; thegrid[2][5] = 0x8D; thegrid[4][3] = 0x8D; thegrid[4][5] = 0x8D;
			thegrid[5][1] = 0x8C; moleinx = 1; moleiny = 5; moleto = 2; moleangle = 64;
			break;
		case 7:
			thegrid[1][2] = 0x21; thegrid[2][2] = 0x21; thegrid[3][2] = 0x21;
			thegrid[1][6] = 0x21; thegrid[2][6] = 0x21; thegrid[3][6] = 0x21;
			thegrid[2][4] = 0x8C; moleinx = 4; moleiny = 2; moleto = 3; moleangle = 128;
			break;
		case 8:
			thegrid[4][2] = 0x8D; thegrid[4][3] = 0x8D; thegrid[4][4] = 0x8D;
			thegrid[4][5] = 0x8D; thegrid[4][6] = 0x8D;
			thegrid[6][4] = 0x8C; moleinx = 4; moleiny = 6; moleto = 4; moleangle = 192;
			break;
		case 9:
			thegrid[2][4] = 0x33; thegrid[4][2] = 0x8D; thegrid[4][6] = 0x8D;
			thegrid[3][4] = 0x8C; moleinx = 4; moleiny = 3; moleto = 1; moleangle = 0;
			break;
		case 10:
			thegrid[1][6] = 0x74; thegrid[0][6] = 0x44; thegrid[0][7] = 0x14;
			thegrid[0][8] = 0x54; thegrid[1][8] = 0x24;
			thegrid[1][7] = 0x8C; moleinx = 7; moleiny = 1; moleto = 4; moleangle = 192;
			break;
		default:
			thegrid[1][4] = 0x8D; thegrid[5][4] = 0x8D;
			thegrid[3][2] = 0x8D; thegrid[3][6] = 0x8D;
			thegrid[3][4] = 0x8C; moleinx = 4; moleiny = 3; moleto = 1; moleangle = 0;
			break;
	}
	tempbm = (BITMAP *) mydata[DIRT01].dat;
/* This looks a bit dodgy, alter 'dirt' to be lighter or darker */
	if (level%2) for (i=0;i<64;i++) for (j=0;j<64;j++) tempbm->line[i][j] &= 0x7F;
	else for (i=0;i<64;i++) for (j=0;j<64;j++) tempbm->line[i][j] |= 0x80;

/* Draw screen */
	acquire_bitmap(display1);
	clear (display1); DrawGrid (display1);
	blit (mydata[PANEL1].dat, display1, 0, 0, 0, 0, 128, 48);
	blit (mydata[PANEL2].dat, display1, 0, 0, SCRWIDTH-128, SCRHEIGHT-48, 128, 48);
	release_bitmap(display1);
	acquire_bitmap(display2);
	clear (display2); DrawGrid (display2);
	blit (mydata[PANEL1].dat, display2, 0, 0, 0, 0, 128, 48);
	blit (mydata[PANEL2].dat, display2, 0, 0, SCRWIDTH-128, SCRHEIGHT-48, 128, 48);
	release_bitmap(display2);

/* set up 'next' pieces */
	srandom (time (NULL));
	for (i=0;i<6;i++) nextblock[i] = (random()%7)+1;
	for (i=0;i<8;i++) givenblock[i] = 6;
	nextshunt = 0;

	crosses = 0; crossnote = 0;
	for (j=0;j<BLY;j++) for (i=0;i<BLX;i++) crossgrid[j][i] = 0;
	puttimer = 0; drawtimes = 0;
	moletimer = 0; molex = 0; moley = 0;
	moleaframe = 0; moleprog = 0; moletimer = 1000-(level*50)-(difficulty*200);
	if (moletimer<200) moletimer = 200;
	moledelaydef = 7-(level/2)-difficulty;
	if (moledelaydef<1) moledelaydef = 1; moledelay = moledelaydef;
	blockstogo = 8+(level*2);
/* position_mouse doesn't seem to work under WinAllegro at the moment, so commented out */
#ifndef __WIN32__
	position_mouse (SCRWIDTH/2, SCRHEIGHT/2);
#endif

	if (level<21) strcpy (topmessage, leveltxt[level]);
	else sprintf (topmessage, "Level %d", level);
	messagedur = 600;
	return 0;
}

/* There is a small bug in here somewhere. Draws '-200' over unused blocks */
void ScrubBlocks (BITMAP * dest)
{
int i, j;

	for (j=0;j<BLY;j++) for (i=0;i<BLX;i++)
		if ((thegrid[j][i]&0x70) && !(thegrid[j][i]&0x80))
		{
			score -= 200;
			textout_centre_ex (dest, mydata[TAHOMA].dat,
				"-200", i*64+GXOFF+32, j*64+GYOFF+22, makecol (10, 10, 10), -1);
			textout_centre_ex (dest, mydata[TAHOMA].dat,
				"-200", i*64+GXOFF+30, j*64+GYOFF+20, makecol (255, 255, 255), -1);
		}
#ifdef SOUNDON
	if (sfxvol)
					SEALPlaySnd (mydata[APPLAUSE].dat, sfxvol, 128, 1000, FALSE);
#endif
}

void CloseGame (void)
{
int i;

	for (i=0;i<8;i++) if (pieces[i] != NULL) destroy_bitmap (pieces[i]);
}

/* This is the 'callback' function for if someone alt-tabs away, or Molefest loses
 * the screen for some reason, it redraws everything (nearly) */
#ifdef __WIN32__
void GameRedraw (void)
{
	acquire_bitmap(display1);
	clear (display1); DrawGrid (display1);
	blit (mydata[PANEL1].dat, display1, 0, 0, 0, 0, 128, 48);
	blit (mydata[PANEL2].dat, display1, 0, 0, SCRWIDTH-128, SCRHEIGHT-48, 128, 48);
	release_bitmap(display1);
	acquire_bitmap(display2);
	clear (display2); DrawGrid (display2);
	blit (mydata[PANEL1].dat, display2, 0, 0, 0, 0, 128, 48);
	blit (mydata[PANEL2].dat, display2, 0, 0, SCRWIDTH-128, SCRHEIGHT-48, 128, 48);
	release_bitmap(display2);
	redraw = 0;
}
#endif

/* This is the biggy, the main game loop and other rubbish */
void StartGame (void)
{
int timedelay, option, mymousex, mymousey, mymousedn, oldx, oldy, oldx2, oldy2;
int oldmx, oldmy, oldmx2, oldmy2;
int keyu, keyd, keyl, keyr, keyx, keyy, keyf, keyff;
int paused, overold, overnew;
BITMAP *mouset1, *mouset2, *tempmt, *molet1, *molet2;

	myfadeout ();
	acquire_bitmap(display1);
	clear (display1);
	release_bitmap(display1);
	acquire_bitmap(display2);
	clear (display2);
	release_bitmap(display2);
	level = 1; score = 0; endgame = 0;
	mouset1 = create_bitmap (12, 22);
	if (mouset1 == NULL) { set_palette (desktop_palette);
		alert ("create_bitmap error", NULL, NULL, "&OK", NULL, 'o', 0); return; }
	mouset2 = create_bitmap (12, 22);
	if (mouset2 == NULL) { set_palette (desktop_palette);
		alert ("create_bitmap error", NULL, NULL, "&OK", NULL, 'o', 0); return; }
	molet1 = create_bitmap (48, 48);
	if (mouset1 == NULL) { set_palette (desktop_palette);
		alert ("create_bitmap error", NULL, NULL, "&OK", NULL, 'o', 0); return; }
	molet2 = create_bitmap (48, 48);
	if (mouset2 == NULL) { set_palette (desktop_palette);
		alert ("create_bitmap error", NULL, NULL, "&OK", NULL, 'o', 0); return; }
	digbm = create_bitmap (64, 64);
	if (digbm == NULL) { set_palette (desktop_palette);
		alert ("create_bitmap error", NULL, NULL, "&OK", NULL, 'o', 0); return; }
	rockbm = create_bitmap (64, 64);
	if (rockbm == NULL) { set_palette (desktop_palette);
		alert ("create_bitmap error", NULL, NULL, "&OK", NULL, 'o', 0); return; }

#ifdef MUSICON
	if (myxm==NULL && modvol) {
		myxm = load_mod ("data/lurid.xm");
		if (myxm!=NULL) play_mod (myxm, TRUE); 
		set_mod_volume (modvol);
	}
#endif
#ifdef USESEAL
	SEALStartMusic (1, modvol);
#endif
	
	while (!endgame) {
	clear (digbm); switch ((((level-1)%6)+1)) {
		case 1:
			blit (mydata[GRASS01].dat, rockbm, 0, 0, 0, 0, 64, 64);
			break;
		case 2:
			blit (mydata[GRASS02].dat, rockbm, 0, 0, 0, 0, 64, 64);
			break;
		case 3:
			blit (mydata[GRASS03].dat, rockbm, 0, 0, 0, 0, 64, 64);
			break;
		case 4:
			blit (mydata[GRASS04].dat, rockbm, 0, 0, 0, 0, 64, 64);
			break;
		case 5:
			blit (mydata[GRASS05].dat, rockbm, 0, 0, 0, 0, 64, 64);
			break;
		case 6:
			blit (mydata[GRASS06].dat, rockbm, 0, 0, 0, 0, 64, 64);
			break;
	} draw_rle_sprite (rockbm, mydata[ROCK1].dat, 0, 0);
	if (InitGame ()) {
		acquire_bitmap(display1);
		clear (display1);
		release_bitmap(display1);
		acquire_bitmap(display2);
		clear (display2);
		release_bitmap(display2);
		set_palette (desktop_palette);
		alert ("Couldn't initialise Game", NULL, NULL, "&OK", NULL, 'o', 0); return; }

	keyu = 0; keyd = 0; keyl = 0; keyr = 0; keyx = 4; keyy = 3; keyf = 0; keyff = 0;
	paused = 0; overold = 0; overnew = 0;
	fpsdelay = 0; fpstimer = 0; fpscounter = 0; fps = 0;
	mymousex = mouse_x; mymousey = mouse_y; mymousedn = 0;
	oldx = mymousex; oldy = mymousey; option = 0; drawtimes = 0;
	oldx2 = oldx; oldy2 = oldy;
	oldmx = molex-6; oldmy = moley;
	oldmx2 = oldmx; oldmy2 = oldmy;
	acquire_bitmap(display1);
	blit (display1, mouset1, mymousex, mymousey, 0, 0, 12, 22);
	blit (display1, molet1, molex-6, moley, 0, 0, 48, 48);
	release_bitmap(display1);
	acquire_bitmap(display2);
	blit (display2, mouset2, mymousex, mymousey, 0, 0, 12, 22);
	blit (display2, molet2, molex-6, moley, 0, 0, 48, 48);
	release_bitmap(display2);
	while (mouse_b & 1); myfadein(); while (1)
	{
		if (fpsdelay) { timedelay = fpsdelay; fpsdelay = 0;
		if (vsyncon==1) vsync ();
#ifndef __WIN32__
		scroll_screen (0, scroffset);
		if (scroffset) dbuffer = display1; else dbuffer = display2;
#else
		WinFlip ();
#endif
		scroffset = SCRHEIGHT-scroffset;
		if (vsyncon==2) vsync ();

		oldx2 = oldx; oldy2 = oldy;
		oldx = mymousex; oldy = mymousey;
		mymousex = mouse_x; mymousey = mouse_y;
		tempmt = mouset1; mouset1 = mouset2; mouset2 = tempmt;

		acquire_bitmap(dbuffer);
		blit (mouset1, dbuffer, 0, 0, oldx2, oldy2, 12, 22);
		oldmx2 = oldmx; oldmy2 = oldmy;
		oldmx = molex-6; oldmy = moley;
		tempmt = molet1; molet1 = molet2; molet2 = tempmt;
		blit (molet1, dbuffer, 0, 0, oldmx2, oldmy2, 48, 48);
		if (oldx2>=GXOFF && oldx2<(GXOFF+(BLX*64)) && oldy2>=GYOFF && oldy2<(GYOFF+(BLY*64)))
			DrawGridTile (dbuffer, (oldx2-GXOFF)/64, (oldy2-GYOFF)/64);
		if (puttimer) DrawGridTile (dbuffer, putx, puty);
		if (drawtimes) { drawtimes--; DrawGridTile (dbuffer, drawbx, drawby); }
		release_bitmap(dbuffer);
#ifdef __WIN32__
		if (redraw) GameRedraw ();
#endif

		while (timedelay) {
			if (paused) { if (keypressed()) { paused = 0; while (key[KEY_F1]); } else break; }
			if ((mouse_b & 1 || keyf) && !mymousedn)
			{
				if (puttimer) {
#ifdef SOUNDON
	if (sfxvol)
					SEALPlaySnd (mydata[CHING1].dat, sfxvol, 128, 1000, FALSE);
					mymousedn = 1; keyf = 0;
#endif
				} else {
				mymousedn = 1; keyf = 0;
				if (GetGridTile (mymousex, mymousey))
					SetGridTile (mymousex, mymousey); }
			}
			if (!(mouse_b & 1)) mymousedn = 0;

			if (key[KEY_UP] && !keyu) { do { keyy--; if (keyy<0) keyy = BLY-1; } while (!thegrid[keyy][keyx]);
				position_mouse (keyx*64+GXOFF+32, keyy*64+GYOFF+32); keyu = 1; }
			if (key[KEY_DOWN] && !keyd) { do { keyy++; if (keyy>=BLY) keyy = 0; } while (!thegrid[keyy][keyx]);
				position_mouse (keyx*64+GXOFF+32, keyy*64+GYOFF+32); keyd = 1; }
			if (key[KEY_LEFT] && !keyl) { do { keyx--; if (keyx<0) keyx = BLX-1; } while (!thegrid[keyy][keyx]);
				position_mouse (keyx*64+GXOFF+32, keyy*64+GYOFF+32); keyl = 1; }
			if (key[KEY_RIGHT] && !keyr) { do { keyx++; if (keyx>=BLX) keyx = 0; } while (!thegrid[keyy][keyx]);
				position_mouse (keyx*64+GXOFF+32, keyy*64+GYOFF+32); keyr = 1; }
			if ((key[KEY_ENTER] || key[KEY_SPACE]) && !keyff) { keyf = 1; keyff = 1; }
			if (key[KEY_F1]) { paused = 1; clear_keybuf(); }
			if (!key[KEY_UP]) keyu = 0;
			if (!key[KEY_DOWN]) keyd = 0;
			if (!key[KEY_LEFT]) keyl = 0;
			if (!key[KEY_RIGHT]) keyr = 0;
			if (!key[KEY_ENTER] && !key[KEY_SPACE]) keyff = 0;

			moleaframe++; if (moleaframe==(4<<3)) moleaframe = 0;
			MoveMole (); UpdateDigbm ();
			if (nextshunt) nextshunt--;
			if (messagedur) messagedur--; { if (!messagedur) topmessage[0] = 0; }
			timedelay--;
			if (endgame) break;
		}

		if (blockstogo<0) break;
		if (endgame) break;
		if (key[KEY_ESC]) { endgame = 2; break; }
		overnew = ((mymousex-GXOFF)/64) + (((mymousey-GYOFF)/64)*10);
#ifdef SOUNDON
		if ((overnew != overold) && mymousex>GXOFF && mymousex<(BLX*64+GXOFF) &&
			mymousey>GYOFF && mymousey<(BLY*64+GYOFF))
			if (GetGridTile (mymousex, mymousey))
			if (sfxvol>60)
				SEALPlaySnd (mydata[CLICK2].dat, sfxvol-60, 128, 1500, FALSE);
#endif
		overold = overnew;

		acquire_bitmap(dbuffer);
		DrawNextPanel (dbuffer);
		DrawScore (dbuffer);
		DrawTopMessage (dbuffer);
		DrawDigbm (dbuffer);
		DrawMouseTile (dbuffer, mymousex, mymousey);
		PutTimeDraw ();
		blit (dbuffer, mouset1, mymousex, mymousey, 0, 0, 12, 22);
		CalcMolePos ();
		blit (dbuffer, molet1, molex-6, moley, 0, 0, 48, 48);
		switch (moleaframe>>3)
		{
			case 0:
				rotate_sprite (dbuffer, mydata[MOLE2].dat,
					molex, moley, moleangle<<16);
			break;
			case 1:
			case 3:
				rotate_sprite (dbuffer, mydata[MOLE1].dat,
					molex, moley, moleangle<<16);
			break;
			case 2:
				rotate_sprite (dbuffer, mydata[MOLE3].dat,
					molex, moley, moleangle<<16);
			break;
		}

		draw_rle_sprite (dbuffer, mydata[GAMEMOUSE].dat, mymousex, mymousey);
		release_bitmap(dbuffer);
	} }

	if (endgame<2) { 
		acquire_bitmap(dbuffer);
		ScrubBlocks (dbuffer);
		DrawScore (dbuffer);
		release_bitmap(dbuffer);
		if (vsyncon==1) vsync ();
#ifndef __WIN32__
		scroll_screen (0, scroffset);
		if (scroffset) dbuffer = display1; else dbuffer = display2;
#else
		WinFlip ();
#endif
		scroffset = SCRHEIGHT-scroffset;
		if (vsyncon==2) vsync ();
		fpsdelay = 0; timedelay = 0; while (1) { timedelay+=fpsdelay; fpsdelay = 0; if (timedelay>100) break; }
	}

	myfadeout ();
	acquire_bitmap(display1);
	clear (display1);
	release_bitmap(display1);
	acquire_bitmap(display2);
	clear (display2);
	release_bitmap(display2);
	if (!endgame) level++;
	CloseGame ();
	}
	destroy_bitmap (mouset1); destroy_bitmap (mouset2);
	destroy_bitmap (molet1); destroy_bitmap (molet2);
	destroy_bitmap (digbm); destroy_bitmap (rockbm);
#ifdef MUSICON
	if (myxm!=NULL) { stop_mod (); destroy_mod (myxm); myxm = NULL; }
#endif
#ifdef USESEAL
	SEALStopMusic ();
#endif

	if (endgame<2) GoHighScore ();
}

/* WinAllegro callback to note screen has been lost */
#ifdef __WIN32__
void Callback (void)
{
	redraw = 1;
}
#endif

/* Yep, the prog starts here */
int main (int argc, char *argv[])
{
int choice;

	fliptype = 0; if (argc > 1) {
		if (!strcmp(argv[1], "-fliptype3")) fliptype = 3;
		if (!strcmp(argv[1], "-fliptype4")) fliptype = 4;
	}
	ReadPrefs ();
	ReadHighScores ();
#ifdef __WIN32__
//	WinAllegro_SetWindowTitle ("Molefest - WinAllegro version");
	set_display_switch_callback(SWITCH_IN, Callback);
#endif
	allegro_init ();
	install_timer ();
	install_keyboard ();
	install_mouse ();

#ifdef SOUNDON
#ifndef USESEAL
	reserve_voices (16, -1);
	if (install_sound (DIGI_AUTODETECT, MIDI_NONE, NULL) < 0)
        {
        printf ("Error initializing sound card");
//        return 1;
        }
#endif
#endif
#ifdef MUSICON
	if (install_mod (14)==-1) { } //allegro_exit (); return -1; }
	myxm = NULL;
#endif
	if (fliptype == 4) { choice = GFX_AUTODETECT_WINDOWED; fliptype = 0; }
	else { choice = GFX_AUTODETECT; }
#ifndef __WIN32__
	if (set_gfx_mode (GFX_AUTODETECT_WINDOWED, SCRWIDTH, SCRHEIGHT, 0, SCRHEIGHT*2)<0)
		{ allegro_exit (); printf ("Couldn't open %d*%d*%d screen\n",
			SCRWIDTH, SCRHEIGHT, SCRDEPTH); return -1; }
#else
	if (set_gfx_mode (choice, SCRWIDTH, SCRHEIGHT, 0, 0)<0) //SCRHEIGHT*2)<0)
		{ allegro_exit (); return -1; }
#endif
#ifndef __WIN32__
	display1 = create_sub_bitmap (screen, 0, 0, SCRWIDTH, SCRHEIGHT);
	display2 = create_sub_bitmap (screen, 0, SCRHEIGHT, SCRWIDTH, SCRHEIGHT);
#else
	if (fliptype == 0) {
		display1 = create_video_bitmap (SCRWIDTH, SCRHEIGHT);
		if (display1==NULL) { allegro_exit (); return -1; }
		display2 = create_video_bitmap (SCRWIDTH, SCRHEIGHT);
		if (display2==NULL) { allegro_exit (); return -1; }
	}
	if (fliptype == 3) {
		display1 = create_bitmap (SCRWIDTH, SCRHEIGHT);
		display2 = create_bitmap (SCRWIDTH, SCRHEIGHT);
		dbuffer = display1;
	}
#endif
	set_window_title ("Molefest V1.05");
	scroffset = 0; dbuffer = display2;
#ifdef USESEAL
    /* initialize audio library */
    SEALOpen ();
#endif

/* Lock 'n load FPS routines under interrupt */
	LOCK_VARIABLE (fpsdelay); LOCK_VARIABLE (fps);
	LOCK_VARIABLE (fpstimer); LOCK_VARIABLE (fpscounter);
	LOCK_FUNCTION (fpsinterrupt);
	install_int_ex (fpsinterrupt, BPS_TO_TIMER(60));
	
	set_mouse_speed (msespd, msespd);
	mydata = NULL; while (1)
	{
		if (attract ()) { remove_int (fpsinterrupt); allegro_exit ();
			printf ("Error loading attractor\n"); return -1; }
		switch (mainmenu())
		{
			case 0:
				break;
			case 1:
				StartGame ();
				break;
			case 2:
				myfadeout ();
				acquire_bitmap(display1);
				clear (display1);
				release_bitmap(display1);
				acquire_bitmap(display2);
				clear (display2);
				release_bitmap(display2);
				ChangeOptions ();
				break;
			case 3:
				myfadeout ();
				acquire_bitmap(display1);
				clear (display1);
				release_bitmap(display1);
				acquire_bitmap(display2);
				clear (display2);
				release_bitmap(display2);
				score = 0; GoHighScore ();
				break;
			default:
				goto endprog;
		}
	}

endprog:
	myfadeout ();
	unload_datafile (mydata);
	remove_int (fpsinterrupt);
#ifdef __WIN32__
	clear (display1); clear (display2);
	destroy_bitmap (display1);
	destroy_bitmap (display2);
#endif
	allegro_exit ();
	SavePrefs ();
	SaveHighScores ();
#ifdef USESEAL
	SEALClose ();
#endif
	return 0;
}
END_OF_MAIN();
