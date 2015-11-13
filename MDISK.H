#ifndef __MDISK_H__
  #define __MDISK_H__

  #define  INCL_NOPMAPI
  #define  INCL_DOS
  #include <os2.h>

  #define  CPU                0
  #define  DISK1              0
  #define  DISK2              1

  /* from iostone.c */
  #define  BFLUSH_FILE_SIZE (512L*1024L)   /*size of files used to flush buffers*/
  #define  NBFLUSH_FILES    8              /*number of files used to flush buffers*/
  #define  NBLOCKSIZES      9              /*number of different block sizes*/
  #define  SEED             34710373L      /*random number generator seed*/
  #define  CONST            500000L        /*iostone normalization constant*/
  #define  ITER             4              /*number of iterations of the code*/
  #define  BUFFERSIZE       (16L*1024L)    /*size of temporary buffer*/
  #define  NSETS            4              /*number of sets of files*/
  #define  SET_SIZE         99             /*number of files in each set*/
  #define  FNAMELEN         8              /*maximum file name length*/

  struct _CpuThdVars
    {
      TID   tid;
      HEV   hevReady,
            hevDone;
      ULONG ulCount;
      INT   iExitCode;
    };
  typedef struct _CpuThdVars CTV, *PCTV;

  struct _DskThdVars
    {
      ULONG ulTask;
      TID   tid;
      HEV   hevReady,
            hevDone;
      CHAR  szDrive[ 8 ];
      CHAR  szDir[ 16 ];
      ULONG ulCount;
      INT   iExitCode;

      /* globals from IOStone */
      CHAR  tmp[ FNAMELEN ];                    /*a temporary string*/
      CHAR  *files[ NSETS ][ SET_SIZE ];        /*array of file names*/
      CHAR  *buf_flush_files[ NBFLUSH_FILES ];  /*array of names of files to*/
                                                  /*flush system buffers*/
      CHAR  buffer[ BUFFERSIZE ];               /*a temporary buffer*/

      LONG  nbytes;                         /*number of bytes transfered*/
      INT   fd;                             /*file descriptor*/
      LONG  bsize[ NBLOCKSIZES ];           /*array for different block sizes*/
      INT   bfreq[ NBLOCKSIZES ];           /*number of accesses for each block*/
    };
  typedef struct _DskThdVars DTV, *PDTV;

  /* threads */
  VOID APIENTRY ThdDskPerf( ULONG ulTask );
  VOID APIENTRY ThdCpuPerf( ULONG ulTask );

  /* function prototypes */
  void init( PDTV pDisk );
  void removefiles( PDTV pDisk );

  /* global vars */
  extern HEV                hevGo;

#endif  /* __MDISK_H__ */





