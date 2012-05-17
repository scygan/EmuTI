/** AlmostTI: portable TI calcs emulator *********************/
/**                                                         **/
/**                        Maemo.c                          **/
/**                                                         **/
/** This file contains Maemo-dependent subroutines and      **/
/** drivers. It includes screen drivers via Common.h.       **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 2009                      **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#include "TI85.h"
#include "Console.h"
#include "EMULib.h"

#include <string.h>
#include <stdio.h>
#include <png.h>

#define DBUS_SERVICE "com.fms.ati85"

#define WIDTH   (3*(TI83_FAMILY? 96:128)) /* Buffer width    */
#define HEIGHT  (3*64)                    /* Buffer height   */
#define XOFFSET 54                        /* Display offset  */
#define YOFFSET (48+(128*3-WIDTH)/2)      /* Display offset  */

#define MENU_FREEZE  0x1002
#define MENU_RESTORE 0x1003
#define MENU_DEBUG   0x1005
#define MENU_RESET   0x1006
#define MENU_ABOUT   0x1007
#define MENU_TVLINES 0x1009
#define MENU_SOFTEN  0x100A
#define MENU_SHOWFPS 0x100C

#define STATE_DIR    "/home/user/.ati85/"

extern byte StatusPort;

/* Combination of EFF_* bits */
int UseEffects  = EFF_SAVECPU|EFF_SYNC|EFF_FULLSCR;

int InMenu;                /* 1: In MenuTI85(), ignore keys  */
int SyncFreq    = 50;      /* Sync frequency (0=sync off)    */
int FastForward;           /* Fast-forwarded UPeriod backup  */
byte CurKey;               /* Last key pressed on touchscr.  */
Image ScrImage;            /* TI85 display image buffer      */
Image OutImage;            /* Overall display image buffer   */ 
char RAMNameBuf[256];      /* State file name buffer         */
pixel XPal[2];

const char *Title = "AlmostTI-Maemo 1.3"; /* Program version */

void HandleKeys(unsigned int Key);
int LoadPNG(Image *Img,const char *FileName);

/** TouchMap[] ***********************************************/
/** This map translates on-screen rectangles into keys.     **/
/*************************************************************/
struct
{
  byte KeyCode;
  int X,Y,W,H;
} TouchMap[] =
{
  { KBD_RIGHT,  380,45,43,28 },
  { KBD_LEFT,   380,163,43,28 },
  { KBD_UP,     352,89,25,58 },
  { KBD_DOWN,   427,89,25,58 },

  { KBD_F5,     291,43,25,65 },
  { KBD_F4,     291,125,25,65 },
  { KBD_F3,     291,207,25,65 },
  { KBD_F2,     291,289,25,65 },
  { KBD_F1,     291,371,25,65 },

  { KBD_CLEAR,  467,40,30,67 },
  { KBD_POWER,  511,40,30,67 },
  { KBD_DIV,555,40,30,67 },
  { KBD_MUL,    599,40,30,67 },
  { KBD_MINUS,  643,40,30,67 },
  { KBD_PLUS,   687,40,30,67 },
  { KBD_ENTER,  731,40,38,67 },

  { KBD_CUSTOM, 467,123,30,67 },
  { KBD_TAN,    511,123,30,67 },
  { KBD_RPARENT,555,123,30,67 },
  { KBD_9,      599,123,30,67 },
  { KBD_6,      643,123,30,67 },
  { KBD_3,      687,123,30,67 },
  { KBD_SIGN,   731,123,30,67 },
  
  { KBD_MORE,   379,206,30,67 },
  { KBD_DEL,    423,206,30,67 },
  { KBD_PRGM,   467,206,30,67 },
  { KBD_COS,    511,206,30,67 },
  { KBD_LPARENT,555,206,30,67 },
  { KBD_8,      599,206,30,67 },
  { KBD_5,      643,206,30,67 },
  { KBD_2,      687,206,30,67 },
  { KBD_DOT,    731,206,30,67 },

  { KBD_EXIT,   379,289,30,67 },
  { KBD_XVAR,   423,289,30,67 },
  { KBD_STAT,   467,289,30,67 },
  { KBD_SIN,    511,289,30,67 },
  { KBD_EE,     555,289,30,67 },
  { KBD_7,      599,289,30,67 },
  { KBD_4,      643,289,30,67 },
  { KBD_1,      687,289,30,67 },
  { KBD_0,      731,289,30,67 },

  { KBD_2ND,    379,372,30,67 },
  { KBD_ALPHA,  423,372,30,67 },
  { KBD_GRAPH,  467,372,30,67 },
  { KBD_LOG,    511,372,30,67 },
  { KBD_LN,     555,372,30,67 },
  { KBD_SQR,    599,372,30,67 },
  { KBD_COMMA,  643,372,30,67 },
  { KBD_STO,    687,372,30,67 },
  { KBD_ON,     731,372,30,67 },

  { 0,0,0,0,0 }
};

/** ATI85.c **************************************************/
/** The normal, command-line main() is taken from here.     **/
/*************************************************************/
#define main MyMain
#include "ATI85.c"
#undef main

