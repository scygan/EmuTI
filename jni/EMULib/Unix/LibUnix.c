/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                        LibUnix.c                        **/
/**                                                         **/
/** This file contains Unix-dependent implementation        **/
/** parts of the emulation library.                         **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1996-2009                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#include "../EMULib.h"
#include "LibUnix.h"

#include <ctype.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>


#define FPS_COLOR PIXEL(255,0,255)

//extern int MasterSwitch; /* Switches to turn channels on/off */
//extern int MasterVolume; /* Master volume                    */

static volatile int TimerReady = 0;   /* 1: Sync timer ready */
static volatile unsigned int JoyState = 0; /* Joystick state */
static volatile unsigned int LastKey  = 0; /* Last key prsd  */
static volatile unsigned int KeyModes = 0; /* SHIFT/CTRL/ALT */

static int Effects    = /*EFF_SCALE|*/EFF_SAVECPU; /* EFF_* bits */
static int TimerON    = 0; /* 1: sync timer is running       */
static Image OutImg;       /* Scaled output image buffer     */
static const char *AppTitle; /* Current window title         */
static int XSize,YSize;    /* Current window dimensions      */

static int FrameCount;      /* Frame counter for EFF_SHOWFPS */
static int FrameRate;       /* Last frame rate value         */
static struct timeval TimeStamp; /* Last timestamp           */

/** TimerHandler() *******************************************/
/** The main timer handler used by SetSyncTimer().          **/
/*************************************************************/
static void TimerHandler(int Arg)
{
  /* Mark sync timer as "ready" */
  TimerReady=1;
  /* Repeat signal next time */
  signal(Arg,TimerHandler);
}

/** InitUnix() ***********************************************/
/** Initialize Unix/X11 resources and set initial window    **/
/** title and dimensions.                                   **/
/*************************************************************/
int InitUnix(const char *Title,int Width,int Height)
{
  /* Initialize variables */
  AppTitle    = Title;
  XSize       = Width;
  YSize       = Height;
  TimerON     = 0;
  TimerReady  = 0;
  JoyState    = 0;
  LastKey     = 0;
  KeyModes    = 0;
  FrameCount  = 0;
  FrameRate   = 0;

  /* Get initial timestamp */
  gettimeofday(&TimeStamp,0);

  /* No output image yet */
//  OutImg.XImg            = 0;
#ifdef MITSHM
//  OutImg.SHMInfo.shmaddr = 0;
#endif

  /* Done */
  return(1);
}

/** TrashUnix() **********************************************/
/** Free resources allocated in InitUnix()                  **/
/*************************************************************/
void TrashUnix(void)
{
  /* Remove sync timer */
  SetSyncTimer(0);
  /* Shut down audio */
//TrashAudio();
  /* Free output image buffer */
  //FreeImage(&OutImg);
}

void PrintXY(Image *Img,const char *S,int X,int Y,pixel FG,int BG);

