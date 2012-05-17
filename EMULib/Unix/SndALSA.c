/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                        SndALSA.c                        **/
/**                                                         **/
/** This file contains ALSA-dependent sound implementation  **/
/** for the emulation library.                              **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1996-2009                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#include "EMULib.h"
#include "Sound.h"

#include <ctype.h>
#include <stdio.h>
#include <sys/types.h>
#include <asm/types.h>
#include <linux/types.h>
#include <sys/ioctl.h>
#include <sound/asound.h>

static void *ALSA     = 0;  /* Handle to ALSA audio driver   */
static int SndSize;         /* ALSA audio buffer size        */

/** InitAudio() **********************************************/
/** Initialize sound. Returns rate (Hz) on success, else 0. **/
/** Rate=0 to skip initialization (will be silent).         **/
/*************************************************************/
unsigned int InitAudio(unsigned int Rate,unsigned int Latency)
{
  struct snd_pcm_hw_params *HWParams;
  struct snd_pcm_sw_params *SWParams;
  struct snd_output *Log;
  int J,I;

  /* Uninitialize sound, just in case */
  TrashSound();

  /* No sound yet */
  ALSA = 0;

  /* If not initializing sound, drop out */
  if(!Rate) return(0);

  /* Open ALSA PCM device */
  if(snd_pcm_open(&ALSA,"default",SNDRV_PCM_STREAM_PLAYBACK,0)<0) 
  { ALSA=0;return(0); }

  /* Will be logging to STDOUT */
  snd_output_stdio_attach(&Log,stdout,0);

  /* Allocate parameters structures */
  if(snd_pcm_hw_params_malloc(&HWParams)<0) return(0);
  if(snd_pcm_sw_params_malloc(&SWParams)<0)
  { snd_pcm_hw_params_free(HWParams);return(0); }

  /* Set hardware parameters */
  J = snd_pcm_hw_params_any(ALSA,HWParams);
  J = J<0? J:snd_pcm_hw_params_set_access(ALSA,HWParams,SNDRV_PCM_ACCESS_RW_INTERLEAVED);
  J = J<0? J:snd_pcm_hw_params_set_format(ALSA,HWParams,sizeof(sample)>1? SNDRV_PCM_FORMAT_S16_LE:SNDRV_PCM_FORMAT_S8);
  J = J<0? J:snd_pcm_hw_params_set_channels(ALSA,HWParams,1);
  J = J<0? J:snd_pcm_hw_params_set_rate_near(ALSA,HWParams,&Rate,0);
  J = J<0? J:snd_pcm_hw_params_set_buffer_size(ALSA,HWParams,SndSize=((Rate*Latency)/1000+31)&~31);
  J = J<0? J:snd_pcm_hw_params_set_periods(ALSA,HWParams,4,0);
  J = J<0? J:snd_pcm_hw_params(ALSA,HWParams);

  /* Set software parameters */
  J = snd_pcm_sw_params_current(ALSA,SWParams);
  J = J<0? J:snd_pcm_sw_params_get_boundary(SWParams,&I);
  J = J<0? J:snd_pcm_sw_params_set_stop_threshold(ALSA,SWParams,I);
  J = J<0? J:snd_pcm_sw_params(ALSA,SWParams);

  /* Done with parameters structures */
  snd_pcm_hw_params_dump(HWParams,Log);
  snd_pcm_sw_params_dump(SWParams,Log);
  snd_pcm_hw_params_free(HWParams);
  snd_pcm_sw_params_free(SWParams);

  /* Sound initialized */
  return(Rate);  
}

/** TrashAudio() *********************************************/
/** Free resources allocated by InitAudio().                **/
/*************************************************************/
void TrashAudio(void)
{
  /* Uninitialize ALSA sound */
  if(ALSA) snd_pcm_close(ALSA);
  ALSA = 0;
}

/** GetFreeAudio() *******************************************/
/** Get the amount of free samples in the audio buffer.     **/
/*************************************************************/
unsigned int GetFreeAudio(void)
{
  int J;
  
  /* Audio should be initialized */
  if(!ALSA) return(0);

  /* Get available frame count */
  J=snd_pcm_avail_update(ALSA);
  return(J>0? J:0);
}

/** WriteAudio() *********************************************/
/** Write up to a given number of samples to audio buffer.  **/
/** Returns the number of samples written.                  **/
/*************************************************************/
unsigned int WriteAudio(sample *Data,unsigned int Length)
{
  int J;

  /* Audio should be initialized */
  if(!ALSA) return(0);

  /* Send audio data */
  J=snd_pcm_writei(ALSA,Data,Length);

  /* If failed, try recovering */
  if(J<0) J=snd_pcm_recover(ALSA,J,0);

  /* Report problems */
  if(J<0) printf("ALSA: %s (%d)\n",snd_strerror(J),J);

  /* Done */
  return(J>0? J:0);
}