/** main() ***************************************************/
/** Execution starts HERE and not in the ATI85.c main()!    **/
/*************************************************************/
int main(int argc,char *argv[])
{
  Mode    = ATI_TI85;
  UPeriod = 35;
  ARGC    = argc;
  ARGV    = argv;

#ifdef DEBUG
  CPU.Trap  = 0xFFFF;
  CPU.Trace = 0;
#endif

  if(argc>1) return(MyMain(argc,argv));
  else
  {
    /* Initialize hardware */
    if(!InitMachine()) return(1);
    /* Run emulation, then clean up */
    StartTI85(0);
    TrashTI85();
    /* Release hardware */
    TrashMachine();
  }

  /* Done */
  return(0);
}

/** InitMachine() ********************************************/
/** Allocate resources needed by machine-dependent code.    **/
/*************************************************************/
int InitMachine(void)
{
  int M;

  /* Initialize variables */
  InMenu        = 0;
  FastForward   = 0;
  OutImage.Data = 0;
  CurKey        = 0xFF;

  /* Set visual effects */
  GTKSetEffects(UseEffects);

  /* Initialize system resources */
  InitMaemo(Title,800,480,DBUS_SERVICE);

  /* Read and activate settings */
  UPeriod       = GTKGetInteger("draw-frames",UPeriod);
  Mode          = GTKGetInteger("hardware-mode",Mode);

  // @@@ NOT USED IN THIS EMULATOR
  // UseEffects = GTKGetInteger("video-effects",UseEffects);
  // GTKSetEffects(UseEffects);

  /* Figure out configuration */
  for(M=0;Config[M].ROMFile&&((Mode&ATI_MODEL)!=Config[M].Model);++M);

  /* Default to the first model if problem */
  if(!Config[M].ROMFile) Mode=(Mode&~ATI_MODEL)|Config[M=0].Model;

  /* Create state file name */
  strcpy(RAMNameBuf,STATE_DIR);
  strcat(RAMNameBuf,Config[M].RAMFile);
  RAMFile=RAMNameBuf;

  /* Create overall image buffer */
  if(!NewImage(&OutImage,800,480)) return(0);

  /* Crop TI85 display image buffer, rotated 90o */
  CropImage(&ScrImage,&OutImage,XOFFSET,YOFFSET,HEIGHT,WIDTH);

  /* Initialize video to overall image */
  SetVideo(&OutImage,0,0,800,480);

  /* Set initial colors */
  XPal[0] = PIXEL(255,255,255);
  XPal[1] = PIXEL(0,0,0);

  /* Attach keyboard handler */
  SetKeyHandler(HandleKeys);

  /* Add menus */
  GTKAddMenu("Freeze State",MENU_FREEZE);
  GTKAddMenu("Restore State",MENU_RESTORE);
#ifdef DEBUG
  GTKAddMenu("Debugger",MENU_DEBUG);
  GTKAddMenu(0,0);
#endif
  GTKAddMenu("Scanlines",MENU_TVLINES);
  GTKAddMenu("Interpolation",MENU_SOFTEN);
  GTKAddMenu("Framerate",MENU_SHOWFPS);
  GTKAddMenu(0,0);
  GTKAddMenu("About",MENU_ABOUT);
  GTKAddMenu("Reset",MENU_RESET);

  /* Initialize sync timer if needed */
  if((SyncFreq>0)&&!SetSyncTimer(SyncFreq*UPeriod/100)) SyncFreq=0;

  /* Done */
  return(1);
}

/** TrashMachine() *******************************************/
/** Deallocate all resources taken by InitMachine().        **/
/*************************************************************/
void TrashMachine(void)
{
  /* Cancel fast-forwarding */
  if(FastForward)
  {
    GTKSetEffects(UseEffects);
    UPeriod=FastForward;
    FastForward=0;
  }

  /* Save settings */
  GTKSetInteger("draw-frames",UPeriod);
  GTKSetInteger("hardware-mode",Mode);
// @@@ NOT USED IN THIS EMULATOR
//  GTKSetInteger("video-effects",UseEffects);

  FreeImage(&OutImage);
  TrashAudio();
  TrashMaemo();
}

/** SetColor() ***********************************************/
/** Allocate new color.                                     **/
/*************************************************************/
void SetColor(byte N,byte R,byte G,byte B)
{
  /* Set requested color */
  XPal[N&1]=PIXEL(R,G,B);
}

/** PutImage() ***********************************************/
/** Put an image on the screen.                             **/
/*************************************************************/
void PutImage(void)
{
  /* Show rendered screen buffer */
  ShowVideo();
}

/** ShowBackdrop() *******************************************/
/** Show backdrop image with calculator faceplate.          **/ 
/*************************************************************/
int ShowBackdrop(const char *FileName)
{
  char *P;
  int J;

  /* Load background image */
  if(!(P=malloc((ProgDir? strlen(ProgDir):8)+16)))
    J=0;
  else
  {
    strcpy(P,ProgDir? ProgDir:".");
    strcat(P,"/");
    strcat(P,FileName);
    J=LoadPNG(&OutImage,P);
    free(P);
  }

  return(J);
}