/** ShowVideo() **********************************************/
/** Show "active" image at the actual screen or window.     **/
/*************************************************************/
int ShowVideo(void)
{
  Image *Output;
  int SX,SY;

  /* Must have active video image, X11 display */
  if(!VideoImg||!VideoImg->Data) return(0);

  /* Allocate image buffer if none */
  //if(!OutImg.Data&&!NewImage(&OutImg,XSize,YSize)) return(0);

  /* If not scaling or post-processing image, avoid extra work */
  if(!(Effects&(EFF_SOFTEN|EFF_SCALE|EFF_TVLINES)))
  {
	FlipImage(VideoImg);
    return(1);
  }

  /* By default, we will be showing OutImg */
  Output  = &OutImg;
  SX      = 0;
  SY      = 0;

  if(Effects&EFF_SOFTEN)
  {
    /* Apply softening */
    SoftenImage(&OutImg,VideoImg,VideoX,VideoY,VideoW,VideoH);
    /* Apply TV scanlines, if needed */
    if(Effects&EFF_TVLINES)
      TelevizeImage(&OutImg,0,0,OutImg.W,OutImg.H);
  }
  else if(Effects&EFF_TVLINES)
  {
    if(Effects&EFF_SCALE)
    {
      /* Scale VideoImg into OutImg */
      ScaleImage(&OutImg,VideoImg,VideoX,VideoY,VideoW,VideoH);
      /* Apply TV scanlines */
      TelevizeImage(&OutImg,0,0,OutImg.W,OutImg.H);
    }
    else
    {
      /* Center VideoImg in OutImg */
      IMGCopy(&OutImg,(OutImg.W-VideoW)>>1,(OutImg.H-VideoH)>>1,VideoImg,VideoX,VideoY,VideoW,VideoH,-1);
      /* Apply TV scanlines */
      TelevizeImage(&OutImg,(OutImg.W-VideoW)>>1,(OutImg.H-VideoH)>>1,VideoW,VideoH);
    }
  }
  else if((OutImg.W==VideoW)&&(OutImg.H==VideoH))
  {
    /* Use VideoImg directly */
    Output = VideoImg;
    SX     = VideoX;
    SY     = VideoY;
  }
  else if(Effects&EFF_SCALE)
  {
    /* Scale VideoImg to OutImg */
    ScaleImage(&OutImg,VideoImg,VideoX,VideoY,VideoW,VideoH);
  }
  else if((OutImg.W<=VideoW)&&(OutImg.H<=VideoH))
  {
    /* Use VideoImg directly */
    Output = VideoImg;
    SX     = VideoX+((VideoW-OutImg.W)>>1);
    SY     = VideoY+((VideoH-OutImg.H)>>1);
  }
  else
  {
    /* Center VideoImg in OutImg */
    IMGCopy(&OutImg,(OutImg.W-VideoW)>>1,(OutImg.H-VideoH)>>1,VideoImg,VideoX,VideoY,VideoW,VideoH,-1);
  }

  /* Show framerate if requested */
  if((Effects&EFF_SHOWFPS)&&(FrameRate>0))
  {
    char S[8];
    sprintf(S,"%dfps",FrameRate);
    PrintXY(
      &OutImg,S,
      ((OutImg.W-VideoW)>>1)+8,((OutImg.H-VideoH)>>1)+8,
      FPS_COLOR,-1
    );
  }

  /* Wait for sync timer if requested */
  if(Effects&EFF_SYNC) WaitSyncTimer();

  /* Copy image to the window, either using SHM or not */
#ifdef MITSHM
//  if(VideoImg->Attrs&EFF_MITSHM)
//    XShmPutImage(Dsp,Wnd,DefaultGCOfScreen(Scr),Output->XImg,SX,SY,0,0,XSize,YSize,False);
//  else
#endif
//    XPutImage(Dsp,Wnd,DefaultGCOfScreen(Scr),Output->XImg,SX,SY,0,0,XSize,YSize);

  /* Done */
  return(1);
}

/** GetJoystick() ********************************************/
/** Get the state of joypad buttons (1="pressed"). Refer to **/
/** the BTN_* #defines for the button mappings.             **/
/*************************************************************/
unsigned int GetJoystick(void)
{
  /* Count framerate */
  if((Effects&EFF_SHOWFPS)&&(++FrameCount>=300))
  {
    struct timeval NewTS;
    int Time;

    gettimeofday(&NewTS,0);
    Time       = (NewTS.tv_sec-TimeStamp.tv_sec)*1000
               + (NewTS.tv_usec-TimeStamp.tv_usec)/1000;
    FrameRate  = 1000*FrameCount/(Time>0? Time:1); 
    TimeStamp  = NewTS;
    FrameCount = 0;
    FrameRate  = FrameRate>999? 999:FrameRate;
  }

  /* Process any pending events */
  //X11ProcessEvents();

  /* Return current joystick state */
  return(JoyState|KeyModes);
}

