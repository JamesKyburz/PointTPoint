/* CMS REPLACEMENT HISTORY, Element PTP.H*/
/* *4    15-SEP-2003 14:26:30 RD2JK "Reverted back to CRTL"*/
/* *3    15-SEP-2003 12:11:54 RD2JK "Replaced CRTL file i/o for windows with win32 api calls"*/
/* *2    29-JUL-2003 10:14:20 RD2JK ""*/
/* *1    25-JUL-2003 15:58:05 RD2JK "POINTTPOINT API HEADER FILE"*/
/* CMS REPLACEMENT HISTORY, Element PTP.H*/
/**************************************************************
 *                   Point to Point API
 *
 * Written         : James Kyburz 13-Feb-2002
 * Function        : API for sending and receiveing TCP/IP 
 *                   stream data. 
 *                   includes TCP/IP server ( gatekeeper )
 * Version         : 1.0
 * Modifications   :
 *
 *************************************************************/

#ifndef PTP_H
#define PTP_H

#define PTP_VER "1.0"

#ifdef WIN32
   #define V_M_S 0
   #define L_I_N_U_X 0
   #define W_I_N 1
   #define U_N_I_X 0
   #define M_A_C 0
#endif

#ifdef _MAC
   #define V_M_S 0
   #define L_I_N_U_X 0
   #define W_I_N 0
   #define U_N_I_X 0
   #define M_A_C 1
#endif

#ifdef __VMS_VER 
   #define V_M_S 1
   #define L_I_N_U_X 0
   #define W_I_N 0
   #define U_N_I_X 0
   #define M_A_C 0
#endif

#ifdef linux
   #define V_M_S 0
   #define L_I_N_U_X 1
   #define W_I_N 0
   #define U_N_I_X 0
   #define M_A_C 0
#endif

#ifdef UNIX
   #define V_M_S 0
   #define L_I_N_U_X 0
   #define W_I_N 0
   #define U_N_I_X 1
   #define M_A_C 0
#endif

#if V_M_S         
   #elif W_I_N
   #elif L_I_N_U_X
   #elif U_N_I_X
   #elif M_A_C
   #else #error "Platform not supported"
#endif

#if W_I_N
   #define PTP_IOCTL ioctlsocket
   #define PTPINPROGRESS WSAEINPROGRESS
   #define PTP_SET_ERROR(x) WSASetLastError(x)
   #ifndef PTP_IMANAGER
     #define PTP_IMANAGER 0x00
   #endif
   #define PTP_ERROR WSAGetLastError()
   #define PTP_STARTUP_LOG "C:/PTPStartupLog"
#elif V_M_S
   #define PTP_IOCTL ioctl
   #define PTPINPROGRESS EINPROGRESS
   #define PTP_IMANAGER 0x00
   #define PTP_STARTUP_LOG "SYS$LOGIN:PTPStartupLog"
   #define PTP_ERROR errno 
   #define PTP_SET_ERROR(x) errno = x
#else
   #define PTP_IOCTL ioctl
   #define PTPINPROGRESS EINPROGRESS
   #define PTP_IMANAGER 0x00
   #define PTP_STARTUP_LOG "PTPStartupLog.TXT"
   #define PTP_ERROR errno 
   #define PTP_SET_ERROR(x) errno = x
#endif

/* Windows compilation using VS 6.0 / VS.NET Link with wsock32.lib */ 

/* define error constants for all platforms */

#if W_I_N

  #define PTPECONNRESET                WSAECONNRESET
  #define PTPEINTR                     WSAEINTR
  #define PTPEWOULDBLOCK               WSAEWOULDBLOCK
  #define PTPEPERM                     EPERM
  #define PTPEMFILE                    WSAEMFILE
  #define PTPENFILE                    ENFILE
  #define PTPENOMEM                    ENOMEM
  #define PTPENOBUFS                   WSAENOBUFS  
  
#else
   
  #define PTPECONNRESET                ECONNRESET
  #define PTPEINTR                     EINTR
  #define PTPEWOULDBLOCK               EWOULDBLOCK
  #define PTPEPERM                     EPERM
  #define PTPEMFILE                    EMFILE
  #define PTPENFILE                    ENFILE
  #define PTPENOMEM                    ENOMEM
  #define PTPENOBUFS                   ENOBUFS  

#endif

#define PTP_OWN_HOST                 0x0100007F  /* 127.0.0.1 */
#define PTP_LOG_LEVEL_ERROR          0x00
#define PTP_LOG_LEVEL_INFORMATION    0x01
#define PTP_LOG_CODE                 0x00
#define PTP_LOG_LEVEL_NOTHING        0x05

