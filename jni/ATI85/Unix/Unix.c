/** AlmostTI: portable TI calcs emulator *********************/
/**                                                         **/
/**                         Unix.c                          **/
/**                                                         **/
/** This file contains Unix-dependent subroutines and       **/
/** drivers. It includes screen drivers via Display.h.      **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 2009                      **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#include "../TI85.h"
#include "../../EMULib/Console.h"
#include "../../EMULib/EMULib.h"
#include "../../EMULib/Unix/LibUnix.h"
//#include "Sound.h"

#include <string.h>
#include <stdio.h>

#define WIDTH       480                   /* Buffer width    */
#define HEIGHT      800                   /* Buffer height   */
#define XOFFSET     (WIDTH-128*3)/2       /* Display offset  */
#define YOFFSET     24                    /* Display offset  */
#define PORTRAIT

/* Combination of EFF_* bits */
int UseEffects  = EFF_SAVECPU;// /*EFF_SCALE||EFF_MITSHM|EFF_VARBPP|EFF_SYNC*/;

int InMenu;                /* 1: In MenuTI85(), ignore keys  */
int UseStatic   = 1;       /* 1: Use static palette          */
int UseZoom     = 1;       /* Zoom factor (1=no zoom)        */
int UseSound    = 22050;   /* Audio sampling frequency (Hz)  */
int SyncFreq    = 60;      /* Sync frequency (0=sync off)    */
int FastForward;           /* Fast-forwarded UPeriod backup  */
//int SndSwitch;           /* Mask of enabled sound channels */
//int SndVolume;           /* Master volume for audio        */
byte KeyReady;             /* 1: Key has been pressed        */
Image OutImage;            /* Unix display image buffer      */
Image ScrImage;            /* TI85 display image buffer      */
unsigned int XPal[2];      /* Referenced from Common.h       */
unsigned int Black,White;

const char *Title = "AlmostTI-Unix 1.3"; /* Program version  */

void HandleKeys(unsigned int Key);
void HandleMouse(int X, int Y, int State);

/** TouchMap[] ***********************************************/
/** This map translates on-screen rectangles into keys.     **/
/*************************************************************/
struct
{
  byte KeyCode;
  int X,Y,W,H;
} TouchMap[] =
{
  { KBD_RIGHT,  430,368,35,45 },
  { KBD_LEFT,   295,368,35,45 },
  { KBD_UP,     345,337,59,28 },
  { KBD_DOWN,   345,417,59,28 },

  { KBD_F5,     388,275,77,25 },
  { KBD_F4,     295,275,77,25 },
  { KBD_F3,     201,275,77,25 },
  { KBD_F2,     108,275,77,25 },
  { KBD_F1,     12,275,77,25 },

  { KBD_CLEAR,  389,462,78,33 },
  { KBD_POWER,  389,510,78,33 },
  { KBD_DIV,    389,557,78,33 },
  { KBD_MUL,    389,604,78,33 },
  { KBD_MINUS,  389,651,78,33 },
  { KBD_PLUS,   389,699,78,33 },
  { KBD_ENTER,  389,746,78,42 },

  { KBD_CUSTOM, 295,462,78,33 },
  { KBD_TAN,    295,510,78,33 },
  { KBD_RPARENT,295,557,78,33 },
  { KBD_9,      295,604,78,33 },
  { KBD_6,      295,651,78,33 },
  { KBD_3,      295,699,78,33 },
  { KBD_SIGN,   295,746,78,33 },
  
  { KBD_MORE,   203,368,78,33 },
  { KBD_DEL,    203,415,78,33 },
  { KBD_PRGM,   203,462,78,33 },
  { KBD_COS,    203,510,78,33 },
  { KBD_LPARENT,203,557,78,33 },
  { KBD_8,      203,604,78,33 },
  { KBD_5,      203,651,78,33 },
  { KBD_2,      203,699,78,33 },
  { KBD_DOT,    203,746,78,33 },

  { KBD_EXIT,   106,368,78,33 },
  { KBD_XVAR,   106,415,78,33 },
  { KBD_STAT,   106,462,78,33 },
  { KBD_SIN,    106,510,78,33 },
  { KBD_EE,     106,557,78,33 },
  { KBD_7,      106,604,78,33 },
  { KBD_4,      106,651,78,33 },
  { KBD_1,      106,699,78,33 },
  { KBD_0,      106,746,78,33 },

  { KBD_2ND,    12,368,78,33 },
  { KBD_ALPHA,  12,415,78,33 },
  { KBD_GRAPH,  12,462,78,33 },
  { KBD_LOG,    12,510,78,33 },
  { KBD_LN,     12,557,78,33 },
  { KBD_SQR,    12,604,78,33 },
  { KBD_COMMA,  12,651,78,33 },
  { KBD_STO,    12,699,78,33 },
  { KBD_ON,     12,746,78,33 },

  { 0,0,0,0,0 }
};

void SetMouseHandler(void (*Handler)(int X, int Y, int State));