/** GetMouse() ***********************************************/
/** Get mouse position and button states in the following   **/
/** format: RMB.LMB.Y[29-16].X[15-0].                       **/
/*************************************************************/
unsigned int GetMouse(void)
{
  int X,Y,J;
  uint Mask;
//  Window W;

  /* Need to have a display and a window */
  //if(!Dsp||!Wnd) return(0);

  /* Query mouse pointer */
//  if(!XQueryPointer(Dsp,Wnd,&W,&W,&J,&J,&X,&Y,&Mask)) return(0);

  /* If scaling video... */
  if(Effects&(EFF_SOFTEN|EFF_SCALE|EFF_TVLINES))
  {
    /* Scale mouse position */
    X = VideoW*(X<0? 0:X>=XSize? XSize-1:X)/XSize;
    Y = VideoH*(Y<0? 0:Y>=YSize? YSize-1:Y)/YSize;
  }
  else
  {
    /* Translate mouse position */
    X-= ((XSize-VideoW)>>1);
    Y-= ((YSize-VideoH)>>1);
    X = X<0? 0:X>=XSize? XSize-1:X;
    Y = Y<0? 0:Y>=YSize? YSize-1:Y;
  }

  /* Return mouse position and buttons */
/*  return(
    (X&0xFFFF)
  | ((Y&0x3FFF)<<16)
  | (Mask&Button1Mask? 0x40000000:0)
  | (Mask&Button3Mask? 0x80000000:0)
  );*/
  return 0;
}

/** GetKey() *************************************************/
/** Get currently pressed key or 0 if none pressed. Returns **/
/** CON_* definitions for arrows and special keys.          **/
/*************************************************************/
unsigned int GetKey(void)
{
  unsigned int J;

//  X11ProcessEvents();
  J=LastKey;
  LastKey=0;
  return(J);
}

/** WaitKey() ************************************************/
/** Wait for a key to be pressed. Returns CON_* definitions **/
/** for arrows and special keys.                            **/
/*************************************************************/
unsigned int WaitKey(void)
{
  unsigned int J;

  /* Swallow current keypress */
  GetKey();
  /* Wait in 100ms increments for a new keypress */
  while(!(J=GetKey())&&VideoImg) usleep(100000);
  /* Return key code */
  return(J);
}

/** WaitKeyOrMouse() *****************************************/
/** Wait for a key or a mouse button to be pressed. Returns **/
/** the same result as GetMouse(). If no mouse buttons      **/
/** reported to be pressed, do GetKey() to fetch a key.     **/
/*************************************************************/
unsigned int WaitKeyOrMouse(void)
{
  unsigned int J;

  /* Swallow current keypress */
  GetKey();
  /* Make sure mouse keys are not pressed */
  while(GetMouse()&0xC0000000) usleep(100000);
  /* Wait in 100ms increments for a key or mouse click */
  while(!(J=GetKey())&&!(GetMouse()&0xC0000000)&&VideoImg) usleep(100000);
  /* Place key back into the buffer and return mouse state */
  LastKey=J;
  return(GetMouse());
}

/** WaitSyncTimer() ******************************************/
/** Wait for the timer to become ready.                     **/
/*************************************************************/
void WaitSyncTimer(void)
{
  /* Wait in 1ms increments until timer becomes ready */
  while(!TimerReady&&TimerON&&VideoImg) usleep(1000);
  /* Warn of missed timer events */
  if((TimerReady>1)&&(Effects&EFF_VERBOSE))
    LOGI("WaitSyncTimer(): Missed %d timer events.\n",TimerReady-1);
  /* Reset timer */
  TimerReady=0;
}

/** SyncTimerReady() *****************************************/
/** Return 1 if sync timer ready, 0 otherwise.              **/
/*************************************************************/
int SyncTimerReady(void)
{
  /* Return whether timer is ready or not */
  return(TimerReady||!TimerON||!VideoImg);
}

/** SetSyncTimer() *******************************************/
/** Set synchronization timer to a given frequency in Hz.   **/
/*************************************************************/
int SetSyncTimer(int Hz)
{
  struct itimerval TimerValue;

  /* Compute and set timer period */
  TimerValue.it_interval.tv_sec  =
  TimerValue.it_value.tv_sec     = 0;
  TimerValue.it_interval.tv_usec =
  TimerValue.it_value.tv_usec    = Hz? 1000000L/Hz:0;

  /* Set timer */
  if(setitimer(ITIMER_REAL,&TimerValue,NULL)) return(0);

  /* Set timer signal */
  signal(SIGALRM,Hz? TimerHandler:SIG_DFL);

  /* Done */
  TimerON=Hz;
  return(1);
}