#define PTP_PARAM_LINE               0xFF
#define PTP_TAB                      0x09
#define PTP_SERV_BACKLOG             0x01
#define PTP_MAX_MESSAGE_SIZE         0xFFFF
#define PTP_MAX_LOG_MESSAGE_SIZE     0x6D60
#define PTP_SD_BOTH                  0x02
#define PTP_EXIT_SUCCESS             0x00
#define PTP_EXIT_FAILURE             0x01

#define PTP_INTERFACE_TIMEOUT        0x02
#define PTP_INTERFACE_FAILURE        0x00
#define PTP_INTERFACE_SUCCESS        0xFFFFFFFF

#define PTP_MSG_FUNC_REPLIER         0x01
#define PTP_MSG_FUNC_REQUESTER       0x02
#define PTP_MSG_FUNC_SHUTDOWN        0x05
#define PTP_MSG_FUNC_SHUTDOWN_WAIT   0x06

#define PTP_ACK                      'Y'

#if PTP_IMANAGER
  #define PTP_CONSOLE                NULL
#else
  #define PTP_CONSOLE                stdout
#endif

#if W_I_N

#include <process.h>                /* define getpid()              */
#include <winsock2.h>               /* define BSD 4.x socket api    */
#include <winbase.h>                /* define winbase               */

#elif V_M_S

#include <unistd.h>                 /* define getpid()              */
#include <starlet.h>                /* define SYS$ routines         */
#include <in.h>                     /* define internet constants,   */
#include <stropts.h>                /* define IOCTL non blocking    */
#include <inet.h>                   /* define network address info  */
#include <netdb.h>                  /* define network database lib  */
#include <socket.h>                 /* define BSD 4.x socket api    */
#include <unixio.h>                 /* define unix i/o              */
#include <tcp.h>                    /* define tcp contants          */

#else

# include <netinet/in.h>            /*                              */
# include <netinet/tcp.h>           /*                              */
# include <arpa/inet.h>             /*                              */
# include <sys/types.h>             /*                              */
# include <sys/socket.h>            /*                              */

#endif

#include <math.h>                   /* define math functions        */
#include <time.h>                   /* define time                  */
#include <signal.h>                 /* define signal handling       */
#include <stdarg.h>                 /* define variable arguments    */
#include <stdio.h>                  /* define standard i/o          */
#include <stdlib.h>                 /* define standard library      */
#include <string.h>                 /* define string handling       */
#include <errno.h>                  /* define global error number   */        
#include <ctype.h>                  /* define character handling    */

typedef unsigned int uint;
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned long ulong;
typedef void privateVoid;
typedef short privateRtn;
typedef void PTPAPI;

#if W_I_N
   typedef unsigned int  PTP_SOCKET;
   typedef          int  PTP_ADDRLEN;
   typedef const    char PTP_OPTVAL;
#else
   typedef          int  PTP_SOCKET;
   typedef unsigned int  PTP_ADDRLEN;
   typedef const    int  PTP_OPTVAL;
#endif

typedef struct sockaddr         SOCKADDR;
typedef struct sockaddr_in      ISOCKET;
typedef struct hostent          HOSTENT;
typedef struct in_addr          INADDR;
typedef struct message          PTP_MESSAGE;
typedef struct tm               TM;

enum sendType
{
  PTP_SEND,
  PTP_RECEIVE
};

typedef enum sendType SEND_TYPE;

enum messageType
{
  PTP_NONE,
  PTP_REPLIER,
  PTP_REQUESTER
};

typedef enum messageType MESSAGE_TYPE;

enum messageStatus
{
  PTP_IDLE,
  PTP_BUSY
};

typedef enum messageStatus MESSAGE_STATUS;


/* Any amendments in ptpHeader will mean the API
   must be released everywhere and function
   zCreateHeader will have to be amended along
   with all the calls to it.
*/

union ptpHeader
{
  struct uHeader
  {
   uchar ptpIdentifier;
   uchar function;
   char cSize[0x08];
   uchar language;
   uchar encrypt;
   uchar nullTerminator;
  } u ;

  char cHeader[sizeof( struct uHeader )];

};