/** InitMachine() ********************************************/
/** Allocate resources needed by machine-dependent code.    **/
/*************************************************************/
int InitMachine(void)
{
//  int J;

  /* Initialize variables */
  UseZoom       = UseZoom<1? 1:UseZoom>5? 5:UseZoom;
  InMenu        = 0;
  FastForward   = 0;
  OutImage.Data = 0;
  KeyReady      = 0;

  /* Initialize system resources */
  InitUnix(Title,UseZoom*WIDTH,UseZoom*HEIGHT);

  /* Set visual effects */
  X11SetEffects(UseEffects);

  /* Create main image buffer */
  if(!NewImage(&OutImage,WIDTH,HEIGHT)) { TrashUnix();return(0); }
  ClearImage(&OutImage,X11GetColor(0,0,0));
  CropImage(&ScrImage,&OutImage,XOFFSET,YOFFSET,3*128,3*64);

  /* Initialize video to main image */
  SetVideo(&OutImage,0,0,WIDTH,HEIGHT);

  /* Set colors */
  XPal[0] = White = X11GetColor(255,255,255);
  XPal[1] = Black = X11GetColor(0,0,0);

  /* Attach keyboard handler */
  SetKeyHandler(HandleKeys);

  /* Attach mouse handler */
  SetMouseHandler(HandleMouse);

  /* Initialize sound */
//InitSound(UseSound,150);
//SndSwitch=(1<<4)-1;
//SndVolume=255/4;
//SetChannels(SndVolume,SndSwitch);

  /* Initialize sync timer if needed */
//  if((SyncFreq>0)&&!SetSyncTimer(SyncFreq*UPeriod/100)) SyncFreq=0;
  LOGE("Not enabling timer, but SyncFreq = %d", SyncFreq);
#warning timer is not enabled

  /* Done */
  return(1);
}

/** TrashMachine() *******************************************/
/** Deallocate all resources taken by InitMachine().        **/
/*************************************************************/
void TrashMachine(void)
{
  FreeImage(&OutImage);
//TrashAudio();
  TrashUnix();
}

/** SetColor() ***********************************************/
/** Allocate new color.                                     **/
/*************************************************************/
void SetColor(byte N,byte R,byte G,byte B)
{
  /* Set requested color */
  XPal[N&1]=X11GetColor(R,G,B);
}

/** PutImage() ***********************************************/
/** Put an image on the screen.                             **/
/*************************************************************/
void PutImage(void)
{
  /* Show rendered screen buffer */
  ShowVideo();
}

/** Keypad() *************************************************/
/** Poll the keyboard.                                      **/ 
/*************************************************************/
byte Keypad(void)
{
  X11ProcessEvents();
  return(IS_KBD(KBD_ON));
}

/** ShowBackdrop() *******************************************/
/** Show backdrop image with calculator faceplate.          **/
/*************************************************************/
#include "lodepng.h"

int ShowBackdrop(const char *FileName)
{
  /* Not loading backdrop image for now */
  unsigned char* buffer;
  unsigned char* image;
  size_t buffersize, imagesize;// i;
  LodePNG_Decoder decoder;
  
  // Load the image file with given filename.
  LodePNG_loadFile(&buffer, &buffersize, FileName);

  // Decode the png.
  LodePNG_Decoder_init(&decoder);
  LodePNG_decode(&decoder, &image, &imagesize, buffer, buffersize);
  
  // If there's an error, display it and quit
  if (decoder.error) {
      LOGE("error: %d\n", decoder.error);
      return(0);
  }
  if (decoder.infoPng.width != WIDTH) {
      LOGE("error: skin width != %d (got %d)\n", WIDTH, decoder.infoPng.width);
      return(0);
  }
  if (decoder.infoPng.height != HEIGHT) {
      LOGE("error: skin height != %d (got %d)\n", HEIGHT, decoder.infoPng.height);
      return(0);
  }

  // Draw the image on the canvas
  uint j;
  pixel *P = (pixel *)OutImage.Data;
  unsigned char *Q = image;
    
  for (j = HEIGHT * WIDTH; j; j--) {
      *P++ = X11GetColor(*Q,*(Q+1),*(Q+2));
      Q+=4;
  }

  // Cleanup decoder
  free(image);
  free(buffer);
  LodePNG_Decoder_cleanup(&decoder);
  return(1);
}