/** NewImage() ***********************************************/
/** Create a new image of the given size. Returns pointer   **/
/** to the image data on success, 0 on failure.             **/
/*************************************************************/
pixel *NewImage(Image *Img,int Width,int Height)
{
//  XVisualInfo VInfo;
  int Depth,J,I;

  /* Set data fields to ther defaults */
  Img->Data    = 0;
  Img->W       = 0;
  Img->H       = 0;
  Img->L       = 0;
  Img->D       = 0;
//  Img->Attrs   = 0;
  Img->Cropped = 0;

  {
	int ret;
	Img->aBitmap = g_MainBitmap;

    if ((AndroidBitmap_getInfo(g_JNIEnv, Img->aBitmap, &Img->aBitmapInfo)) < 0) {
        LOGE("AndroidBitmap_getInfo() failed ! error=%d", ret);
        return NULL;
    }

    if (Img->aBitmapInfo.format != ANDROID_BITMAP_FORMAT_RGBA_8888) {
        LOGE("Bitmap format is not RGBA_8888 !");
        return NULL;
    }

    //image has pixels locked by default
    LockImage(Img);
  }

  /* Done */
  Img->D     = 24;
  Img->W     = Img->aBitmapInfo.width;
  Img->H     = Img->aBitmapInfo.height;
  Img->L     = Img->aBitmapInfo.stride / 4;
//  Img->Attrs = Effects;
  return(Img->Data);
}

/** FreeImage() **********************************************/
/** Free previously allocated image.                        **/
/*************************************************************/
void FreeImage(Image *Img)
{
  /* Need to initalize library first */
//  if(!Dsp||!Img->Data) return;

#if 0 //def MITSHM
  /* Detach shared memory segment */
  if((Img->Attrs&EFF_MITSHM)&&Img->SHMInfo.shmaddr)
  { XShmDetach(Dsp,&Img->SHMInfo);shmdt(Img->SHMInfo.shmaddr); }
  Img->SHMInfo.shmaddr = 0;
#endif

  /* Get rid of the image */
//  if(Img->XImg) { XDestroyImage(Img->XImg);Img->XImg=0; }
  UnlockImage(Img);

  /* Image freed */
  Img->Data = 0;
  Img->W    = 0;
  Img->H    = 0;
  Img->L    = 0;
}

void LockImage(Image *Img) {
	int ret;
	if ((ret = AndroidBitmap_lockPixels(g_JNIEnv, Img->aBitmap, (void **) &Img->Data)) < 0) {
		        LOGE("AndroidBitmap_lockPixels() failed ! error=%d", ret);
	}
}

void UnlockImage(Image *Img) {
	AndroidBitmap_unlockPixels(g_JNIEnv, Img->aBitmap);
}

void FlipImage(Image *Img) {
	UnlockImage(Img);
	jni_FlipImage();
	LockImage(Img);
}

/** CropImage() **********************************************/
/** Create a subimage Dst of the image Src. Returns Dst.    **/
/*************************************************************/
Image *CropImage(Image *Dst,const Image *Src,int X,int Y,int W,int H)
{
  Dst->Data    = (pixel *)(Src->Data+Src->L*Y+X);
  Dst->Cropped = 1;
  Dst->W       = W;
  Dst->H       = H;
  Dst->L       = Src->L;
  Dst->D       = Src->D;
//  Dst->XImg    = 0;
//  Dst->Attrs   = 0;
  return(Dst);
}

void GenericSetVideo(Image *Img,int X,int Y,int W,int H);

