
#define  INCL_DOS
#define  INCL_NOPMAPI
#include <os2.h>

#include "MDisk.h"

#include <stdio.h>
#include <time.h>
#include <dir.h>
#include <alloc.h>

#define  STACKSIZE       8192

PCTV            pCpu;
PDTV            pDsk[ 2 ];
time_t          tmStartTime,
                tmEndTime;
HEV             hevGo,
                hevTimer;

/* local functions */
int  IsValidDrive( char * szDrive );
void StartCpuThd( void );
void StartDskThd( ULONG ulTask );
void CreateFiles( ULONG ulTask );
void StartTest( void );
void EndTest( void );
void DeleteFiles( ULONG ulTask );
void Cleanup( INT rc );


void main( void )
{
  HTIMER   hTimer;
  ULONG    ulRunTime,
           ulDiskOps,
           ulCpuBasePerf,
           ulTestPerf;

  /* ------------------------------------------------------------- */
  /* display program logo                                          */
  /* ------------------------------------------------------------- */
  printf( "\n\n" );
  printf( "    MDisk - Multitasking Disk Subsystem Performance\n" );
  printf( "                Version 1.00  12/93\n\n" );

  /* allocate thread data structures */
  if( ( pCpu = (PCTV) malloc( sizeof( CTV ) ) ) == NULL )
    {
      printf("Not enough memory to allocate buffer\n");
      DosExit( 1, 1 );  /* terminate program if out of memory */
    }
  if( ( pDsk[ DISK1 ] = (PDTV) malloc( sizeof( DTV ) ) ) == NULL )
    {
      printf("Not enough memory to allocate buffer\n");
      DosExit( 1, 1 );  /* terminate program if out of memory */
    }
  if( ( pDsk[ DISK2 ] = (PDTV) malloc( sizeof( DTV ) ) ) == NULL )
    {
      printf("Not enough memory to allocate buffer\n");
      DosExit( 1, 1 );  /* terminate program if out of memory */
    }

  /* ------------------------------------------------------------- */
  /* get input parameters
  /* ------------------------------------------------------------- */
  printf( "\tEnter 1st drive to test: " );
  scanf( "%s", &pDsk[ DISK1 ]->szDrive );
  if( ! IsValidDrive( pDsk[ DISK1 ]->szDrive ) )
    {
      DosExit( 1, 1 );
    }

  printf( "\tEnter 2nd drive to test: " );
  scanf( "%s", &pDsk[ DISK2 ]->szDrive );
  if( ! IsValidDrive( pDsk[ DISK2 ]->szDrive ) )
    {
      DosExit( 1, 1 );
    }
  printf( "\n" );

  /* create semaphores */
  DosCreateEventSem( NULL, &hevGo,    DC_SEM_SHARED, 0 );
  DosCreateEventSem( NULL, &hevTimer, DC_SEM_SHARED, 0 );
  DosCreateEventSem( NULL, &pCpu->hevReady, DC_SEM_SHARED, 0 );
  DosCreateEventSem( NULL, &pCpu->hevDone,  DC_SEM_SHARED, 0 );
  DosCreateEventSem( NULL, &pDsk[ DISK1 ]->hevReady, DC_SEM_SHARED, 0 );
  DosCreateEventSem( NULL, &pDsk[ DISK1 ]->hevDone,  DC_SEM_SHARED, 0 );
  DosCreateEventSem( NULL, &pDsk[ DISK2 ]->hevReady, DC_SEM_SHARED, 0 );
  DosCreateEventSem( NULL, &pDsk[ DISK2 ]->hevDone,  DC_SEM_SHARED, 0 );

  /* test baseline cpu perf */
  StartCpuThd();

  DosWaitEventSem( pCpu->hevReady, SEM_INDEFINITE_WAIT );

  DosAsyncTimer( 10000, (HSEM) hevTimer, &hTimer );

  StartTest();

  DosWaitEventSem( hevTimer, SEM_INDEFINITE_WAIT );

  EndTest();

  ulCpuBasePerf = pCpu->ulCount / ( tmEndTime - tmStartTime );

  /* initialize test files */
  CreateFiles( DISK1 );
  CreateFiles( DISK2 );

  /* print headings */
  printf( "            Drive   MultiStones     CPU Usage\n" );

  /* test disk 1 by itselt */
  StartCpuThd();
  StartDskThd( DISK1 );

  DosWaitEventSem( pCpu->hevReady, SEM_INDEFINITE_WAIT );
  DosWaitEventSem( pDsk[ DISK1 ]->hevReady, SEM_INDEFINITE_WAIT );

  StartTest();

  DosWaitEventSem( pDsk[ DISK1 ]->hevDone, SEM_INDEFINITE_WAIT );

  EndTest();

  if( pDsk[ DISK1 ]->iExitCode != 0 )
    Cleanup( 1 );

  ulDiskOps  = pDsk[ DISK1 ]->ulCount;
  ulRunTime  = tmEndTime - tmStartTime;
  ulTestPerf = pCpu->ulCount / ulRunTime;

  printf( "           %4s        %6d         %3d %%\n",
          pDsk[ DISK1 ]->szDrive,
          ( ulDiskOps / ulRunTime ),
          100 - (( ulTestPerf * 100 ) / ulCpuBasePerf ) );

  /* test disk 2 by itself */
  StartCpuThd();
  StartDskThd( DISK2 );

  DosWaitEventSem( pCpu->hevReady, SEM_INDEFINITE_WAIT );
  DosWaitEventSem( pDsk[ DISK2 ]->hevReady, SEM_INDEFINITE_WAIT );

  StartTest();

  DosWaitEventSem( pDsk[ DISK2 ]->hevDone, SEM_INDEFINITE_WAIT );

  EndTest();

  if( pDsk[ DISK2 ]->iExitCode != 0 )
    Cleanup( 1 );

  ulDiskOps  = pDsk[ DISK2 ]->ulCount;
  ulRunTime  = tmEndTime - tmStartTime;
  ulTestPerf = pCpu->ulCount / ulRunTime;

  printf( "           %4s        %6d         %3d %%\n",
          pDsk[ DISK2 ]->szDrive,
          ( ulDiskOps / ulRunTime ),
          100 - (( ulTestPerf * 100 ) / ulCpuBasePerf ) );

  /* test both disks */
  StartCpuThd();
  StartDskThd( DISK1 );
  StartDskThd( DISK2 );

  DosWaitEventSem( pCpu->hevReady, SEM_INDEFINITE_WAIT );
  DosWaitEventSem( pDsk[ DISK1 ]->hevReady, SEM_INDEFINITE_WAIT );
  DosWaitEventSem( pDsk[ DISK2 ]->hevReady, SEM_INDEFINITE_WAIT );

  StartTest();

  DosWaitEventSem( pDsk[ DISK1 ]->hevDone, SEM_INDEFINITE_WAIT );
  DosWaitEventSem( pDsk[ DISK2 ]->hevDone, SEM_INDEFINITE_WAIT );

  EndTest();

  if( ( pDsk[ DISK1 ]->iExitCode != 0 ) || ( pDsk[ DISK2 ]->iExitCode != 0 ) )
    Cleanup( 1 );

  ulDiskOps  = pDsk[ DISK1 ]->ulCount + pDsk[ DISK2 ]->ulCount;
  ulRunTime  = tmEndTime - tmStartTime;
  ulTestPerf = pCpu->ulCount / ulRunTime;

  printf( "           %4s        %6d         %3d %%\n",
          "Both",
          ( ulDiskOps / ulRunTime ),
          100 - (( ulTestPerf * 100 ) / ulCpuBasePerf ) );

  printf( "\n\n" );
  Cleanup( 0 );
}