/** HandleKeys() *********************************************/
/** Key handler.                                            **/
/*************************************************************/
void HandleKeys(unsigned int Key)
{
  if(InMenu||CPU.Trace) return;
#if 0
  if(Key&CON_RELEASE)
    switch(Key&CON_KEYCODE)
    {
      case XK_F9:
      case XK_Page_Up:
        if(FastForward)
        {
          X11SetEffects(UseEffects);
          UPeriod=FastForward;
          FastForward=0;
        }
        break;

      case 'q':
      case 'Q':          KBD_RES(KBD_ON);KeyReady=1;break;
      case XK_F1:        KBD_RES(KBD_F1);KeyReady=1;break;
      case XK_F2:        KBD_RES(KBD_F2);KeyReady=1;break;
      case XK_F3:        KBD_RES(KBD_F3);KeyReady=1;break;
      case XK_F4:        KBD_RES(KBD_F4);KeyReady=1;break;
      case XK_F5:        KBD_RES(KBD_F5);KeyReady=1;break;
      case XK_KP_Enter:
      case XK_Return:    KBD_RES(KBD_ENTER);KeyReady=1;break;
      case XK_Shift_L:
      case XK_Shift_R:   KBD_RES(KBD_2ND);KeyReady=1;break;
      case XK_Escape:    KBD_RES(KBD_EXIT);KeyReady=1;break;
      case XK_Left:      KBD_RES(KBD_LEFT);KeyReady=1;break;
      case XK_Right:     KBD_RES(KBD_RIGHT);KeyReady=1;break;
      case XK_Up:        KBD_RES(KBD_UP);KeyReady=1;break;
      case XK_Down:      KBD_RES(KBD_DOWN);KeyReady=1;break;
      case XK_greater:   KBD_RES(KBD_STO);KeyReady=1;break;
      case XK_Delete:
      case XK_BackSpace: KBD_RES(KBD_DEL);KeyReady=1;break;
      case XK_Home:      KBD_RES(KBD_CLEAR);KeyReady=1;break;
      case XK_Alt_L:
      case XK_Alt_R:     KBD_RES(KBD_ALPHA);KeyReady=1;break;
      default:
        Key&=CON_KEYCODE;
        if((Key>=' ')&&(Key<0x80)) { KBD_RES(Key);KeyReady=1; }
        break; 
    }
  else
    switch(Key&CON_KEYCODE)
    {
#ifdef DEBUG
      case XK_Tab:      CPU.Trace=1;break;
#endif
      case XK_F6:        LoadSTA("DEFAULT.STA");break;
      case XK_F7:        SaveSTA("DEFAULT.STA");break;
      case XK_F8:
        UseEffects^=Key&CON_ALT? EFF_SOFTEN:EFF_TVLINES;
        X11SetEffects(UseEffects);
        break;
      case XK_F9:
      case XK_Page_Up:
        if(!FastForward)
        {
          X11SetEffects(UseEffects&~EFF_SYNC);
          FastForward=UPeriod;
          UPeriod=10;
        }
        break;

      case 'q':
      case 'Q':          KBD_SET(KBD_ON);KeyReady=1;break;
      case XK_F11:       ResetTI85(Mode);break;
      case XK_F12:       ExitNow=1;break;

      case XK_F1:        KBD_SET(KBD_F1);KeyReady=1;break;
      case XK_F2:        KBD_SET(KBD_F2);KeyReady=1;break;
      case XK_F3:        KBD_SET(KBD_F3);KeyReady=1;break;
      case XK_F4:        KBD_SET(KBD_F4);KeyReady=1;break;
      case XK_F5:        KBD_SET(KBD_F5);KeyReady=1;break;
      case XK_KP_Enter:
      case XK_Return:    KBD_SET(KBD_ENTER);KeyReady=1;break;
      case XK_Shift_L:
      case XK_Shift_R:   KBD_SET(KBD_2ND);KeyReady=1;break;
      case XK_Escape:    KBD_SET(KBD_EXIT);KeyReady=1;break;
      case XK_Left:      KBD_SET(KBD_LEFT);KeyReady=1;break;
      case XK_Right:     KBD_SET(KBD_RIGHT);KeyReady=1;break;
      case XK_Up:        KBD_SET(KBD_UP);KeyReady=1;break;
      case XK_Down:      KBD_SET(KBD_DOWN);KeyReady=1;break;
      case XK_greater:   KBD_SET(KBD_STO);KeyReady=1;break;
      case XK_Delete:
      case XK_BackSpace: KBD_SET(KBD_DEL);KeyReady=1;break;
      case XK_Home:      KBD_SET(KBD_CLEAR);KeyReady=1;break;
      case XK_Alt_L:
      case XK_Alt_R:     KBD_SET(KBD_ALPHA);KeyReady=1;break;
      default:
        Key&=CON_KEYCODE;
        if((Key>=' ')&&(Key<0x80)) { KBD_SET(Key);KeyReady=1; }
        break; 
    }
#endif
}

/** HandleMouse() *********************************************/
/** Mouse click/unclick handler.                             **/
/*************************************************************/
void HandleMouse(int X, int Y, int State)
{
    int J;
    //int Flags = State ? 0:CON_RELEASE;
    for(J=0;TouchMap[J].W;++J)
      if((X>=TouchMap[J].X)&&(Y>=TouchMap[J].Y))
        if((X<TouchMap[J].X+TouchMap[J].W)&&(Y<TouchMap[J].Y+TouchMap[J].H))
        {
          if (State)
            KBD_SET(TouchMap[J].KeyCode);
          else
            KBD_RES(TouchMap[J].KeyCode);
          break;
        }
}

/** Common.h *************************************************/
/** Common display drivers.                                 **/
/*************************************************************/
#include "../Common.h"