/** SetVideo() ***********************************************/
/** Set part of the image as "active" for display.          **/
/*************************************************************/
void SetVideo(Image *Img,int X,int Y,int W,int H)
{
  /* If video exists, modify its size */
  if(/*Dsp&&*/VideoW&&VideoH)
  {
    int DW,DH,SW,SH;

    /* Make sure window dimensions stay at ~1:1 ratio */
    DW    = W/H>1? W/(W/H):W;
    DH    = H/W>1? H/(H/W):H;
    SW    = VideoW/VideoH>1? VideoW/(VideoW/VideoH):VideoW;
    SH    = VideoH/VideoW>1? VideoH/(VideoH/VideoW):VideoH;
    XSize = XSize*DW/SW;
    YSize = YSize*DH/SH;

  //  if(Wnd) XResizeWindow(Dsp,Wnd,XSize,YSize);
    //FreeImage(&OutImg);
  }

  /* Call default SetVideo() function */
  GenericSetVideo(Img,X,Y,W,H);
}

/** X11SetEffects() ******************************************/
/** Set visual effects applied to video in ShowVideo().     **/
/*************************************************************/
void X11SetEffects(int NewEffects)
{
  /* Set new effects */
  Effects=NewEffects;
}

/** X11ProcessEvents() ***************************************/
/** Process X11 event messages.                             **/
/*************************************************************/
void X11ProcessEvents(void)
{
	int x, y, pressed;
    /* Check for mouse clicks, but only one at a time */
    if(jni_GetTouch(&x, &y, &pressed))
    {
	   (*MouseHandler)(x, y, pressed!=0?1:0);
    }
#if 0
  XEvent E;
  int J;

  /* Need to have display and a window */
  if(!Dsp||!Wnd) return;

  /* Check for keypresses/keyreleases */
  while(XCheckWindowEvent(Dsp,Wnd,KeyPressMask|KeyReleaseMask,&E))
  {
    /* Get key code */
    J=XLookupKeysym((XKeyEvent *)&E,0);

    /* If key pressed... */
    if(E.type==KeyPress)
    {
      /* Process ASCII keys */
      if((J>=' ')&&(J<0x7F)) LastKey=toupper(J);

      /* Special keys pressed... */
      switch(J)
      {
        case XK_Left:         JoyState|=BTN_LEFT;LastKey=CON_LEFT;break;
        case XK_Right:        JoyState|=BTN_RIGHT;LastKey=CON_RIGHT;break;
        case XK_Up:           JoyState|=BTN_UP;LastKey=CON_UP;break;
        case XK_Down:         JoyState|=BTN_DOWN;LastKey=CON_DOWN;break;
        case XK_Shift_L:
        case XK_Shift_R:      KeyModes|=CON_SHIFT;break;
        case XK_Alt_L:
        case XK_Alt_R:        KeyModes|=CON_ALT;break;
        case XK_Control_L:
        case XK_Control_R:    KeyModes|=CON_CONTROL;break;
        case XK_Caps_Lock:    KeyModes|=CON_CAPS;break;
        case XK_Escape:       JoyState|=BTN_EXIT;LastKey=CON_EXIT;break;
        case XK_Tab:          JoyState|=BTN_SELECT;break;
        case XK_Return:       JoyState|=BTN_START;LastKey=CON_OK;break;
        case XK_BackSpace:    LastKey=8;break;

        case 'q': case 'e': case 't': case 'u': case 'o':
          JoyState|=BTN_FIREL;break;
        case 'w': case 'r': case 'y': case 'i': case 'p':
          JoyState|=BTN_FIRER;break;
        case 'a': case 's': case 'd': case 'f': case 'g':
        case 'h': case 'j': case 'k': case 'l': case ' ':
          JoyState|=BTN_FIREA;break;
        case 'z': case 'x': case 'c': case 'v': case 'b':
        case 'n': case 'm':
          JoyState|=BTN_FIREB;break;

        case XK_Page_Up:
          if(KeyModes&CON_ALT)
          {
            /* Volume up */
//            SetChannels(MasterVolume<247? MasterVolume+8:255,MasterSwitch);
            /* Key swallowed */
            J=0;
          } 
          break;

        case XK_Page_Down:
          if(KeyModes&CON_ALT)
          {
            /* Volume down */
//            SetChannels(MasterVolume>8? MasterVolume-8:0,MasterSwitch);
            /* Key swallowed */
            J=0;
          } 
          break;
      }

      /* Call user handler */
      if(J&&KeyHandler) (*KeyHandler)(J|KeyModes);
    }

    /* If key released... */
    if(E.type==KeyRelease)
    {
      /* Special keys released... */
      switch(J)
      {
        case XK_Left:         JoyState&=~BTN_LEFT;break;
        case XK_Right:        JoyState&=~BTN_RIGHT;break;
        case XK_Up:           JoyState&=~BTN_UP;break;
        case XK_Down:         JoyState&=~BTN_DOWN;break;
        case XK_Shift_L:
        case XK_Shift_R:      KeyModes&=~CON_SHIFT;break;
        case XK_Alt_L:
        case XK_Alt_R:        KeyModes&=~CON_ALT;break;
        case XK_Control_L:
        case XK_Control_R:    KeyModes&=~CON_CONTROL;break;
        case XK_Caps_Lock:    KeyModes&=~CON_CAPS;break;
        case XK_Escape:       JoyState&=~BTN_EXIT;break;
        case XK_Tab:          JoyState&=~BTN_SELECT;break;
        case XK_Return:       JoyState&=~BTN_START;break;

        case 'q': case 'e': case 't': case 'u': case 'o':
          JoyState&=~BTN_FIREL;break;
        case 'w': case 'r': case 'y': case 'i': case 'p':
          JoyState&=~BTN_FIRER;break;
        case 'a': case 's': case 'd': case 'f': case 'g':
        case 'h': case 'j': case 'k': case 'l': case ' ':
          JoyState&=~BTN_FIREA;break;
        case 'z': case 'x': case 'c': case 'v': case 'b':
        case 'n': case 'm':
          JoyState&=~BTN_FIREB;break;
      }

      /* Call user handler */
      if(J&&KeyHandler) (*KeyHandler)(J|CON_RELEASE|KeyModes);
    }
  }

  /* Check for mouse clicks, but only one at a time */
  if(XCheckWindowEvent(Dsp,Wnd,ButtonPressMask|ButtonReleaseMask,&E))
  {
    (*MouseHandler)(E.xbutton.x, E.xbutton.y, E.type==ButtonPress?1:0);
  }

  /* Check for focus change events */
  for(E.type=0;XCheckWindowEvent(Dsp,Wnd,FocusChangeMask,&E););
  /* If saving CPU and focus is out... */
  if((Effects&EFF_SAVECPU)&&(E.type==FocusOut))
  {
    /* Pause audio */
//    J=MasterSwitch;
//    SetChannels(MasterVolume,0);
//    PauseAudio(1);
    /* Wait for focus-in event */  
    do
      while(!XCheckWindowEvent(Dsp,Wnd,FocusChangeMask,&E)&&VideoImg) sleep(1);
    while((E.type!=FocusIn)&&VideoImg);
    /* Resume audio */
//    PauseAudio(0);
//    SetChannels(MasterVolume,J);
  }

  /* If window has been resized, remove current output buffer */
  for(E.type=0;XCheckWindowEvent(Dsp,Wnd,StructureNotifyMask,&E););
  if((E.type==ConfigureNotify)&&!E.xconfigure.send_event)
    if((XSize!=E.xconfigure.width)||(YSize!=E.xconfigure.height))
    {
      FreeImage(&OutImg);
      XSize=E.xconfigure.width;
      YSize=E.xconfigure.height;
    }
#endif
}

/** X11GetColor **********************************************/
/** Get pixel for the current screen depth based on the RGB **/
/** values.                                                 **/
/*************************************************************/
unsigned int X11GetColor(unsigned char R,unsigned char G,unsigned char B)
{
  int J;

  /* If using constant BPP, just return a pixel */
  if(/*!Dsp||*/!(Effects&EFF_VARBPP)) return(PIXEL(R,G,B));

  /* If variable BPP, compute pixel based on the screen depth */
//J=DefaultDepthOfScreen(Scr);
  return(
    J<=8?  (((7*(R)/255)<<5)|((7*(G)/255)<<2)|(3*(B)/255))
  : J<=16? (((31*(R)/255)<<11)|((63*(G)/255)<<5)|(31*(B)/255))
  : J<=32? (((R)<<16)|((G)<<8)|B)
  : 0
  );  
}
