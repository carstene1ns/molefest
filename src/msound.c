#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <allegro.h>
#include "moledata.h"
//#define USESEAL 1
//#define FORWIN 1
#ifdef FORWIN
#include <winalleg.h>
#endif
#ifdef USESEAL
#include "midasdll.h"
#endif

extern DATAFILE * mydata;

#ifdef USESEAL
static MIDASmodule myxm = NULL;
static MIDASmodulePlayHandle hplayHandle;
static MIDASsample samples[16];
static MIDASsamplePlayHandle lastsample;
//static LPAUDIOWAVE SEALsamp[16];
//static HAC SEALvoices[4];
//static int SEALuse;
//static AUDIOINFO info;
#endif

#ifdef USESEAL

void SEALPauseMod (void) {
	if (myxm == NULL) return;
	MIDASsetMusicVolume (hplayHandle, 0);	
}

void SEALResumeMod (int vol) {
	if (myxm == NULL) return;
	MIDASsetMusicVolume (hplayHandle, vol/4+1);
}

void SEALOpen (void)
{
int i;

	lastsample = 0;
	MIDASstartup();
	MIDASsetOption(MIDAS_OPTION_DSOUND_HWND, (DWORD) win_get_window());
	MIDASsetOption(MIDAS_OPTION_DSOUND_MODE, MIDAS_DSOUND_STREAM);
	MIDASinit();
	MIDASstartBackgroundPlay(100);
	MIDASopenChannels(20);
	MIDASallocAutoEffectChannels(4);
	for (i=0;i<16;i++) samples[i] = 0;
	samples[0] = MIDASloadWaveSample ("SOUNDFX\\APPLAUSE.WAV", 0);
	samples[1] = MIDASloadWaveSample ("SOUNDFX\\CHING1.WAV", 0);
	samples[2] = MIDASloadWaveSample ("SOUNDFX\\CLICK1.WAV", 0);
	samples[3] = MIDASloadWaveSample ("SOUNDFX\\CLICK2.WAV", 0);
}

void SEALClose (void)
{
int i;

	if (lastsample != 0)
		MIDASstopSample (lastsample);
	MIDASfreeAutoEffectChannels();
	for (i=0;i<10;i++) {
		if (samples[i] != 0)
			MIDASfreeSample (samples[i]); samples[i] = 0;
	}
	MIDASstopBackgroundPlay();
	MIDASclose();
}

int SEALStartMusic (int swhich, int vol)
{
//int i;
	if (myxm != NULL) {
//		ASetModuleVolume(vol/4+1);	
		MIDASsetMusicVolume (hplayHandle, vol/4+1);
		return 0;
	}

//	for (i=0;i<4;i++) SEALvoices[i] =0;

	if (swhich == 0) {
		myxm = MIDASloadModule ("K_PBOX.XM");
	} else { if (swhich == 1) {
		myxm = MIDASloadModule ("LURID.XM");
	} else {
		return -1;
	} }

	hplayHandle = MIDASplayModule(myxm, TRUE);
	MIDASsetMusicVolume (hplayHandle, vol/4+1);
	return 0;
}

void SEALStopMusic (void)
{
//int i;

	if (myxm == NULL) return;
	MIDASstopModule(hplayHandle);
	MIDASfreeModule(myxm); myxm = NULL;
/*	for (i=0;i<16;i++) {
		if (SEALsamp[i] != NULL) 
			ADestroyAudioData (SEALsamp[i]); SEALsamp[i] = NULL;
	}
	ACloseVoices();
	AFreeModuleFile(myxm);
	myxm = NULL;
*/
}
#endif

void SEALPlaySnd (SAMPLE * smpl, int vol, int pan, int frq, int loop)
{
#ifdef USESEAL
int i;
#endif

	if (vol == 0 ) return;
#ifdef USESEAL
	if (myxm == NULL) return;
	i = 0;
	if (smpl == (SAMPLE*) mydata[APPLAUSE].dat) i = 0;
	if (smpl == (SAMPLE*) mydata[CHING1].dat) i = 1;
	if (smpl == (SAMPLE*) mydata[CLICK1].dat) i = 2;
	if (smpl == (SAMPLE*) mydata[CLICK2].dat) i = 3;
	switch (frq) {
		case 100: frq = 1102; break;
		case 800: frq = 11000; break;
		case 2000: frq = 22050; break;
		default: frq = 11025; break;
	}
	lastsample = MIDASplaySample(samples[i], MIDAS_CHANNEL_AUTO, 0, frq, vol/4+1, MIDAS_PAN_MIDDLE);
	return;
#else
	play_sample (smpl, vol, 127, frq, 0);
#endif
}