int IsValidDrive( char * Drive )
{
  if( ( Drive[ 0 ] >= 'c' ) && ( Drive[ 0 ] <= 'z' ) )
    Drive[ 0 ] -= 0x20;

  if( ( Drive[ 0 ] < 'C' ) || ( Drive[ 0 ] > 'Z' ) )
    {
      printf( "Invalid drive spec\n" );
      return( FALSE );
    }

  Drive[ 1 ] = ':';
  Drive[ 2 ] = 0;

  return( TRUE );
}

void StartDskThd( ULONG ulTask )
{
  ULONG ulPostCount;
  PSZ   pszDir = "/MDisk ";
  INT   i = 0;

  pDsk[ ulTask ]->ulTask       = ulTask;
  pDsk[ ulTask ]->ulCount      = 0;
  pDsk[ ulTask ]->iExitCode    = 0;

  while( pszDir[ i ] )
    pDsk[ ulTask ]->szDir[ i ] = pszDir[ i++ ];

  DosResetEventSem( pDsk[ ulTask ]->hevReady, &ulPostCount );
  DosResetEventSem( pDsk[ ulTask ]->hevDone,  &ulPostCount );

  DosCreateThread( &pDsk[ ulTask ]->tid,
                   ThdDskPerf,
                   (ULONG)pDsk[ ulTask ],
                   0,
                   STACKSIZE );
}


