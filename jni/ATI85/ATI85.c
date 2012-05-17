/** AlmostTI: portable TI calcs emulator *********************/
/**                                                         **/
/**                          ATI85.c                        **/
/**                                                         **/
/** This file contains generic main() procedure statrting   **/
/** the emulation.                                          **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1994-2009                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/

#include "EMULib.h"
#include "TI85.h"
#include "Help.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

char *Options[]=
{ 
  "ti82","ti83","ti83+","ti83se","ti84","ti84+","ti84se","ti85","ti86",
  "verbose","uperiod","help","home","link","trap",
  "shm","noshm","saver","nosaver",
  0
};

extern const char *Title;/* Program title                       */
extern int   UseEffects; /* Platform-dependent effects          */
extern int   SyncScreen; /* Sync screen updates (#ifdef MSDOS)  */
extern int   FullScreen; /* Use 320x240 VGA (#ifdef MSDOS)      */

int main(int argc,char *argv[])
{
  char *RAMName=0;
  int N,J,I;

#ifdef DEBUG
  CPU.Trap  = 0xFFFF;
  CPU.Trace = 0;
#endif

#if defined(UNIX) || defined(MAEMO)
  ARGC = argc;
  ARGV = argv;
#endif

#if defined(MSDOS) || defined(WINDOWS)
  /* No separate console, so no messages */
  Verbose=0;
#else
  Verbose=5;
#endif

  for(N=1,I=0;N<argc;N++)
    if(*argv[N]!='-')
      switch(I++)
      {
        case 0: RAMName=argv[N];break;
        default: printf("%s: Excessive filename '%s'\n",argv[0],argv[N]);
      }
    else
    {    
      for(J=0;Options[J];J++)
        if(!strcmp(argv[N]+1,Options[J])) break;
      switch(J)
      {
        case 0:  Mode=(Mode&~ATI_MODEL)|ATI_TI82;break;
        case 1:  Mode=(Mode&~ATI_MODEL)|ATI_TI83;break;
        case 2:  Mode=(Mode&~ATI_MODEL)|ATI_TI83P;break;
        case 3:  Mode=(Mode&~ATI_MODEL)|ATI_TI83SE;break;
        case 4:  Mode=(Mode&~ATI_MODEL)|ATI_TI84;break;
        case 5:  Mode=(Mode&~ATI_MODEL)|ATI_TI84P;break;
        case 6:  Mode=(Mode&~ATI_MODEL)|ATI_TI84SE;break;
        case 7:  Mode=(Mode&~ATI_MODEL)|ATI_TI85;break;
        case 8:  Mode=(Mode&~ATI_MODEL)|ATI_TI86;break;
        case 9:  N++;
                 if(N<argc) Verbose=atoi(argv[N]);
                 else printf("%s: No verbose level supplied\n",argv[0]);
                 break;
  	case 10: N++;
                 if(N>=argc)
                   printf("%s: No screen update period supplied\n",argv[0]);
                 else
                 {
                   J=atoi(argv[N]);
                   if((J>0)&&(J<20)) UPeriod=J;
                 }
                 break;
	case 11: printf("%s by Marat Fayzullin    (C)FMS 1994-2009\n",Title);
                 for(J=0;HelpText[J];J++) puts(HelpText[J]);
                 return(0);
                 break;
        case 13: N++;
                 if(N>=argc)
                   printf("%s: No peer address supplied\n",argv[0]);
                 else
                   LinkPeer=argv[N];
                 break;

#ifdef DEBUG
        case 14: N++;
                 if(N>=argc)
                   printf("%s: No trap address supplied\n",argv[0]);
                 else
                   if(!strcmp(argv[N],"now")) CPU.Trace=1;
                   else sscanf(argv[N],"%hX",&CPU.Trap);
                 break;
#endif /* DEBUG */

#ifdef UNIX
#ifdef MITSHM
        case 15: UseEffects|=EFF_MITSHM;break;
        case 16: UseEffects&=~EFF_MITSHM;break;
#endif
        case 17: UseEffects|=EFF_SAVECPU;break;
        case 18: UseEffects&=~EFF_SAVECPU;break;
#endif /* UNIX */

        default: printf("%s: Wrong option '%s'\n",argv[0],argv[N]);
      }
    }

  if(!InitMachine()) return(1);
  StartTI85(RAMName);
  TrashTI85();
  TrashMachine();
  return(0);
}