/** Keypad() *************************************************/
/** Poll the keyboard.                                      **/ 
/*************************************************************/
byte Keypad(void)
{
  unsigned int J,X,Y;
  byte NewKey;

  GTKProcessEvents();

  J      = GetMouse();
  NewKey = 0xFF;

  if(J&0xC0000000)
  {
    X = J&0xFFFF;
    Y = (J>>16)&0x3FFF;

    for(J=0;TouchMap[J].W;++J)
      if((X>=TouchMap[J].X)&&(Y>=TouchMap[J].Y))
        if((X<TouchMap[J].X+TouchMap[J].W)&&(Y<TouchMap[J].Y+TouchMap[J].H))
        {
          NewKey=TouchMap[J].KeyCode;
          break;
        }
  }

  if(NewKey!=CurKey)
  {
    if(CurKey!=0xFF) KBD_RES(CurKey);
    if(NewKey!=0xFF) KBD_SET(NewKey);
    CurKey = NewKey;
  }

  return(IS_KBD(KBD_ON));
}

/** MaemoMenu() **********************************************/
/** Maemo specific menu to select a new calculator mode.    **/
/*************************************************************/
void MaemoMenu(void)
{
  unsigned int J,X,Y,M;
  int OldMode,NewMode;
  char *P;

  /* No menu image */
  ClearImage(&OutImage,PIXEL(0,0,0));

  /* Load menu image */
  if(P=malloc((ProgDir? strlen(ProgDir):8)+16))
  {
    strcpy(P,ProgDir? ProgDir:".");
    strcat(P,"/Menu.png");
    LoadPNG(&OutImage,P);
    free(P);
  }

  /* Mark up icons with rectangles for now */
//  for(Y=0;Y<OutImage.H;Y+=OutImage.H/3)
//    for(X=0;X<OutImage.W;X+=OutImage.H/3)
//      IMGDrawRect(&OutImage,X+10,Y+10,OutImage.H/3-20,OutImage.H/3-20,PIXEL(255,255,255));

  /* Show menu image */
  ShowVideo();

  /* Find current calc model */
  for(M=0;Config[M].ROMFile&&((Mode&ATI_MODEL)!=Config[M].Model);++M);

  /* Memorize current modes */
  OldMode = Mode;
  X       = M;

  do
  {
    /* Wait for a mouse click or quit */
    for(J=0;VideoImg&&!ExitNow&&!(J=WaitKeyOrMouse());)
      if((GetKey()&CON_KEYCODE)==GDK_F6) break;

    /* If menu selection has been successful... */
    if(J)
    {
      /* Compute item index */
      Y = J&0xFFFF;
      X = OutImage.H-1-((J>>16)&0x3FFF);
      J = (3*X/OutImage.H)+3*(3*Y/OutImage.H);

      /* Depending on selected item... */
      switch(J)
      {
        case 12: /* RESET */
          ResetTI85(Mode);
          X=M;J=0;
          break;

        case 14: /* EXIT */
          ExitNow=1;
          X=M;J=0;
          break;

        default:
          /* Find appropriate configuration */
          for(X=0;(X!=J)&&Config[X].ROMFile;++X);
          /* If same configuration, exit */
          if(X==M) { J=0;break; }
          /* If new configuration has been found... */
          /* @@@ TEMPORARY: DISABLE TI83+SE and newer calcs for now */
          if(Config[X].ROMFile&&(Config[X].Model<ATI_TI83SE))
          {
            /* Save current hardware state */      
            if(RAMFile) SaveSTA(RAMFile);
            /* Try resetting emulator to the new hardware model */
            NewMode = (Mode&~ATI_MODEL)|Config[X].Model;
            if(ResetTI85(NewMode)==NewMode) J=0;
          }
          break;
      }
    }
  }
  while(J);

  /* If hardware model has been changed... */
  if((Mode^OldMode)&ATI_MODEL)
  {
    /* Load state for the new model */
    if(Config[X].RAMFile)
    {
      strcpy(RAMNameBuf,STATE_DIR);
      strcat(RAMNameBuf,Config[X].RAMFile);
      RAMFile=RAMNameBuf;
      LoadSTA(RAMFile);
    }
  }
  else
  {
    /* Reload backdrop image for the old model */
    if(Config[M].Backdrop) ShowBackdrop(Config[M].Backdrop);
  }
}

/** HandleKeys() *********************************************/
/** Key handler.                                            **/
/*************************************************************/
void HandleKeys(unsigned int Key)
{
  /* When F12 or ESCAPE pressed, exit no matter what */
  if(((Key&CON_KEYCODE)==GDK_F12)||((Key&CON_KEYCODE)==GDK_Escape))
  { ExitNow=1;return; }

  if(InMenu||CPU.Trace) return;

  if(Key&CON_RELEASE)
    switch(Key&CON_KEYCODE)
    {
    }
  else
    switch(Key&CON_KEYCODE)
    {
      case GDK_F6:
        /* Run touch menu */
        InMenu=1;
        MaemoMenu();
        InMenu=0;
        break;
    }
}

/** Common.h *************************************************/
/** Common display drivers.                                 **/
/*************************************************************/
#include "Common.h"