struct message 
{
   PTP_MESSAGE *nextQ;
   PTP_MESSAGE *prevQ;
   PTP_MESSAGE *pairQ;
   union ptpHeader header;
   MESSAGE_TYPE type;
   MESSAGE_STATUS status;
   short deleteFlag : 0x01;
   short validated : 0x01;
   short noSize : 0x01;
   short processedQuestion : 0x01;
   uint size;
   char *p;
   uint bytesReceived;
   int getSize;
   PTP_SOCKET channel;
};

long zPutGetData( 
        long *,
        FILE *,
        PTP_SOCKET *,
        char *,
        long *,
        const struct timeval *,
        const long ,
        const SEND_TYPE,
        const char * );

PTP_MESSAGE *zAddQ( 
        PTP_MESSAGE **,
        PTP_SOCKET );

privateVoid zWriteToLogger(
        const char *,
        const int,
        va_list );

privateVoid zPTPLog( 
        FILE *, 
        const char, 
        const char *, 
        const int, 
        const int, 
        const char *, 
        ... );

privateVoid zCleanup( 
        FILE *, 
        const char, 
        const char, 
        PTP_SOCKET *, 
        const char );

privateVoid zTime(    
        char *, 
        const char );

privateVoid zDelQ( 
        PTP_MESSAGE **, 
        PTP_MESSAGE * );

privateVoid zReceiveAnswer(
        long *, 
        PTP_SOCKET *,
        char *, 
        long *, 
        const long *, 
        const long *, 
        long * );

privateVoid zEncrypt( 
        uchar *, 
        int );

privateVoid zDecrypt( 
        uchar *, 
        int );

privateVoid zSend( 
        long *, 
        PTP_SOCKET *, 
        const char *, 
        const long *, 
        const long *, 
        const long *, 
        const long *, 
        const uchar ); 

privateVoid zWriteMessageToDisk( 
        long *,
        const char *,
        const char *,
        const long *,
        const long *,
        const long * );

privateVoid zRecv( 
        long *, 
        PTP_SOCKET *, 
        char *, 
        long *, 
        const long *, 
        const long *,
        long * );

privateVoid zSendFunction( 
        long *, 
        const char *, 
        const long *, 
        const long *, 
        const uchar, 
        const uchar );

#if W_I_N && !PTP_IMANAGER
  static void zCleanupWinSock();
#endif

privateRtn zCreateHeader( 
        union ptpHeader *, 
        const long , 
        const uchar, 
        const long , 
        const long );  

privateRtn zConnect( 
        long *, 
        const char *, 
        const long *, 
        const char *, 
        const long *, 
        PTP_SOCKET *, 
        const long *,
        long * );

privateRtn zGetServAddr( 
        void *, 
        const char * );

privateRtn zConfig( 
        const char, 
        const char *, 
        ... );

privateRtn zGetParam( 
        FILE *, 
        FILE *, 
        char *, 
        char *, 
        const uchar );

privateRtn zPutGetStream( 
        long *, 
        FILE *,
        PTP_SOCKET *, 
        char *, 
        long *, 
        const long *, 
        const long,
        const SEND_TYPE,
        const char * );

PTPAPI PTP_Connect( 
        long *, 
        const char *, 
        const long *, 
        const char *, 
        const long *, 
        PTP_SOCKET *, 
        const long *, 
        long * );

PTPAPI PTP_ReceiveRequest( 
        long *, 
        PTP_SOCKET *, 
        char *, 
        long *, 
        const long *, 
        const long *, 
        long * );

PTPAPI PTP_SendReply( 
        long *, 
        PTP_SOCKET *, 
        const char *, 
        const long *, 
        const long *, 
        const long *, 
        const long * );

PTPAPI PTP_SendRequest( 
        long *, 
        PTP_SOCKET *, 
        const char *, 
        const long *, 
        const long *, 
        const long *, 
        const long * );

PTPAPI PTP_SendSafeRequest( 
        long *, 
        PTP_SOCKET *, 
        const char *, 
        const char *, 
        const long *, 
        const long *, 
        const long *, 
        const long * );

PTPAPI PTP_ReceiveReply( 
        long *, 
        PTP_SOCKET *, 
        char *, 
        long *, 
        const long *, 
        const long *, 
        long * );

PTPAPI PTP_Shutdown( 
        long *, 
        const long *, 
        const char * );

PTPAPI PTP_Server( 
        long *, 
        const char * );

PTPAPI PTP_Cleanup( 
        PTP_SOCKET * );

PTPAPI PTP_ParseFileHeader(
        long *,
        const char *,
        long *,
        long *,
        long *,
        long *,
        long * );

PTPAPI PTP_GetHeaderSize(
        long * );

#endif