void StartCpuThd( void )
{
  ULONG ulPostCount;

  pCpu->ulCount      = 0;
  pCpu->iExitCode    = 0;

  DosResetEventSem( pCpu->hevReady, &ulPostCount );
  DosResetEventSem( pCpu->hevDone,  &ulPostCount );
  DosResetEventSem( hevGo,          &ulPostCount );
  DosResetEventSem( hevTimer,       &ulPostCount );

  DosCreateThread( &pCpu->tid,
                   ThdCpuPerf,
                   (ULONG)pCpu,
                   0,
                   STACKSIZE );
}


void StartTest( void )
{
  tmStartTime = time( NULL );
  DosPostEventSem( hevGo );
}


void EndTest( void )
{
  tmEndTime = time( NULL );

  DosPostEventSem( pCpu->hevDone );

  /* this ensures that the cpu thd has time to end before */
  /* it gets restarted for the next test                  */
  DosSleep( 1000 );
}


void CreateFiles( ULONG ulTask )
{
  char szDir[ 16 ] = "/MDisk ";

  szDir[ 6 ] = (char)( ulTask + 0x30 );
  szDir[ 7 ] = 0;

  setdisk( (int)( pDsk[ ulTask ]->szDrive[ 0 ] - 'A' ) );
  mkdir( szDir );
  chdir( szDir );

  /* create test files */
  init( pDsk[ ulTask ] );
}


void DeleteFiles( ULONG ulTask )
{
  char szDir[ 16 ] = "/MDisk ";

  szDir[ 6 ] = (char)( ulTask + 0x30 );
  szDir[ 7 ] = 0;

  setdisk( (int)( pDsk[ ulTask ]->szDrive[ 0 ] - 'A' ) );
  chdir( szDir );

  /*remove files*/
  removefiles( pDsk[ ulTask ] );

  chdir( "\\" );
  rmdir( szDir );
}


void Cleanup( INT rc )
{
  if( rc )
    {
      DosBeep( 1000, 500 );
      printf( "Fatal error - test aborted\n" );
    }

  DeleteFiles( DISK1 );
  DeleteFiles( DISK2 );

  free( pCpu );
  free( pDsk[ DISK1 ] );
  free( pDsk[ DISK2 ] );

  DosExit( 1, rc );
}


VOID APIENTRY ThdCpuPerf( ULONG ulTask )
{
  INT   i;
  ULONG ulPostCt = 0;
  PCTV  pCpu;

  pCpu = (PCTV) ulTask;

  DosPostEventSem( pCpu->hevReady );
  DosWaitEventSem( hevGo, SEM_INDEFINITE_WAIT );

  DosSetPriority( PRTYS_THREAD, PRTYC_IDLETIME, PRTYD_MAXIMUM, 0 );

  while( ! ulPostCt )
    {
      for( i = 1; i > 0; i++ );
      pCpu->ulCount++;
      DosQueryEventSem( pCpu->hevDone, &ulPostCt );
    }

  DosExit( 0, 0 );
}


