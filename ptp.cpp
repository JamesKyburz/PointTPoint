/* CMS REPLACEMENT HISTORY, Element PTP.CPP*/
/* *13    1-OCT-2003 08:57:50 RD2JK "Change silent shutdown"*/
/* *12   25-SEP-2003 12:33:57 RD2JK ""*/
/* *11   15-SEP-2003 14:26:16 RD2JK "Reverted back to CRTL"*/
/* *10   15-SEP-2003 12:11:44 RD2JK "Replaced CRTL file i/o for windows with win32 api calls"*/
/* *9    28-AUG-2003 14:21:20 RD2JK ""*/
/* *8    28-AUG-2003 12:09:34 RD2JK "Fix in putgetdata"*/
/* *7    22-AUG-2003 11:50:25 RD2JK ""*/
/* *6    22-AUG-2003 10:41:10 RD2JK "Fix order for sendsafe"*/
/* *5    18-AUG-2003 09:29:58 RD2JK ""*/
/* *4     1-AUG-2003 11:57:52 RD2JK ""*/
/* *3    29-JUL-2003 10:14:10 RD2JK ""*/
/* *2    28-JUL-2003 16:15:35 RD2JK ""*/
/* *1    25-JUL-2003 15:57:47 RD2JK "POINTTPOINT API SOURCE FILE"*/
/* CMS REPLACEMENT HISTORY, Element PTP.CPP*/
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

/******************************************
 * zTime                                  *
 * internal routine for setting time as   *
 * year-month-day_hour:minutes:seconds    *
 * returns privateVoid                    *
 ******************************************/

privateVoid zTime( 
        char *timeBuf,        /* storage */
        const char maxSize )  /* Size of storage */
{                                                         
        TM *t;
        time_t sec;
        ( void ) time( &sec );
        t = localtime( &sec );
        /* Assign the formatted time string to the callee */
        strftime( timeBuf, maxSize - 0x01, "%Y-%m-%d_%H:%M:%S", t );
}

#if W_I_N && !PTP_IMANAGER
/******************************************
 * zCleanupWinSocks                       *
 * Cleanup windows sockets                *
 * returns void                           *
 ******************************************/
static void zCleanupWinSock( )
{
        WSACleanup();
}
#endif


/******************************************
 * zPTPLog                                *
 * Write information to disk or console   *
 * Information can be errors or data you  *
 * always want to log.                    *
 * returns privateVoid                    *
 ******************************************/

/* Do not create function if calling from COM as will
   call a COM component there to log */

#if !PTP_IMANAGER
privateVoid zPTPLog( 
        FILE *fp,            /* file or console */
        const char outLevel, /* level of output */
        const char *funcName,/* callee function */
        const int type,      /* type of information i.e error/informational */
        const int code,      /* code to be used for error code if needed */
        const char *fmt,     /* format argument for vprintf routines */
        ... )                /* varied arguments */
{
        va_list ap;      /* called arguments */
        char timeBuf[0x50];      
        char message[PTP_MAX_LOG_MESSAGE_SIZE];
        char *logLevel[] = { "-E-",
                             "-I-",
                             "-W-",
                             "-D-",
                             "-S-",
                             "" };

        /* if level of output is 0 or pointer (fp) is not set log nothing.
           if the level of output is not 3 (all output).
           if level is 1 only report errors,
           if level is 2 only report informational + errors,
           if level is 3 report everything.
        */
                                   
        if ( !outLevel || !fp )
        {
          return;
        }

        if ( outLevel == 0x01 && type != 0x00 )
        {
          return;
        }
        
        if ( outLevel == 0x02 && type != 0x00 && type != 0x01 )
        {
          return;
        }

        va_start( ap, fmt );                                            

        /* check if type is normal text with no type (plain text)*/
        if ( type != PTP_LOG_LEVEL_NOTHING )
        {
          /* Timestamp the log entry */
          zTime( timeBuf, sizeof( timeBuf ) );
          fprintf( fp, "\n%s", timeBuf );
          fprintf( fp, " - " );
          message[vsprintf( message, fmt, ap )] = '\x00';
          /* write message to log file */
          fprintf( fp, "%s%s%s, system msg:%s, errno:%d\n", funcName, 
                   logLevel[type], message, strerror( PTP_ERROR ), PTP_ERROR );
        } 

        else
        {   
          /* write message to log file */
          vfprintf( fp, fmt, ap );
        }

        /* flush file if the pointer is not pointing to console device */
        if( fp != PTP_CONSOLE && fp )
        {
          fflush( fp );
        }
        va_end( ap );
}
#endif

/******************************************
 * zCleanup                               *
 * internal routine for cleaning up a     *
 * socket device using close and shutdown *
 * returns privateVoid                    *
 ******************************************/

privateVoid zCleanup( 
        FILE *fp,               /* file or console for logging */
        const char outLevel,    /* level of output */
        const char shut,        /* 1=perform shutdown() */
        PTP_SOCKET *sockFd,     /* connected socket channel */
        const char gatekeeper ) /* is the socket the gatekeeper in PTP_Server */
{

        /* if no connection (socket descriptor is 0) then do nothing */
        if( !*sockFd )
        {
          return;
        }

        /* does the calle want to shutdown the socket ? */
        if (shut)
        {
           /* See if connection already shutdown by checking PTP_ERROR 
              do not shutdown connection if error is anything 
              this can happen when the connection dies before call to
              this function
           */
          if ( PTP_ERROR != PTPECONNRESET )
          {
            /* shutdown both directions of the connection */
            if ( shutdown( *sockFd, PTP_SD_BOTH ) < 0x00 )
            {
              zPTPLog( 
                fp, outLevel, "zCleanup", 
                PTP_LOG_LEVEL_INFORMATION, 0x00, 
                "%s %d\nSource: %s V%s , Compiled: %s %s Line: %d\n", 
                "Failed to shutdown connection on channel", *sockFd, 
                __FILE__, PTP_VER, __DATE__, __TIME__, __LINE__ 
              );
            }
          }
        }
/* close the socket (freeing the device) */
#if W_I_N
        if ( closesocket( *sockFd ) < 0x00 )
#else
        if ( close( *sockFd ) < 0x00 )
#endif
        {

          zPTPLog( 
            fp, outLevel, "zCleanup", 
            PTP_LOG_LEVEL_INFORMATION, 0x00, 
            "%s %d\n/Source: %s V%s , Compiled: %s %s Line: %d\n", 
            "Failed to close connection", *sockFd, 
            __FILE__, PTP_VER, __DATE__, __TIME__, __LINE__ 
          );

        }
 
        /* reset the channel number to 0 */
        *sockFd = 0x00;
}

/******************************************
 * zCreateHeader                          *
 * internal routine for assigning the     *
 * header values used by both             *
 * client/server to send header to        *
 * gatekeeper                             *
 * returns PTP_EXIT_SUCCESS               *
 *         PTP_EXIT_FAILURE               *
 ******************************************/

privateRtn zCreateHeader( 
        union ptpHeader *details, /* the header block */
        const long buffSize,      /* to assign to data size in header */
        const uchar funk,         /* client or server type */
        const long encrypt,       /* 1=encryted data will be sent */
        const long language )     /* language of client/server data */
{
        char fmt[10];
        double max;

        /* Check the buffer size to send does not exceed the character 
        representation in bytes in the header */
        if ( buffSize > 
            ( max = (pow( 10, (sizeof( details->u.cSize ) - 0x01) ) - 0x01) ) )
        {
          zPTPLog( PTP_CONSOLE, 0x03, "zCreateHeader", PTP_LOG_LEVEL_ERROR, 
                   0x00, 
               "%s %d %s %d %s %d\nSource: %s V%s , Compiled: %s %s Line: %d\n",
                   "Message max size exceeded, max =", 
                   ( int ) max,
                   ",buffer size=",
                   buffSize,
                   ",function=",
                   funk,
                   __FILE__, PTP_VER, __DATE__, __TIME__, __LINE__ ); 

          return PTP_EXIT_FAILURE;

        }

        sprintf( fmt, "%%%d.%dd", sizeof ( details->u.cSize ) - 0x01, 
                                  sizeof ( details->u.cSize ) - 0x01); 
        /* size in ascii bytes */
        sprintf( details->u.cSize, fmt, buffSize );
        /* assign all header values */
        details->u.function = funk;
        details->u.ptpIdentifier = sizeof ( details->cHeader );
        details->u.encrypt = ( uchar ) encrypt;
        details->u.language = ( uchar ) language;

        return PTP_EXIT_SUCCESS;
}

/******************************************
 * zAddQ                                  *
 * internal routine for creating a link   *
 * in a linked list and setting default   *
 * values                                 *
 * note - only called from PTP_Server     *
 * returns a pointer to the new link      *
 ******************************************/

PTP_MESSAGE *zAddQ( 
        PTP_MESSAGE **lastQ,   /* last link in list */
        PTP_SOCKET channel )   /* messages connected socket channel */
{
        PTP_MESSAGE *prevQ;
        PTP_MESSAGE *que;      /* pointer used to create new link */
        
        que = ( PTP_MESSAGE * ) malloc( sizeof( PTP_MESSAGE ) );

        if ( !que )            /* no space available ? */
        { 
          return NULL;
        }

        /* if there was a link before make it to point to the new 
           link (as it is now the last link),  and set the last links
           prevQ to it.
        */

        if ( *lastQ )
        {
          prevQ = *lastQ;
          prevQ->nextQ = que;
          que->prevQ = prevQ;
        }

        /* no previous link */
        else
        {
          que->prevQ = NULL;
        }

        /* set the last link the newly created link */
        *lastQ = que;

        /* set default values */

        que->nextQ = NULL;
        que->pairQ = NULL;
        que->type = PTP_NONE;
        que->status = PTP_IDLE;
        que->bytesReceived = 0x00;
        que->deleteFlag = 0x00;
        que->validated = 0x00;
        que->processedQuestion = 0x00;
        que->noSize = 0x00;
        que->channel = channel;
        return que;
}

/******************************************
 * zDelQ                                  *
 * internal routine for deleting a link   *
 * in a linked list                       *
 * note - only called from PTP_Server     *
 * resets last link if needed             *
 * returns privateVoid                    *
 ******************************************/

privateVoid zDelQ( 
        PTP_MESSAGE **lastQ, 
        PTP_MESSAGE *que )
{
        if ( !que )
        { 
          return;
        }

        if ( que->prevQ )
        {
          if ( que->nextQ )
          {
            que->prevQ->nextQ = que->nextQ;
            que->nextQ->prevQ = que->prevQ;
          }
          else
          {
            *lastQ = que->prevQ;
            que->prevQ->nextQ = NULL;
          }
        }
        else
        {
          if ( que->nextQ )
          {
            que->nextQ->prevQ = NULL;
          }
          else
          {
            *lastQ = NULL;
          }
        }
        
        if ( que->pairQ )
        {
          que->pairQ->pairQ = NULL;
          que->pairQ = NULL;
        }
        
        free( que );
}


/******************************************
 * zGetParam                              *
 * gets the value of a keyword contained  *
 * in a file must use format as example   *
 * example line :-                        *
 * TCP/IP port          '1234'            *
 * TCP/IP port is the keyword and 1234 is *
 * the return value                       *
 * note: only called from zConfig         *
 * returns PTP_EXIT_SUCCESS               *
 *         PTP_EXIT_FAILURE               *
 ******************************************/

privateRtn zGetParam( 
        FILE *stream,        /* point to log output */
        FILE *fp,            /* config file pointer */
        char *keyword,       /* keyword */
        char *result,        /* result */
        const uchar length ) /* max length 1 - 255 of result */
{
        char inparam[PTP_PARAM_LINE + 0x01];
        char *pek = inparam, *pek2;
        
        /* keep reading a line until ! is not the first
           character (comment character)
        */
        while( fgets( inparam, PTP_PARAM_LINE, fp) != NULL)
        {
          if ( ( *pek + 0x00 ) != '!' )
          {
            break;
          }
        }

        /* if the keyword does not exist return error */
        if ( strstr( pek, keyword ) != NULL ) 
        {
          /* find ' in line */
          pek = strstr( pek, "'" );
          if ( pek )
          {
            /* find next ' in line */
            pek2 = strstr( pek + 0x01, "'" );
            if ( pek2 )
            {           
              *( pek2 + 0x00 ) = '\0';
              /* copy value inbetween two 's */
              strcpy( keyword, pek + 0x01 );
              return PTP_EXIT_SUCCESS;
            }
          }
        }

        zPTPLog( stream, 0x03, "zGetParam", PTP_LOG_LEVEL_ERROR, 
                 0x00, 
                 "%s %s\nSource: %s V%s , Compiled: %s %s Line: %d\n",
                 "SERVER config - invalid keyword", 
                 keyword,
                 __FILE__, PTP_VER, __DATE__, __TIME__, __LINE__ ); 

        return PTP_EXIT_FAILURE;

}

/******************************************
 * zConfig                                *
 * opens a config file calling zGetParam  *
 * with each given argument. The result   *
 * of the found keyword in the config     *
 * file is then written to the same       *
 * address.                               *
 * Because this routine is called during  *
 * the startup of PTP_Server a temporary  *
 * log file PTP_STARTUP_LOG is opened to  *
 * record any failures, on success the    *
 * file is deleted.                       *
 *                                        *
 * note: only called from PTP_Server and  *
 *       PTP_Shutdown                     *
 * returns PTP_EXIT_SUCCESS               *
 *         PTP_EXIT_FAILURE               *
 ******************************************/
privateRtn zConfig( 
        const char argc,       /* number of arguments */
        const char *filename,  /* filename of config file */
        ... )                  /* ... varied argument declaration */
{
        va_list   ap;      /* varied arguments */
        FILE     *fp;      /* config file */
        FILE *stream;      /* log output */
        char     *keyword; /* keyword
                              note the result will be written to the
                              same address
                           */

        char tmpFil[0xFF]; /* Temporary filename to report failures */
                
        time_t ticks;    

        long retCode = PTP_INTERFACE_SUCCESS;

        register i;        /* register variable to loop through arguments */

        ticks = time( ( time_t * ) NULL );

        /* create a filename that is unique */

        sprintf( tmpFil, "%s%ld%08o.TXT\0", PTP_STARTUP_LOG, 
                 ( long ) ticks, getpid() );

        /* open temporary log file */

        stream = fopen( tmpFil, "a+" );

        if ( NULL != filename && NULL != stream )
        {
          fprintf( stream, "%s= %s", "Config file", filename ); 
        }

        /* do no processing if filename or config file not given */
        if (NULL == filename) 
        {                        
          zPTPLog( stream, 0x03, "zConfig", PTP_LOG_LEVEL_ERROR, 
                   0x00, 
                   "%s\nSource: %s V%s , Compiled: %s %s Line: %d\n",
                   "SERVER- no config file found in call to zConfig"
                   __FILE__, PTP_VER, __DATE__, __TIME__, __LINE__ );

          return PTP_EXIT_FAILURE;
        }

        /* open the config file for reading checking for any errors */
        if( !(fp = fopen( filename, "r" )) ) 
        {  

          zPTPLog( stream, 0x03, "zConfig", PTP_LOG_LEVEL_ERROR,
                   0x00, 
                   "%s %s\nSource: %s V%s , Compiled: %s %s Line: %d\n",
                   "SERVER - failed to open configuration file",
                   filename, 
                   __FILE__, PTP_VER, __DATE__, __TIME__, __LINE__ ); 
          if ( stream )
          {
            fclose( stream );
            return PTP_EXIT_FAILURE;
          }
        }

        /* start variable argument processing */
        va_start( ap, filename );   

        /* for argc times call GetParam with keyword from arg */
        for( i=0x00; i<argc; i++ ) 
        {                
          /* get called argument */
          keyword = ( char * ) va_arg( ap, char * );
          if ( !keyword ) continue;              
          /* if keyword not found report error */
          if( (zGetParam( stream, fp, keyword, keyword, 
                          PTP_PARAM_LINE )) == PTP_EXIT_FAILURE ) 
          {
            retCode = PTP_INTERFACE_FAILURE;
            break;
          }
        }

        /* close log file */
        if ( fp ) 
        {
          fclose( fp );
        }

        /* end variable argument processing */
        va_end( ap ); 
        
        /* close temporary log file and remove if no errors occured */
        if ( stream )
        {
          fclose( stream );
          if ( retCode == PTP_INTERFACE_SUCCESS )
          {
            ( void ) remove( tmpFil );
          }
        }

        return retCode == 
               PTP_INTERFACE_SUCCESS ? PTP_EXIT_SUCCESS : PTP_EXIT_FAILURE;
}

/******************************************
 * zGetServAddr                           *
 * gets the ip address of a host using    *
 * DNS lookup                             *
 * note: only called from PTP_Server and  *
 *       zConnect, will hang if the host  *
 * does not exists in DSN as the lookup   *
 * will go out to the internet and will   *
 * timeout after a few minutes.            *
 * returns PTP_EXIT_SUCCESS               *
 *         PTP_EXIT_FAILURE               *
 ******************************************/
privateRtn zGetServAddr( 
        void *addrptr, 
        const char * buf ) 
{
        INADDR val;                     /* sock in structure */
        HOSTENT *host;                  /* host entity structure */
        val.s_addr = inet_addr( buf );  /* use internet format ip address */

        /* if the host name was an ip address return success */
        if ( ( int ) val.s_addr != ( int ) INADDR_NONE ) 
        {
           memcpy( addrptr, &val, sizeof( INADDR ) );
           return PTP_EXIT_SUCCESS;
        }

        /* DNS lookup where the host must exist in the database */

        if ( host = gethostbyname( buf ) ) 
        {
          memcpy( addrptr, host->h_addr, sizeof( INADDR ) );
        }

        else
        {
          /* host not found */
          return PTP_EXIT_FAILURE;
        }

        return PTP_EXIT_SUCCESS;
}


/******************************************
 * zConnect                               *
 * creates a socket                       *
 * and attempts to connect to a IP server *
 * note: only called from PTP_Connect     *
 * if the connect fails the failover host *
 * will be used                           *
 * retCode PTP_INTERFACE_SUCESS           *
 *         PTP_INTERFACE_FAILURE          *
 *         PTP_INTERFACE_TIMEOUT          *
 * returns PTP_EXIT_SUCCESS               *
 *         PTP_EXIT_FAILURE               *
 ******************************************/
privateRtn zConnect( 
        long *retCode,            /* address of return value */
        const char *host,         /* host to connect to */
        const long *port,         /* port to connect to */
        const char *failoverHost, /* failover host to connect to */
        const long *failoverPort, /* failover port to connect to */
        PTP_SOCKET *sockFd,       /* channel that will be connected */ 
        const long *timeout,      /* timeout */
        long *failed )            /* failover count */
{
        ISOCKET clientAddr;    /* client address used for connection */
        PTP_ADDRLEN clientAddrLen = sizeof ( clientAddr );
        register int rounds;
        ulong block = 0x01;
        long ackSize = 0x01;
        fd_set writeFds;
        struct timeval tv,*pTv = NULL;
        char ack[2];
        const char *currentHost;
        const long *currentPort;

        /* if a socket exists return success as already
           connected
        */

        if ( getpeername( *sockFd, 
                          (struct sockaddr *) &clientAddr, 
                          &clientAddrLen ) )
        {
          *retCode = PTP_INTERFACE_FAILURE;
          for ( rounds = 0x00; rounds < 0x02; rounds++ )
          {

            /* An error occured so cleanup socket if exists (*sockFd!=0)*/
            if ( (*sockFd != 0x00 ) && (*retCode !=  PTP_INTERFACE_SUCCESS) )
            {
              zCleanup ( PTP_CONSOLE, 0x03, 0x00, sockFd, 
                         0x00 );
            }

            currentHost = *failed == 0x00 ? host : failoverHost;
            currentPort = *failed == 0x00 ? port : failoverPort;

            /* if end point information not supplied go back to for check 
               making sure to set failed back 
            */

            if ( !currentHost )
            {
              *failed = *failed == 0x00 ? 0x01 : 0x00;
              continue;
            }

            if ( !currentPort ) 
            {
              *failed = *failed == 0x00 ? 0x01 : 0x00;
              continue;
            }

            if ( !*currentHost ) 
            {
              *failed = *failed == 0x00 ? 0x01 : 0x00;
              continue;
            }

            if ( !*currentPort ) 
            {
              *failed = *failed == 0x00 ? 0x01 : 0x00;
              continue;
            }
            
            *retCode = PTP_INTERFACE_SUCCESS;                
            clientAddr.sin_family = AF_INET;
            clientAddr.sin_port   =  htons( ( ushort ) *currentPort );

            /* get ip address of host return error if not found */
            if( zGetServAddr( &clientAddr.sin_addr, currentHost ) )
            {

              zPTPLog( PTP_CONSOLE, 0x03, "zConnect",
                       PTP_LOG_LEVEL_ERROR, 0x00, 
                       "%s %s\nSource: %s V%s , Compiled: %s %s Line: %d\n",
                       "DSN lookup failed for host",
                       currentHost,
                       __FILE__, PTP_VER, __DATE__, __TIME__, __LINE__ ); 
              *retCode = PTP_INTERFACE_FAILURE;
              *failed = *failed == 0x00 ? 0x01 : 0x00;
              continue;
            }

            /* create socket for outbound connection report any errors */

            if ( (*sockFd = socket( AF_INET, SOCK_STREAM, 0x00)) <= 0x00 )
            { 
              zPTPLog( PTP_CONSOLE, 0x03, "zConnect", 
                       PTP_LOG_LEVEL_ERROR, 0x00, 
                       "%s\nSource: %s V%s , Compiled: %s %s Line: %d\n",
                       "Outbound socket creation failed",
                       __FILE__, PTP_VER, __DATE__, __TIME__, __LINE__ );
              *retCode = PTP_INTERFACE_FAILURE;
              return PTP_EXIT_FAILURE;                                     
            }

            /* set socket to nonblocking so as to not wait for the connect
               call to complete. the checking is done in the select statement
               to check if the socket is writeable ( here timeout applies ) 
               and by using getpeername to check if the connect has completed
            */

            PTP_IOCTL( *sockFd, FIONBIO, ( ulong * ) &block );

            /* 
            * Connect socket 1 to addr. 
            */ 

            ( void ) connect( *sockFd, (struct sockaddr *)&clientAddr, 
                           sizeof( clientAddr ) ); 
            /* If no timeout exists or is less than 3 set the wait time in 
               select to max 3 seconds otherwise use the timeout sent */

            tv.tv_sec = (*timeout < 0x02 && *timeout ) ? *timeout : 0x03;
            tv.tv_usec = 0x00;
            pTv = &tv;

            FD_ZERO( &writeFds );
            FD_SET( *sockFd, &writeFds );

            if ( (select ( *sockFd + 0x01, 
                           ( fd_set *) NULL, 
                           &writeFds, 
                           ( fd_set *) NULL, 
                           pTv )) <= 0 )             
            {
              zPTPLog( 
                PTP_CONSOLE, 0x03, "zConnect", 
                PTP_LOG_LEVEL_ERROR, 0x00,
                "%s %s:%d\nSource: %s V%s , Compiled: %s %s Line: %d\n",
                "Connection timed out on",
                currentHost,
                *currentPort,
                 __FILE__, PTP_VER, __DATE__, __TIME__, __LINE__ 
              );            
              *retCode = PTP_INTERFACE_TIMEOUT;
              *failed = *failed == 0x00 ? 0x01 : 0x00;
              continue;
            }

            /* check if connected */
            else if ( getpeername( *sockFd, 
                                   (struct sockaddr *) &clientAddr, 
                                   &clientAddrLen ) )
            {
              zPTPLog( 
                PTP_CONSOLE, 0x03, "zConnect", 
                PTP_LOG_LEVEL_ERROR, 0x00, 
                "%s %s:%d\nSource: %s V%s , Compiled: %s %s Line: %d\n",
                "Connection to server failed on",
                currentHost,
                *currentPort, 
                __FILE__, PTP_VER, __DATE__, __TIME__, __LINE__  
              );

              *retCode = PTP_INTERFACE_FAILURE;
              *failed = *failed == 0x00 ? 0x01 : 0x00;
              continue;
            }
            
            /* Turn off non blocking socket */          
            block = 0x00;
            PTP_IOCTL( *sockFd, FIONBIO, ( ulong * ) &block );
            
            /* Connected was succesfull, now get ack from server, this is 
               done after memory and security checks
            */

            if ( *retCode == PTP_INTERFACE_SUCCESS )
            {             
              /* If receive of ack failed return error (at end of function)
                 do not cleanup socket on error as this is done in this 
                 function */
              if ( zPutGetStream( retCode, PTP_CONSOLE, sockFd, ack, 
                                  &ackSize, timeout, 
                                  0x00, PTP_RECEIVE, "zConnect" ) )
              {
                *failed = *failed == 0x00 ? 0x01 : 0x00;
                continue;
              }
     
              /* receive worked so now check contents of ack received from 
                 gatekeeper if not Y then return error
              */
              else if ( ack[0] != PTP_ACK )
              {
                *retCode = PTP_INTERFACE_FAILURE;
        
                zPTPLog( 
                  PTP_CONSOLE, 0x03, "zConnect", PTP_LOG_LEVEL_INFORMATION, 
                  0x00, 
                  "%s %s\nSource: %s V%s , Compiled: %s %s Line: %d\n",
                  "failed to get connect ack from ",
                  currentHost,
                  __FILE__, PTP_VER, __DATE__, __TIME__, __LINE__  
                );
                *failed = *failed == 0x00 ? 0x01 : 0x00;
                continue;
              }
              break;
            /* now we are connected ! */
            } /* end if */
          } /* end for */

        } /* getpeername != 0x00 */

        /* A connection already exists so do nothing */
        else /* getpeername == 0x00 */
        {
          *retCode = PTP_INTERFACE_SUCCESS;
        }  

        /* if an error occured so cleanup socket if exists (*sockFd!=0)*/
        if ( (*sockFd != 0x00 ) && (*retCode !=  PTP_INTERFACE_SUCCESS) )
        {
          zCleanup ( PTP_CONSOLE, 0x03, 0x00, sockFd, 0x00 );
        }

        return *retCode == 
               PTP_INTERFACE_SUCCESS ? PTP_EXIT_SUCCESS : PTP_EXIT_FAILURE;
}

/******************************************
 * zPutGetStream                          *
 * reads or writes to/from a connected    *
 * socket.                                *
 * This routines calls zPutGetData until  *
 * all bytes are either written or read   *
 * retCode PTP_INTERFACE_SUCESS           *
 *         PTP_INTERFACE_FAILURE          *
 *         PTP_INTERFACE_TIMEOUT          *
 * returns PTP_EXIT_SUCCESS               *
 *         PTP_EXIT_FAILURE               *
 ******************************************/
privateRtn zPutGetStream(         
        long *retCode,               /* return value */
        FILE *fp,                    /* where to log */
        PTP_SOCKET *sockFd,          /* connected socket channel */
        char *buffer,                /* data to be sent/received */
        long *buffSize,              /* buffer size */
        const long *timeout,         /* timeout */
        const long clearCallOnError, /* 1=call zCleanup on errors, 0=do not */
        const SEND_TYPE sType,       /* PTP_SEND / PTP_RECEIVE ? */
        const char *caller )         /* calling function */
{
     
        char *sendReceive[] = { "zPutGetStream/send",
                                "zPutGetStream/recv" };
        struct timeval tv;
               long bytes, totalBytes, 
               localBuffSize = *buffSize;

        /* clear error */
        PTP_SET_ERROR( 0x00 );

        /* if no connection return error */
        if( *sockFd <= 0x00 )
        {
          zPTPLog( fp, 0x03, sendReceive[sType], 
                   PTP_LOG_LEVEL_ERROR, 0x00, 
                   "[%s]%s\nSource: %s V%s , Compiled: %s %s Line: %d\n", 
                   caller,
                   "invalid socket descriptor ",
                   __FILE__, PTP_VER, __DATE__, __TIME__, __LINE__ );

          *retCode = PTP_INTERFACE_FAILURE;
          return PTP_EXIT_FAILURE;
        }

        /* if buffSize = 0 then return error */
        if( !*buffSize)
        {
          zPTPLog( fp, 0x03, sendReceive[sType], 
                   PTP_LOG_LEVEL_ERROR, 0x00, 
                   "[%s]%s\nSource: %s V%s , Compiled: %s %s Line: %d\n", 
                   caller,
                   "invalid size",
                   __FILE__, PTP_VER, __DATE__, __TIME__, __LINE__ );

          if ( clearCallOnError )
          {
            zCleanup ( fp, 0x03, 0x01, sockFd, 0x00 );
          } 
          *retCode = PTP_INTERFACE_FAILURE;
          return PTP_EXIT_FAILURE;
        }

        tv.tv_sec = *timeout;
        tv.tv_usec = 0x00;

        for( totalBytes = 0x00, bytes = 0x01; 
             localBuffSize && bytes; 
             totalBytes+= bytes, buffer+=bytes )
        {
          bytes = zPutGetData( retCode, 
                               fp,
                               sockFd, 
                               buffer, 
                               &localBuffSize, 
                               tv.tv_sec ? &tv : NULL, 
                               clearCallOnError, 
                               sType,
                               caller );

          /* once data has started to be transmitted ignore the senders
             timeout and use 2 seconds,  this is so if there are any
             delays in the middle of communication the function will
             return instead of hanging
          */

          tv.tv_sec = 0x02;

        }

        if ( sType == PTP_RECEIVE )
        {
          *buffer = '\x00';
          *buffSize = totalBytes;
        }

        return *retCode == PTP_INTERFACE_SUCCESS ? 
               PTP_EXIT_SUCCESS : PTP_EXIT_FAILURE;
}

/******************************************
 * zPutGetData                            *
 * reads or writes to/from a connected    *
 * socket.                                *
 * retCode PTP_INTERFACE_SUCESS           *
 *         PTP_INTERFACE_FAILURE          *
 *         PTP_INTERFACE_TIMEOUT          *
 * returns bytes written or read          *
 *         PTP_EXIT_FAILURE               *
 ******************************************/
long zPutGetData( 
        long *retCode,               /* return value */
        FILE *fp,                    /* where to log */
        PTP_SOCKET *sockFd,          /* connected socket channel */
        char *buffer,                /* data to be sent/received */
        long *buffSize,              /* buffer size */
        const struct timeval *pTv,   /* timeout */
        const long clearCallOnError, /* 1=call zCleanup on errors, 0=do not */
        const SEND_TYPE sType,       /* PTP_SEND / PTP_RECEIVE ? */
        const char *caller )         /* calling function */
{
     
        char *sendReceive[] = { "zPutGetStream/send",
                                "zPutGetStream/recv" };
        int bytes = 0x00, rc;
        int sendReceiveSize;
        fd_set readFds, writeFds, rSet;
        ISOCKET clientAddr;    /* client address used for connection */
        PTP_ADDRLEN clientAddrLen = sizeof ( clientAddr );

        /* get connection details, if not connected return */
        if ( getpeername( *sockFd, 
               (struct sockaddr *) &clientAddr, 
               &clientAddrLen ) )
        {
          zPTPLog( fp, 0x03, sendReceive[sType], 
                   PTP_LOG_LEVEL_ERROR, 0x00, 
                   "[%s]%s\nSource: %s V%s , Compiled: %s %s Line: %d\n", 
                   caller,
                   "invalid socket descriptor ",
                   __FILE__, PTP_VER, __DATE__, __TIME__, __LINE__ );

          *retCode = PTP_INTERFACE_FAILURE;
          if ( clearCallOnError )
          {
            zCleanup ( fp, 0x03, 0x01, sockFd, 0x00 );
          } 
          return 0x00; /* no bytes written or read */
        }

        /* clear error */
        PTP_SET_ERROR( 0x00 );

        /* assign the correct FD variables for either send/receive */

        if ( sType == PTP_RECEIVE )
        {
          FD_ZERO( &readFds );
          FD_SET( *sockFd, &readFds );
          rSet = readFds;
        }

        else 
        {
          FD_ZERO( &writeFds );
          FD_SET( *sockFd, &writeFds );
          rSet = writeFds;
        }

        /* can only send or receive 64k at a time so check here */

        sendReceiveSize = ( int ) *buffSize > PTP_MAX_MESSAGE_SIZE ? 
                          PTP_MAX_MESSAGE_SIZE : *buffSize;

        /* check socket for readable or writeable */
        rc = sType == PTP_RECEIVE ? 
        select ( *sockFd + 0x01, &rSet, (fd_set *) NULL, (fd_set *) NULL, 
                 ( struct timeval * ) pTv) :
        select ( *sockFd + 0x01, (fd_set *) NULL, &rSet, (fd_set *) NULL, 
                 ( struct timeval * ) pTv);

        /* If the errors are not fatal  
           ignore the errors otherwise report error */

        if ( PTP_ERROR == PTPEPERM ) 
        {
          PTP_SET_ERROR( 0x00 );
        }
       
        /* socket is ready for send or recv */
        if ( rc > 0x00 )
        {
          bytes = sType == PTP_RECEIVE ? 
            recv( *sockFd, buffer, sendReceiveSize, 0x00 ) :
            send( *sockFd, buffer, sendReceiveSize, 0x00 );
          /* if bytes written or read is <= 0 then report error */
          if ( bytes <= 0x00 )
          {
            zPTPLog( fp, 0x03, sendReceive[sType], 
              PTP_LOG_LEVEL_ERROR, 0x00, 
              "[%s]%s %d%s%s:%d\nSource: %s V%s , Compiled: %s %s Line: %d\n", 
              caller,
              "send/recv failed on channel", *sockFd,
              ",connection was ", 
              inet_ntoa( clientAddr.sin_addr ), 
              ntohs( clientAddr.sin_port ),
              __FILE__, PTP_VER, __DATE__, __TIME__, __LINE__ 
            );

            if ( clearCallOnError )
            {
              zCleanup ( fp, 0x03, 0x01, sockFd, 0x00 );
            } 
            *retCode = PTP_INTERFACE_FAILURE;
          }

          else
          {
            *retCode = PTP_INTERFACE_SUCCESS;
            *buffSize -= bytes;
          } 
        }
        /* the socket was not readable/writable because of error or timeout 
           if PTP_ERROR is 0 then timeout was the cause
        */
        else
        {
          if ( PTP_ERROR == 0x00 )                                
          {
            if ( clearCallOnError )
            {
              zCleanup ( fp, 0x03, 0x01, sockFd, 0x00 );
            } 
            *retCode = PTP_INTERFACE_TIMEOUT;
          }
          else
          {
             zPTPLog( fp, 0x03, sendReceive[sType], 
               PTP_LOG_LEVEL_ERROR, 0x00, "%s", 
               "[%s]%s %d%s%s:%d\nSource: %s V%s , Compiled: %s %s Line: %d\n", 
               caller,
               "select failed on channel", *sockFd,
               ",connection was ", 
               inet_ntoa( clientAddr.sin_addr ), 
               ntohs( clientAddr.sin_port ), 
               __FILE__, PTP_VER, __DATE__, __TIME__, __LINE__ 
             );

             if ( clearCallOnError )
             {
               zCleanup ( fp, 0x03, 0x01, sockFd, 0x00 );
             } 
            *retCode = PTP_INTERFACE_FAILURE;
          }
        }

        return *retCode == PTP_INTERFACE_SUCCESS ? bytes : 0x00;
}

/******************************************
 * zDecrypt                               *
 * Decrypts data                          *
 * returns privateVoid                    *
 ******************************************/
privateVoid zDecrypt( 
        uchar *p, 
        int bufferSize )
{
        uchar offset;

        for( ; bufferSize; bufferSize-- )
        {
          offset = *p << 6;
          *p >>= 2;
          *p+=offset;
          *p = *p ^ ( uchar ) 12;
          *p = ~*p;
          *p++;
        }

}

/******************************************
 * zEncrypt                               *
 * Encrypts data                          *
 * returns privateVoid                    *
 ******************************************/
privateVoid zEncrypt( 
        uchar *p, 
        int bufferSize )
{
        uchar offset;
        
        for( ; bufferSize; bufferSize-- )
        {
          *p = ~*p;
          *p = *p ^ ( uchar ) 12;
          offset = *p >> 6;
          *p <<= 2;
          *p+=offset;
          *p++;
        }

}

/******************************************
 * zSendFunction                          *
 * Sends a function to the server         *
 * retCode PTP_INTERFACE_SUCESS           *
 *         PTP_INTERFACE_FAILURE          *
 *         PTP_INTERFACE_TIMEOUT          *
 * returns PTP_EXIT_SUCCESS               *
 *         PTP_EXIT_FAILURE               *
 ******************************************/
privateVoid zSendFunction( 
        long *retCode,        /* return value */
        const char *host,     /* host to send */
        const long *port,     /* port to send */
        const long *timeout,  /* timeout of operation */
        const uchar function, /* function to send */
        const uchar p1 )      /* parameter to send */
{
        PTP_SOCKET sockFd = 0x00;
        union ptpHeader details;
        long headerSize = sizeof ( details.cHeader ) - 0x01;
        long failed = 0x00;

        /* if connect failed return error */
        if ( zConnect( retCode, host, port, 0x00, 0x00, &sockFd, 
                       timeout, &failed ) )
        {
          return;
        }

        /* create header to send to server */
        ( void )
        zCreateHeader( &details, PTP_MAX_MESSAGE_SIZE , function, 
                       0x00, p1 );

        /* send function to server and disconnect */
        if ( !(zPutGetStream( retCode, PTP_CONSOLE, &sockFd, 
                              details.cHeader, &headerSize, timeout, 
                              0x01, PTP_SEND, "zSendFunction" )) )
        {
          zCleanup ( PTP_CONSOLE, 0x03, 0x01, ( PTP_SOCKET * )&sockFd, 0x00 );
        }

}

/******************************************
 * zReceiveAnswer                         *
 * receives answer from a requester       *
 * retCode PTP_INTERFACE_SUCESS           *
 *         PTP_INTERFACE_FAILURE          *
 *         PTP_INTERFACE_TIMEOUT          *
 * returns PTP_EXIT_SUCCESS               *
 *         PTP_EXIT_FAILURE               *
 ******************************************/
privateVoid zReceiveAnswer(    
        long *retCode,           /* return value */
        PTP_SOCKET *sockFd,      /* connected socket channel */
        char *buffer,            /* buffer that will receive data */
        long *buffSize,          /* buffer size received */
        const long *maxBuffSize, /* max size of buffer */
        const long *timeout,     /* timeout */
        long *language )         /* language of requester */
{

        union ptpHeader details;
        long headerSize = sizeof( details.cHeader ) - 0x01;
        long tmpBuffSize = 0x01;
        char ack[2] = { PTP_ACK, 0x00 };

        /* get header from replier */
        if ( zPutGetStream( retCode, PTP_CONSOLE, sockFd, details.cHeader, 
                            &headerSize, 
                            timeout, 0x01, PTP_RECEIVE,
                            "zReceiveAnswer" ) )
        {
          return;
        }
         
        /* send ack to replier */
        ack[0] = PTP_ACK;
        if ( zPutGetStream( retCode, PTP_CONSOLE, sockFd, ack, 
                            &tmpBuffSize, timeout, 
                            0x01, PTP_SEND,
                            "zReceiveAnswer" ) )
        {
          return;
        }

        /* get language from received header */

        *language = ( long ) details.u.language; 

     
/*      To do text conversion based on language if needed
*/

        /* check max size < size of data to arrive */
        tmpBuffSize = *buffSize = atoi( details.u.cSize );
#if PTP_IMANAGER

        m_TemporaryPointer = buffer = 
        ( char * ) malloc( ( size_t ) tmpBuffSize + 0x01 );
        if ( !m_TemporaryPointer )
        {
          zPTPLog( PTP_CONSOLE, 0x03, "zReceiveAnswer", 
                   PTP_LOG_LEVEL_ERROR, 0x00, 
                   "%s\nSource: %s V%s , Compiled: %s %s Line: %d\n", 
                   "failed/dynamic memory exhausted",
                   __FILE__, PTP_VER, __DATE__, __TIME__, __LINE__ );

          zCleanup ( PTP_CONSOLE, 0x03, 0x01, sockFd, 0x00 );
          *retCode = PTP_INTERFACE_FAILURE;
          return;
        }

        buffer[0x00] = '\x00';
#else

        buffer[0x00] = '\x00';

        if ( tmpBuffSize > *maxBuffSize )
        {
          zPTPLog( PTP_CONSOLE, 0x03, "zReceiveAnswer", 
                   PTP_LOG_LEVEL_ERROR, 0x00, 
                 "%d %s %d %s %d\nSource: %s V%s , Compiled: %s %s Line: %d\n", 
                   tmpBuffSize,
                   "byte message to big for",
                   *maxBuffSize,
                   "byte receiving buffer on channel", *sockFd,
                   __FILE__, PTP_VER, __DATE__, __TIME__, __LINE__ );


          zCleanup ( PTP_CONSOLE, 0x03, 0x01, sockFd, 0x00 );
          *retCode = PTP_INTERFACE_FAILURE;
          return;
        }
#endif

        /* receive data from replier */
        if ( zPutGetStream( retCode, PTP_CONSOLE, sockFd, buffer, 
                            buffSize, timeout, 
                            0x01, PTP_RECEIVE, "zReceiveAnswer" ) )
        {
          return;
        }

        /* if data is encryped decrypt */
        if( details.u.encrypt )
        {
          zDecrypt( ( uchar * ) buffer, tmpBuffSize );
        }
 
#if !PTP_IMANAGER
        /* send ack to replier */
        *buffSize = 0x01;
        ( void ) zPutGetStream( retCode, PTP_CONSOLE,sockFd, ack, 
                                buffSize, timeout, 0x00,
                                PTP_SEND, "zReceiveAnswer" );
#endif

        /* set buffSize to received bytes */
        *buffSize = tmpBuffSize;
}

/******************************************
 * zRecv                                  *
 * receives data from a requester         *
 * retCode PTP_INTERFACE_SUCESS           *
 *         PTP_INTERFACE_FAILURE          *
 *         PTP_INTERFACE_TIMEOUT          *
 * returns PTP_EXIT_SUCCESS               *
 *         PTP_EXIT_FAILURE               *
 ******************************************/
privateVoid zRecv( 
        long *retCode,           /* return value */
        PTP_SOCKET *sockFd,      /* connected socket channel */
        char *buffer,            /* buffer to receive data */
        long *buffSize,          /* will contain received bytes */
        const long *maxBuffSize, /* max size buffer can receive */
        const long *timeout,     /* timeout */
        long *language )         /* language of requester */
{
        long tmpBuffSize = 0x01;
        union ptpHeader details;
        long headerSize = sizeof( details.cHeader ) - 0x01;
        char ack[2] = { PTP_ACK, 0x00 };

        ( void )
        zCreateHeader( &details, PTP_MAX_MESSAGE_SIZE, PTP_MSG_FUNC_REPLIER, 
                       0x00, 0x00 );


        /* send header to gatekeeper */
        if ( zPutGetStream( retCode, PTP_CONSOLE, sockFd, details.cHeader, 
                            &headerSize, 
                            timeout, 0x01, PTP_SEND, "zRecv" ) )
        {
          return;
        }


        headerSize = sizeof ( details.cHeader ) - 0x01;

        /* receive header from requester */
        if ( zPutGetStream( retCode, PTP_CONSOLE, sockFd, details.cHeader, 
                            &headerSize, 
                            timeout, 0x01, PTP_RECEIVE, "zRecv" ) )
        {
          return;
        }

        /* send ack to requester */
        ack[0] = PTP_ACK;
        if ( zPutGetStream( retCode, PTP_CONSOLE, sockFd, ack, 
                            &tmpBuffSize, timeout, 
                            0x01, PTP_SEND, "zRecv" ) )
        {
          return;
        }

        tmpBuffSize = *buffSize = atoi( details.u.cSize );
        *language = ( long ) details.u.language; 

/*      To do text conversion based on language if needed
*/

#if PTP_IMANAGER
        m_TemporaryPointer = buffer = 
        ( char * ) malloc( ( size_t ) tmpBuffSize + 0x01 );
        if ( !m_TemporaryPointer )
        {
          zPTPLog( PTP_CONSOLE, 0x03, "zReceiveAnswer", 
                   PTP_LOG_LEVEL_ERROR, 0x00, 
                   "%s\nSource: %s V%s , Compiled: %s %s Line: %d\n", 
                   "failed/dynamic memory exhausted",
                   __FILE__, PTP_VER, __DATE__, __TIME__, __LINE__ );

          zCleanup ( PTP_CONSOLE, 0x03, 0x01, sockFd, 0x00 );
          *retCode = PTP_INTERFACE_FAILURE;
          return;
        }

        buffer[0x00] = '\x00';

#else
        
        /* check bytes to receive does not exceed max size */

        buffer[0x00] = '\x00';

        if ( tmpBuffSize > *maxBuffSize )
        {
          zPTPLog( PTP_CONSOLE, 0x03, "zRecv", 
                   PTP_LOG_LEVEL_ERROR, 0x00, 
                 "%d %s %d %s %d\nSource: %s V%s , Compiled: %s %s Line: %d\n", 
                   tmpBuffSize,
                   "byte message to big for",
                   *maxBuffSize,
                   "byte receiving buffer on channel", *sockFd,
                   __FILE__, PTP_VER, __DATE__, __TIME__, __LINE__ );

          zCleanup ( PTP_CONSOLE, 0x03, 0x01, sockFd, 0x00 );
          *retCode = PTP_INTERFACE_FAILURE;
          return;
        }
#endif

        /* receive data from requester */
        if ( !zPutGetStream( retCode, PTP_CONSOLE, sockFd, buffer, 
                             buffSize, timeout, 
                             0x01, PTP_RECEIVE, "zRecv" ) )
        {
#if !PTP_IMANAGER

          *buffSize = 0x01;
           ack[0] = PTP_ACK;
          /* send ack to requester */
          ( void ) zPutGetStream( retCode, PTP_CONSOLE, sockFd, ack, 
                                  buffSize, timeout, 
                                  0x01, PTP_SEND, "zRecv" );
#endif
        }

        /* set buffsize to bytes received */
        *buffSize = tmpBuffSize;

        /* if the data is encrypted decrypt */
        if ( details.u.encrypt )
        {
          zDecrypt( ( uchar * ) buffer, *buffSize );
        }
}

/******************************************
 * zWriteMessageToDisk                    *
 * retCode PTP_INTERFACE_SUCESS           *
 *         PTP_INTERFACE_FAILURE          *
 * this routine will save the contents    *
 * to a PTP file and create a PTPACK file *
 * with the same name to tell a           *
 * distributor that data is ready to be   * 
 * sent                                   *
 * returns PTP_EXIT_SUCCESS               *
 *         PTP_EXIT_FAILURE               *
 ******************************************/
privateVoid zWriteMessageToDisk( 
        long *retCode,        /* return value */
        const char *path,     /* path to write file to */
        const char *buffer,   /* data to send */
        const long *buffSize, /* size of data to send */
        const long *language, /* language to send */
        const long *encrypt ) /* encrypt = 1, 0 = not encrypted */
{
        union ptpHeader details;
        long headerSize = sizeof( details.cHeader ) - 0x01;
        long tmpBuffSize = 0x01;
        char tmpFileName[0xFF];
        char tmpFileNameACK[0xFF];
        int bytes;
        FILE *fp;

#if W_I_N
        SYSTEMTIME sysTime;
        GUID id;

        GetSystemTime( &sysTime );

        if ( CoCreateGuid(&id) )
        {
          *retCode = PTP_INTERFACE_FAILURE;
          zPTPLog( 
                PTP_CONSOLE, 0x03, "zWriteMessageToDisk", 
                PTP_LOG_LEVEL_ERROR, 0x00, 
                "%s \nSource: %s V%s , Compiled: %s %s Line: %d\n", 
                "Error creating GUID for PTP_SendSafeRequest", 
                __FILE__, PTP_VER, __DATE__, __TIME__, __LINE__ 
              );
          return;
        }

        bytes = sprintf( tmpFileName,
                         "%s%d%d%d%d%d%d%d%d%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X.PTP\0",
                         path,
                         sysTime.wYear,
                         sysTime.wMonth,
                         sysTime.wDayOfWeek,
                         sysTime.wDay,
                         sysTime.wHour,
                         sysTime.wMinute,
                         sysTime.wSecond,
                         sysTime.wMilliseconds,
                         id.Data1,
                         id.Data2,
                         id.Data3,
                         id.Data4[0],
                         id.Data4[1],
                         id.Data4[2],
                         id.Data4[3],
                         id.Data4[4],
                         id.Data4[5],
                         id.Data4[6],
                         id.Data4[7] );

#elif V_M_S

        int rc;
        char tmp[0xFF];
        long ltime[0x02];
        unsigned short int timBuf [0x07];
        rc = sys$gettim( &ltime );

        if ( !(rc & 0x01) )
        {
          *retCode = PTP_INTERFACE_FAILURE;
          zPTPLog( 
                PTP_CONSOLE, 0x03, "zWriteMessageToDisk", 
                PTP_LOG_LEVEL_ERROR, 0x00, 
                "%s \nSource: %s V%s , Compiled: %s %s Line: %d\n", 
                "Error calling sys$gettim for PTP_SendSafeRequest", 
                __FILE__, PTP_VER, __DATE__, __TIME__, __LINE__ 
              );
          return;
        }

        rc = sys$numtim( &timBuf, &ltime );

        if ( !(rc & 0x01) )
        {
          *retCode = PTP_INTERFACE_FAILURE;
          zPTPLog( 
                PTP_CONSOLE, 0x03, "zWriteMessageToDisk", 
                PTP_LOG_LEVEL_ERROR, 0x00, 
                "%s \nSource: %s V%s , Compiled: %s %s Line: %d\n", 
                "Error calling sys$numtim for PTP_SendSafeRequest", 
                __FILE__, PTP_VER, __DATE__, __TIME__, __LINE__ 
              );
          return;
        }

        ( void ) tmpnam( tmp );

        bytes = sprintf( tmpFileName, "%s%d%d%d%d%d%d%d%s%08o.PTP\0", 
                         path,
                         timBuf[0x00],
                         timBuf[0x01],
                         timBuf[0x02],
                         timBuf[0x03],
                         timBuf[0x04],
                         timBuf[0x05],
                         timBuf[0x06],
                         tmp,
                         getpid() );

#else
        char tmp[0xFF];
        time_t ticks;
        ticks = time( ( time_t * ) NULL );
        ( void ) tmpnam( tmp );

        bytes = sprintf( tmpFileName, "%s%ld%s%08o.PTP\0", 
                         path,
                         ( long ) ticks, 
                         tmp,
                         getpid() );
#endif
                               
        *retCode = PTP_INTERFACE_FAILURE;

        if ( !(fp = fopen( tmpFileName, "wb" )) )
        { 
          zPTPLog( 
                PTP_CONSOLE, 0x03, "zWriteMessageToDisk", 
                PTP_LOG_LEVEL_ERROR, 0x00, 
                "%s %s\nSource: %s V%s , Compiled: %s %s Line: %d\n", 
                "Error opening file for PTP_SendSafeRequest with filename", 
                tmpFileName, 
                __FILE__, PTP_VER, __DATE__, __TIME__, __LINE__ 
              );
          return;
        }

        if ( zCreateHeader( &details, *buffSize, PTP_MSG_FUNC_REQUESTER, 
                           *encrypt, 
                           *language ) )
        {
          return;
        }

        if ( !(fwrite( details.cHeader, headerSize, 0x01, fp )) )
        {
          zPTPLog( 
                PTP_CONSOLE, 0x03, "zWriteMessageToDisk", 
                PTP_LOG_LEVEL_ERROR, 0x00, 
                "%s %s\nSource: %s V%s , Compiled: %s %s Line: %d\n", 
             "Error writing header data for PTP_SendSafeRequest with filename", 
                tmpFileName, 
                __FILE__, PTP_VER, __DATE__, __TIME__, __LINE__ 
              );
          return;
        }

        if ( ferror( fp ) )
        {
          zPTPLog( 
                PTP_CONSOLE, 0x03, "zWriteMessageToDisk", 
                PTP_LOG_LEVEL_ERROR, 0x00, 
                "%s %s\nSource: %s V%s , Compiled: %s %s Line: %d\n", 
             "Error writing header data for PTP_SendSafeRequest with filename", 
                tmpFileName, 
                __FILE__, PTP_VER, __DATE__, __TIME__, __LINE__ 
              );
          return;
        }

        if ( !(fwrite( buffer, *buffSize, 0x01, fp )) )
        {
          zPTPLog( 
                PTP_CONSOLE, 0x03, "zWriteMessageToDisk", 
                PTP_LOG_LEVEL_ERROR, 0x00, 
                "%s %s\nSource: %s V%s , Compiled: %s %s Line: %d\n", 
            "Error writing message data for PTP_SendSafeRequest with filename", 
                tmpFileName, 
                __FILE__, PTP_VER, __DATE__, __TIME__, __LINE__ 
              );
          return;
        }

        if ( ferror( fp ) )
        {
          zPTPLog( 
                PTP_CONSOLE, 0x03, "zWriteMessageToDisk", 
                PTP_LOG_LEVEL_ERROR, 0x00, 
                "%s %s\nSource: %s V%s , Compiled: %s %s Line: %d\n", 
            "Error writing message data for PTP_SendSafeRequest with filename", 
                tmpFileName, 
                __FILE__, PTP_VER, __DATE__, __TIME__, __LINE__ 
              );
          return;
        }

        if ( fflush( fp ) )
        {
          zPTPLog( 
                PTP_CONSOLE, 0x03, "zWriteMessageToDisk", 
                PTP_LOG_LEVEL_ERROR, 0x00, 
                "%s %s\nSource: %s V%s , Compiled: %s %s Line: %d\n", 
                "Error flushing file for PTP_SendSafeRequest with filename", 
                tmpFileName, 
                __FILE__, PTP_VER, __DATE__, __TIME__, __LINE__ 
              );
          return;
        }

        if ( fclose( fp ) )
        {
          zPTPLog( 
                PTP_CONSOLE, 0x03, "zWriteMessageToDisk", 
                PTP_LOG_LEVEL_ERROR, 0x00, 
                "%s %s\nSource: %s V%s , Compiled: %s %s Line: %d\n", 
                "Error closing file for PTP_SendSafeRequest with filename", 
                tmpFileName, 
                __FILE__, PTP_VER, __DATE__, __TIME__, __LINE__ 
              );
          return;
        }

        sprintf( tmpFileNameACK, "%sACK\0", tmpFileName );

        if ( !(fp = fopen( tmpFileNameACK, "wb" )) )
        {  
          zPTPLog( 
                PTP_CONSOLE, 0x03, "zWriteMessageToDisk", 
                PTP_LOG_LEVEL_ERROR, 0x00, 
                "%s %s\nSource: %s V%s , Compiled: %s %s Line: %d\n", 
                "Error opening file for PTP_SendSafeRequest with filename", 
                tmpFileNameACK, 
                __FILE__, PTP_VER, __DATE__, __TIME__, __LINE__ 
              );
          return;
        }

        /* 
          do not check the status of fclose as the .PTPACK file has been
          created so distributor will act on it
        */

        ( void ) fclose( fp );

        *retCode = PTP_INTERFACE_SUCCESS;

}

/******************************************
 * zSend                                  *
 * sends data to a replier                *
 * retCode PTP_INTERFACE_SUCESS           *
 *         PTP_INTERFACE_FAILURE          *
 *         PTP_INTERFACE_TIMEOUT          *
 * returns PTP_EXIT_SUCCESS               *
 *         PTP_EXIT_FAILURE               *
 ******************************************/
privateVoid zSend( 
        long *retCode,        /* return value */
        PTP_SOCKET *sockFd,   /* connected channel */
        const char *buffer,   /* data to send */
        const long *buffSize, /* size of data to send */
        const long *timeout,  /* timeout */
        const long *language, /* language to send */
        const long *encrypt,  /* encrypt = 1, 0 = not encrypted */
        const uchar funk )    /* requester / replier */
{
        union ptpHeader details;
        long headerSize = sizeof( details.cHeader ) - 0x01;
        long tmpBuffSize = 0x01;
        char ack[2];

        if ( zCreateHeader( &details, *buffSize, funk, *encrypt, 
                           *language ) )
        {
          zCleanup ( PTP_CONSOLE, 0x03, 0x00, sockFd, 0x00 );
          *retCode = PTP_INTERFACE_FAILURE;
          return;
        }

        /* send header to gatekeeper */
        if ( zPutGetStream( retCode, PTP_CONSOLE, sockFd, details.cHeader, 
                            &headerSize, 
                            timeout, 0x01, PTP_SEND, "zSend" ) )
        {
          return;
        }

        /* get ack from replier return on error */
        if ( zPutGetStream( retCode, PTP_CONSOLE, sockFd, ack, 
                            &tmpBuffSize, timeout, 0x01,
                            PTP_RECEIVE, "zSend" ) )
        {
          return;
        }
 
        /* check ack from replier */
        if ( ack[0] != PTP_ACK )
        {
          *retCode = PTP_INTERFACE_FAILURE;

          zPTPLog( PTP_CONSOLE, 0x03, "zSend", 
                   PTP_LOG_LEVEL_INFORMATION, 0x00, 
                   "%s %d\nSource: %s V%s , Compiled: %s %s Line: %d\n", 
                   "header NAK on channel", *sockFd,
                   __FILE__, PTP_VER, __DATE__, __TIME__, __LINE__ );

          zCleanup ( PTP_CONSOLE, 0x03, 0x00, sockFd, 0x00 );

          return;
        }

        /* if encryption asked for encrypt */
        if ( *encrypt )
        {
          zEncrypt( ( uchar * ) buffer, *buffSize );
        }

        /* send data to replier */
        if ( zPutGetStream( retCode, PTP_CONSOLE, sockFd, ( char * ) buffer, 
                            ( long * ) buffSize, timeout, 0x01, 
                            PTP_SEND, "zSend" ) )
        {
          return;
        }

        /* get ack from replier return on error*/
        tmpBuffSize = 0x01;
        if ( !zPutGetStream( retCode, PTP_CONSOLE, sockFd, ack, 
                             &tmpBuffSize, timeout, 0x01, 
                             PTP_RECEIVE, "zSend" ) )
        {
          /* check ack contents */
          if ( ack[0] == PTP_ACK )
          {
            *retCode = PTP_INTERFACE_SUCCESS;
          }
          else /* error so disconnect */
          {
            *retCode = PTP_INTERFACE_FAILURE;
            zPTPLog( PTP_CONSOLE, 0x03, "zSend", 
                     PTP_LOG_LEVEL_INFORMATION, 0x00, 
                     "%s %d\nSource: %s V%s , Compiled: %s %s Line: %d\n", 
                     "buffer NAK on channel", *sockFd,
                     __FILE__, PTP_VER, __DATE__, __TIME__, __LINE__ );

            zCleanup ( PTP_CONSOLE, 0x03, 0x00, sockFd, 0x00 );
          }
        } 

}


/******************************************
 * PTP_Server API function                *
 * gatekeeper function                    *
 * retCode PTP_INTERFACE_SUCESS           *
 *         PTP_INTERFACE_FAILURE          *
 * returns void                           *
 ******************************************/
PTPAPI PTP_Server( 
        long *retCode,                 /* return code */
        const char *configLocation )   /* config file path location and name */
{
        PTP_OPTVAL optVal = 0x01; /* used to set socket properties */

        PTP_SOCKET connSockFd,        /* client socket descriptor */
                   listenSockFd;      /* listen socket descriptor for server */
 
        TM *t, savedTm;               /* used to get syste time for */
        time_t sec;                   /* GateKeeper log file name */

        int bytes,                    /* byte count of received ip packets */
            alive = 0x01,             /* set state of server to alive */
            refuse = 0x00,            /* refuse new connections */
            outLevel = 0x00,          /* outLevel for logging set in config */
            trace,                    /* trace messages set in config */
            connectionCount = 0x00,   /* connectionCount counter */
            totalConnections = 0x00,  /* total connections counter */
            errorsAllowed,            /* errors allowed set in config */
            errorCount = 0x00,        /* error counter */
            distributor,              /* type of server distributor or
                                         request/reply */
            securityHostCount = 0x00, /* number of allowed hosts in config */
    openSecurity = 0x00;      /* no security - any host allowed */

        register i;               /* counter for inbound security hosts check */

        long sendSize,            /* send size when forwarding packets */
             msgTimeout;          /* timeout for forwarding packets in config*/ 

        uint maxFd;               /* descriptor counter for select */

        FILE *fp = stdout;        /* file pointer set to console */

        fd_set rSet,              /* fd set for select statement */
               allSet,            /* fd set for select statement */
               exceptSet;         /* fd set for exceptions in select */

        /* timeout for select interval */
        struct timeval tv = { 0x00, 0x00 }, *pTv = NULL;

        ushort TCPPort;             /* IP port to listen on */

        ISOCKET clientAddr,         /* connected client address */ 
                servAddr,           /* server address */
                securityAddr[0x0B]; /* security addresses */

        /* registers for link list */
        auto PTP_MESSAGE *pairQ, *messageQ, *lastQ = NULL, *saveQ;

        /* ack sent when a connection arrives */
        char ack[2] = { PTP_ACK, 0x00 };

        /* type of connection */
        char *type[] = { "Unknown",
                         "Replier",
                         "Requester" };
                            
        char ownHost[0x50],                  /* own host name */
             ipBuffer[PTP_MAX_MESSAGE_SIZE], /* buffer to store all packets */
             timeBuf[0x50],                  /* timestamp buffer */
             *p;                             /* temporary pointer to char */ 

        /* Log file name */
        char logFileName[PTP_PARAM_LINE]            = "Logging\0";

/* config string variables */
        char cfgTCPPort[PTP_PARAM_LINE]             = "TCP/IP port\0";
        char cfgOutputLevel[PTP_PARAM_LINE]         = "Output level\0";
        char cfgTraceMessages[PTP_PARAM_LINE]       = "Trace messages\0";
        char cfgLogPath[PTP_PARAM_LINE]             = "Logging\0";
        char cfgMsgTimeout[PTP_PARAM_LINE]          = "Message timeout\0";
        char cfgServerWaitTime[PTP_PARAM_LINE]      = "Server wait time\0";
        char cfgErrorsAllowed[PTP_PARAM_LINE]       = "Errors allowed\0";
        char cfgDistributor[PTP_PARAM_LINE]         = "Distributor\0";
        char cfgIpAddress[0x0C] [PTP_PARAM_LINE]    = { 
                                                      "Host\0",
                                                      "Host\0",
                                                      "Host\0",
                                                      "Host\0",
                                                      "Host\0",
                                                      "Host\0",
                                                      "Host\0",
                                                      "Host\0",
                                                      "Host\0",
                                                      "Host\0",
                                                      "Host\0",
                                                      "Host\0"
                                                      };

        /* client address length for BSD api */
        PTP_ADDRLEN clientAddrLen = sizeof ( clientAddr );

        *retCode = PTP_INTERFACE_FAILURE;

        /* get the config settings */
        if ( zConfig ( 0x14, configLocation, 
                             cfgTCPPort, cfgOutputLevel, 
                             cfgTraceMessages, cfgLogPath, cfgMsgTimeout, 
                             cfgServerWaitTime, cfgErrorsAllowed, 
                             cfgDistributor, &cfgIpAddress[0x00], 
                             &cfgIpAddress[0x01], &cfgIpAddress[0x02], 
                             &cfgIpAddress[0x03], &cfgIpAddress[0x04], 
                             &cfgIpAddress[0x05], &cfgIpAddress[0x06], 
                             &cfgIpAddress[0x07], &cfgIpAddress[0x08], 
                             &cfgIpAddress[0x09], &cfgIpAddress[0x0A],
                             &cfgIpAddress[0x0B]) 
          == PTP_EXIT_FAILURE )
        {
          return;
        }

        /* check if to log to console, to file or not at all */
        /* not console ? */
        if ( strncmp( cfgLogPath, "STDOUT", 0x04 ))
        {
          /* not to log ? */
          if ( !strncmp( cfgLogPath, "NONE", 0x04 ))
          {
            /* NONE found to do not log at all */
            fp = NULL;
          }
          else
          {
            /* Log file as config file path + GateKeeper_DD_MM_YYYY.LOG*/

            ( void ) time( &sec );
            t = localtime( &sec );
            memmove( ( TM * )&savedTm, t, sizeof( TM ) );

            strftime( timeBuf, sizeof( timeBuf ) - 0x01, "%d_%m_%Y", t );
            sprintf( logFileName, "%sGateKeeper_%s.LOG", cfgLogPath, timeBuf );

            /* create log file if not created, otherwise open with append */
            fp = fopen( logFileName, "a+" );
          }
        }

        /* setup variables using config settings (converting where needed) */

        outLevel = atoi( cfgOutputLevel );
        trace = atoi( cfgTraceMessages );
        errorsAllowed = atoi( cfgErrorsAllowed );
        distributor = atoi( cfgDistributor );
        TCPPort = ( ushort ) atoi( cfgTCPPort );
        msgTimeout = atoi( cfgMsgTimeout );
                
        zPTPLog( fp, outLevel, "", PTP_LOG_LEVEL_NOTHING, 0x00, 
                 "Source: %s V%s , Compiled: %s %s \n",
                 __FILE__, PTP_VER, __DATE__, __TIME__ ); 

        /* setup time interval for select statement */
        if ( (tv.tv_sec = atoi( cfgServerWaitTime )) > 0x00 )
        {
          pTv = &tv;
        }

        zTime( timeBuf, sizeof( timeBuf ) );

        /* log that the server is started */
        zPTPLog( fp, outLevel, "", PTP_LOG_LEVEL_NOTHING, 
                 0x00, "** %s Server started **\n", 
                 timeBuf );

        /* log the connection limit of O/S */
        zPTPLog( fp, outLevel, "", PTP_LOG_LEVEL_NOTHING, 
                 0x00, "** O/S connection limit = %d **\n", FD_SETSIZE );
                
#if W_I_N

        /* for windows only setup windows sockets checking the version */

        WORD        wVersionRequested;
        WSADATA     wsaData;
        wVersionRequested = MAKEWORD( 0x01, 0x01 ); 
        if ( WSAStartup( wVersionRequested, &wsaData ) != 0x00 ) 
        {
          zPTPLog( fp, 0x03, "PTP_Server", 
                   PTP_LOG_LEVEL_ERROR , 
                   0x00, 
                   "%s\nSource: %s V%s , Compiled: %s %s Line: %d\n", 
                   "Error initlializing Windows sockets", 
                   __FILE__, PTP_VER, __DATE__, __TIME__, __LINE__ );
          return;
        }

        /* Make sure WSACleanup will be called if not using COM API
           which does this in the destructor */

        #if !PTP_IMANAGER
          atexit( zCleanupWinSock );
        #endif

#endif

        /* setup address of port to listen on and socket type */
        servAddr.sin_family      = AF_INET;
        servAddr.sin_port        = htons( TCPPort );
        servAddr.sin_addr.s_addr = INADDR_ANY;
        
        /* create listener socket checking for errors */
        if ( (listenSockFd = socket( AF_INET, SOCK_STREAM, 0x00 )) <= 0x00 ) 
        {
          zPTPLog( fp, outLevel, "PTP_Server", 
                   PTP_LOG_LEVEL_ERROR , 0x00, 
                   "%s\nSource: %s V%s , Compiled: %s %s Line: %d\n", 
                   "Failed to create listener socket",
                   __FILE__, PTP_VER, __DATE__, __TIME__, __LINE__ );
          return;
        }

        /* set socket to reuseable so if the server is restarted
           it will not complain with bind errors,
           this does not work on some window operations systems so don't 
           exit if it fails 
        */

        if ( setsockopt( listenSockFd, SOL_SOCKET, SO_REUSEADDR, &optVal, 
                         sizeof( optVal ) ) < 0x00 )
        {           
          zPTPLog( fp, outLevel, "PTP_Server", 
                   PTP_LOG_LEVEL_ERROR , 0x00, 
                   "%s\nSource: %s V%s , Compiled: %s %s Line: %d\n", 
                   "Failed to set reuseable option on listener socket",
                   __FILE__, PTP_VER, __DATE__, __TIME__, __LINE__ );

        }

        /* bind the socket to the port settings */
        if ( bind( listenSockFd, (SOCKADDR *) &servAddr, sizeof( servAddr ) ) 
                   < 0x00 ) 
        {
          zPTPLog( fp, outLevel, "PTP_Server", 
                   PTP_LOG_LEVEL_ERROR , 0x00,  
                   "%s\nSource: %s V%s , Compiled: %s %s Line: %d\n", 
                   "Failed to bind listener socket",
                   __FILE__, PTP_VER, __DATE__, __TIME__, __LINE__ );

          zCleanup( fp, outLevel, 0x01, &listenSockFd, 0x01 );
          return;
        }
        
        /* set the socket to listen */
        if ( listen( listenSockFd, PTP_SERV_BACKLOG ) < 0x00 )
        {
          zPTPLog( fp, outLevel, "PTP_Server", 
                   PTP_LOG_LEVEL_ERROR , 
                   0x00, 
                   "%s\nSource: %s V%s , Compiled: %s %s Line: %d\n",  
                   "Failed to set listener socket passive",
                   __FILE__, PTP_VER, __DATE__, __TIME__, __LINE__ );

          zCleanup( fp, outLevel, 0x01, &listenSockFd, 0x01 );
          return;
        }

        memset( ownHost, 0x00, sizeof( ownHost ) );
        gethostname( ownHost, sizeof( ownHost ) );

        ( void ) zGetServAddr( &servAddr.sin_addr, ownHost );

        /* get the ip addresses of all the hosts that 
           are allowed to connect to this gatekeeper
        */

        for ( i=0x00; i<0x0C; i++, securityHostCount++ )
        {
          if ( !strlen( cfgIpAddress[i] ) )
          {
            break;
          }
  if ( i==0x00 && *cfgIpAddress[i] == '*' )
  {
    openSecurity = 0x01;
break;
  }
          if ( zGetServAddr( &securityAddr[i].sin_addr, cfgIpAddress[i] ) )
          {
            zPTPLog( fp, outLevel, "PTP_Server",
                     PTP_LOG_LEVEL_INFORMATION, 0x00, 
                     "%s %s\nSource: %s V%s , Compiled: %s %s Line: %d\n",
                     "DSN lookup failed for host",
                     cfgIpAddress[i],
                     __FILE__, PTP_VER, __DATE__, __TIME__, __LINE__ ); 
            securityAddr[i].sin_addr.S_un.S_addr = 0x00;
          }
        }

        clientAddrLen = sizeof( clientAddr );
              
        maxFd = listenSockFd;

        /* setup FD variables for select call */ 

        FD_ZERO ( &allSet );
        FD_ZERO ( &exceptSet );
        FD_SET ( listenSockFd, &allSet );
        FD_SET ( listenSockFd, &exceptSet );

        zPTPLog( fp, outLevel, "", PTP_LOG_LEVEL_NOTHING, 0x00,                
                 "Waiting connection on [%s] %s port: %d%s%d%s%d%s%d \n",
                 ownHost, inet_ntoa( servAddr.sin_addr ), 
                 ntohs( servAddr.sin_port ) ,
                 "\nError count = ", errorCount,
                 ",Current connection count = ", connectionCount,
                 ",Total accepted connections = ", totalConnections );

        /* now we are ready to do the reall work getting data and connections */

        /* while alive is not 0 loop */
        while( alive )
        {

          PTP_SET_ERROR( 0x01 ); /* clear error */

          /* if refuse is set loop break out of loop
             if connection count is less than 2 */

          if ( refuse )
          {
            if ( connectionCount < 0x02 )
            {
              break;
            }
          }

          rSet = allSet;

          /* wait for data or a connection to arrive */
          if ( select( maxFd + 0x01, &rSet, ( fd_set * ) NULL, 
                       &exceptSet, pTv ) <= 0x00 ) 
          {            
            /* check for error or timeout */
            if ( PTP_ERROR == PTPEPERM ) 
            {
              PTP_SET_ERROR( 0x00 );
            }
            if ( PTP_ERROR == PTPEINTR )
            {
              continue;
            }
            if ( PTP_ERROR == 0x00 ) /* timeout so check date of log file */
            {
              if ( fp!=PTP_CONSOLE && fp!=NULL ) 
              {
                time( &sec );
                t = localtime( &sec );

                /* If the date has changed open a new log file */
                if( t->tm_year != savedTm.tm_year ||
                    t->tm_mon  != savedTm.tm_mon  ||
                    t->tm_mday != savedTm.tm_mday )
                {
                  strftime( timeBuf, sizeof( timeBuf ) - 0x01, "%d_%m_%Y", t );
                  sprintf( logFileName, "%sGateKeeper_%s.LOG", cfgLogPath, 
                           timeBuf );
                  fp = freopen( logFileName, "a+", fp );
                  memmove( ( TM * )&savedTm, t, sizeof( TM ) );
                }
              } 
            }
            else /* error in select clause */
            {
              zPTPLog( fp, outLevel, "PTP_Server", 
                       PTP_LOG_LEVEL_ERROR, 0x00, 
                       "%s\nSource: %s V%s , Compiled: %s %s Line: %d\n", 
                       "Failure in select clause",
                       __FILE__, PTP_VER, __DATE__, __TIME__, __LINE__ );

              /* check error count with config settings
                 and if exceeded allowed errors exit now */
              if( (++errorCount) >= errorsAllowed ) 
              {
                alive = 0x00;
              }
              continue;
            }

          } /* Select  */

          /* check for exception in select */
          if ( FD_ISSET ( listenSockFd, &exceptSet ) ) 
          {
            zPTPLog( fp, outLevel, "PTP_Server", 
                     PTP_LOG_LEVEL_ERROR,
                     0x00,  
                     "%s\nSource: %s V%s , Compiled: %s %s Line: %d\n", 
                     "Exception in clause error",
                     __FILE__, PTP_VER, __DATE__, __TIME__, __LINE__ );

              /* check error count with config settings
                 and if exceeded allowed errors exit now */
            if( (++errorCount) >= errorsAllowed ) 
            {
              alive = 0x00;
            }
            continue;
          }

          /* connection arrived ? */
          if ( FD_ISSET ( listenSockFd, &rSet ) ) 
          {

            /* accept the connect */
            connSockFd = accept( listenSockFd, (SOCKADDR *) &clientAddr, 
                                 &clientAddrLen );

            /* check for error */
            if ( connSockFd <= 0x00 )
            {
              zPTPLog( fp, outLevel, "PTP_Server", 
                       PTP_LOG_LEVEL_ERROR , 0x00, 
                       "%s\nSource: %s V%s , Compiled: %s %s Line: %d\n", 
                       "Failed to accept client connection",
                       __FILE__, PTP_VER, __DATE__, __TIME__, __LINE__ );
               
              /* If any of these occur it either means out of memory
                 or the connection count has not exceeded
                 FD_SETSIZE but has exceeded the amount of allowed
                 descriptors. So to minimize distruption refuse all new
                 connections,  process current requests and shutdown
                 when no connections exist.
              */

              if ( PTP_ERROR == PTPEMFILE ||
                   PTP_ERROR == PTPENOMEM ||
                   PTP_ERROR == PTPENFILE ||
                   PTP_ERROR == PTPENOBUFS )
              {
                refuse = 0x01;
                continue;
              }
              else
              {
                /* unknown error so check error count and continue */
                if( (++errorCount) >= errorsAllowed ) 
                {
                  alive = 0x00;
                }
                continue;
              }
            }

            totalConnections++;

            if ( !refuse )
            {
              zPTPLog( fp, outLevel, "", PTP_LOG_LEVEL_NOTHING, 0x00, 
                       "Accepted connection from %s port: %d on channel %d\n",
                       inet_ntoa( clientAddr.sin_addr ), 
                       ntohs( clientAddr.sin_port ), connSockFd );

            }
            else
            {
              /* refuse new connections when in shutdown wait state */
              if ( refuse )
              {
                zPTPLog( fp, outLevel, "", PTP_LOG_LEVEL_NOTHING, 0x00, 
                         "Refused connection from %s port: %d on channel %d%s",
                         inet_ntoa( clientAddr.sin_addr ), 
                         ntohs( clientAddr.sin_port ), connSockFd,
                         "\nas shutdown wait in progress\n" );
                FD_CLR ( connSockFd, &allSet );
                zCleanup ( fp, outLevel, 0x01, &connSockFd, 0x00 );
                continue;
              }
            }

            if ( connectionCount + 0x02 == FD_SETSIZE )
            {
              zPTPLog( fp, outLevel, "PTP_Server", 
                       PTP_LOG_LEVEL_ERROR , 0x00, 
                       "%s\nSource: %s V%s , Compiled: %s %s Line: %d\n", 
                       "O/S max reached for select",
                       __FILE__, PTP_VER, __DATE__, __TIME__, __LINE__ );

              FD_CLR ( connSockFd, &allSet );
              zCleanup ( fp, outLevel, 0x01, &connSockFd, 0x00 );
              continue;
            }

            /* security check incomming connection if not localhost and 
   if not Open Security*/
            if ( !openSecurity &&
 clientAddr.sin_addr.S_un.S_addr != PTP_OWN_HOST &&
                 clientAddr.sin_addr.S_un.S_addr != 
                 servAddr.sin_addr.S_un.S_addr )
            {
              /* loop through hosts in config file to match the ip address */
              for( i = 0x00; i < securityHostCount; i++ )
              {
                if ( clientAddr.sin_addr.S_un.S_addr == 
                     securityAddr[i].sin_addr.S_un.S_addr )
                {
                  break;
                }
              }

              /* check if no match was found and if so disconnect connection */

              if ( i == securityHostCount )
              {
                zPTPLog( fp, outLevel, "", PTP_LOG_LEVEL_NOTHING, 
                         0x00, 
                     "Unauthorized connection from %s port: %d on channel %d\n",
                         inet_ntoa( clientAddr.sin_addr ), 
                         ntohs( clientAddr.sin_port ), connSockFd );
                FD_CLR ( connSockFd, &allSet );
                zCleanup ( fp, outLevel, 0x01, &connSockFd, 0x00 );
                continue;
              }

            }

            /* passed security check so now add connection to linked list 
               checking enough memory */

            if ( NULL==(messageQ = zAddQ( &lastQ, connSockFd )) )
            {
              zPTPLog( fp, outLevel, "PTP_Server", 
                       PTP_LOG_LEVEL_ERROR, 0x01, 
                       "%s\nSource: %s V%s , Compiled: %s %s Line: %d\n", 
                       "zAddQ failed/dynamic memory exhausted",
                       __FILE__, PTP_VER, __DATE__, __TIME__, __LINE__ );

              FD_CLR ( connSockFd, &allSet );
              zCleanup ( fp, outLevel, 0x01, &connSockFd, 0x00 );
              continue;
            }

            messageQ->p = ( char * ) &messageQ->header.cHeader;
            messageQ->getSize = sizeof( messageQ->header.cHeader ) - 0x01;
                     
            if ( connSockFd > maxFd )
            {
              maxFd = connSockFd;
            }
          
            connectionCount++;

            FD_SET ( connSockFd, &allSet );

            /* send ack to connecting requester/replier 
               this is done so that when security checks fail or other
               problems occur that the calle knows this in zConnect 
               where all failover handling is done,  without this ack
               the failure would be reported in a sending or receiving
               function which is not what is wanted 
            */
            sendSize = 0x01;
            if ( zPutGetStream( retCode, PTP_CONSOLE, 
                                &messageQ->channel, ack, &sendSize, 
                                &msgTimeout, 0x00, PTP_SEND, "PTP_Server" ) )
            {
              zPTPLog( fp, outLevel, "PTP_Server", 
                       PTP_LOG_LEVEL_INFORMATION, 0x01, 
                       "%s\nSource: %s V%s , Compiled: %s %s Line: %d\n", 
                       "Send of accept ack failed in server",
                       __FILE__, PTP_VER, __DATE__, __TIME__, __LINE__ );

              messageQ->deleteFlag = 0x01;
              goto handleNext;
            }

            zPTPLog( fp, outLevel, "", PTP_LOG_LEVEL_NOTHING, 0x00,
                     "Waiting connection on [%s] %s port: %d%s%d%s%d%s%d \n",
                     ownHost, inet_ntoa( servAddr.sin_addr ), 
                     ntohs( servAddr.sin_port ) ,
                     "\nError count = ", errorCount,
                     ",Current connection count = ", connectionCount,
                     ",Total accepted connections = ", totalConnections );

          } 
           
          /* for each link in the linked list check for data arrival
             or disconnect */
          for ( messageQ=lastQ; messageQ; ) 
          {
            /* see if data arrived on this socket */
            if ( FD_ISSET( messageQ->channel, &rSet ) ) 
            {
              /* check not to call recv with size 0 */
              if ( messageQ->getSize <= 0x00 )
              {
                messageQ->deleteFlag = 0x01;
                goto handleNext;
              }
          
              /* receive data upto getsize limit delete link on error */
              if ( (bytes = recv( messageQ->channel, ( char * ) messageQ->p, 
                                  messageQ->getSize, 0x00 ) ) <= 0x00) 
              {
                messageQ->deleteFlag = 0x01;
                goto handleNext;
              } 
          
              /* data received */
              else if ( bytes > 0x00 ) 
              {

                messageQ->p[bytes] = '\x00';
                /* output data to logfile or console  */

                if( outLevel && trace && bytes <= 100 )
                { 
 
                  zTime( timeBuf, sizeof( timeBuf ) );
                  
                  zPTPLog( fp, 0x03, "", 
                           PTP_LOG_LEVEL_NOTHING, 0x00, 
                           "%s message of %d bytes at %s on channel %d - \n",
                           type[messageQ->type], 
                           bytes, timeBuf, 
                           messageQ->channel );

                  /* Output all bytes in the message even if they contain 0 */
                  for ( i=0x00, p=messageQ->p; i<bytes; i++, *p++ )
                  {
                    zPTPLog( fp, 0x03, "", PTP_LOG_LEVEL_NOTHING, 0x00, "%c",
                             *p );
                  }                  

                  zPTPLog( fp, 0x03, "", PTP_LOG_LEVEL_NOTHING, 0x00, "\n" );

                }          

                /* keep track of bytes received */
                messageQ->bytesReceived += bytes;

                /* validate incoming message tag (first byte) 
                   delete link and disconnect if not correct */

                if ( !messageQ->validated 
                     && messageQ->header.u.ptpIdentifier != 
                        sizeof ( messageQ->header ) )
                {
                  zPTPLog( fp, outLevel, "PTP_Server", 
                           PTP_LOG_LEVEL_NOTHING, 0x00, "%s\n", 
                  "Incorrect message type / header size mismatch will clear " );
                  messageQ->deleteFlag = 0x01;
                  goto handleNext;
                }
                else if ( !messageQ->validated )
                {
                  messageQ->validated = 0x01;
                }
          
                /* check header has been received, if so process */

                if ( messageQ->bytesReceived == 
                     sizeof( messageQ->header.cHeader ) - 0x01)
                {

                  /* get type of connection requester / replier */
                  messageQ->type = messageQ->header.u.function == 
                    PTP_MSG_FUNC_REPLIER ? PTP_REPLIER : PTP_REQUESTER;
                  messageQ->p = &ipBuffer[0];

                  /* if the type is a requester and is not currently
                     sending a reply to replier or
                     if the type is a replier and is currently sending
                     a reply to a requester then set getSize to
                     the header size.  Otherwise allways use 2 as
                     2 ack's will be sent when receiving a message
                     1 for the header,  1 for the complete data
                  */
                    
                  if ( (messageQ->type == PTP_REQUESTER && 
                        !messageQ->processedQuestion) ||
                       (messageQ->type == PTP_REPLIER && 
                       messageQ->processedQuestion) )
                  {
                    messageQ->getSize = messageQ->size = 
                      atoi( messageQ->header.u.cSize ); 
                    /* get size is either the number in the header
                       or if that exceeds 64K then 64K is used.
                    */
                    messageQ->getSize = 
                      messageQ->getSize > PTP_MAX_MESSAGE_SIZE ? 
                        PTP_MAX_MESSAGE_SIZE : messageQ->getSize;
                  }

                  else
                  {
                    messageQ->getSize = messageQ->size = 0x02;
                  }
                 
                  /* get the function from the sent header */
                  switch( messageQ->header.u.function )
                  {
                    case PTP_MSG_FUNC_SHUTDOWN : /* shutdown server */
                         *retCode = PTP_INTERFACE_SUCCESS;
                         messageQ->noSize = 0x01;
                         alive = 0x00;
                         break;
                    case PTP_MSG_FUNC_SHUTDOWN_WAIT : /* shutdown server 
                                                         when no connections
                                                         exist */
                         *retCode = PTP_INTERFACE_SUCCESS;
                         messageQ->noSize = 0x01;
                         refuse = 0x01;
                         break;
                    case PTP_MSG_FUNC_REPLIER :  /* requester */
                    case PTP_MSG_FUNC_REQUESTER :   /* replier */

                         if ( !messageQ->processedQuestion )
                         {
                           messageQ->pairQ = NULL;
                           for ( pairQ=lastQ; pairQ; pairQ=pairQ->prevQ )
                           {
                             /* Select a link that is idle, 
                                contains no pair, has opposite type and 
                                where we have received its header */
                             if( pairQ->status == PTP_IDLE && 
                                 !pairQ->pairQ && pairQ->type != 
                                 messageQ->type && pairQ->type != PTP_NONE )
                             {
                               /* found a pair so get each links to 
                                  point to each other */
                               messageQ->pairQ = pairQ;
                               pairQ->pairQ = messageQ;
                               pairQ->status = pairQ->pairQ->status = PTP_BUSY;
                               break;
                             } 
                           }
                         }

                         /* check if no pair found */
                         if ( NULL == messageQ->pairQ )
                         {
                           if ( messageQ->processedQuestion )
                           {
                             zPTPLog( fp, outLevel, "PTP_Server", 
                                      PTP_LOG_LEVEL_INFORMATION, 0x00, 
                                      "%s\n", "Link broken with pair" );
                                      messageQ->deleteFlag = 0x01;
                                      goto handleNext;
                           }
                         }
                         
                         /* if a pair is found process */
                         else
                         {
                           /* check state of connection if
                              sending or receiving data */
                           if ( !messageQ->processedQuestion )
                           {
                             /* Select the pair that is the requester 
                                and send its header to the replier */
                             pairQ = messageQ->type == PTP_REQUESTER ? 
                               messageQ : messageQ->pairQ;
                           }
                           else
                           {
                             /* Select the pair that is the replier and 
                                send its header to the requester */
                             pairQ = messageQ->type == PTP_REPLIER ? 
                               messageQ : messageQ->pairQ;
                           }

                           sendSize = sizeof( pairQ->header.cHeader ) - 0x01;
                           if ( zPutGetStream( retCode, fp, 
                                               &pairQ->pairQ->channel,
                                               pairQ->header.cHeader, 
                                               &sendSize, &msgTimeout, 0x00, 
                                               PTP_SEND, "PTP_Server" ) )
                           {
                             zPTPLog( fp, outLevel, "PTP_Server", 
                                      PTP_LOG_LEVEL_NOTHING, 
                                      0x01, "%s %d\n", 
                                      "Send of header to channel",
                                      pairQ->pairQ->channel   );

                             messageQ->deleteFlag = 0x01;
                             goto handleNext;
                           }
                         }
                         break;
                   default: /* unknown function so disconnect and delete link */
                         zPTPLog( fp, outLevel, "PTP_Server", 
                                  PTP_LOG_LEVEL_NOTHING, 0x00, 
                                  "%s\n", "Unknown function will clear" );
                         messageQ->deleteFlag = 0x01;
                         goto handleNext;
                         break;
                  }
            
                  if ( !messageQ->size && !messageQ->noSize )
                  {
                    zPTPLog( fp, outLevel, "PTP_Server", 
                             PTP_LOG_LEVEL_INFORMATION, 0x00, 
                             "%s\n", "Size unknown will clear" );
                    messageQ->deleteFlag = 0x01;
                    goto handleNext;
                  }
                }
            
                /* if only par of the header received decrement
                   getSize to get the rest and increment the pointer to
                   point to the write byte in the header */
                else if ( messageQ->bytesReceived < 
                          (sizeof ( messageQ->header.cHeader ) - 0x01) )
                {
                  messageQ->p += bytes;
                  messageQ->getSize -= bytes;
                }

                else /* data is being sent after the header */
                {                  
                  messageQ->getSize -= bytes;
                  messageQ->getSize = messageQ->size - 
                    messageQ->getSize > PTP_MAX_MESSAGE_SIZE ? 
                    PTP_MAX_MESSAGE_SIZE : messageQ->getSize;

                  /* check if the pair exists */
                  if ( NULL == messageQ->pairQ )
                  {
                    zPTPLog( fp, outLevel, "PTP_Server", 
                             PTP_LOG_LEVEL_INFORMATION, 0x00, 
                             "%s\n", "Link broken with pair" );
                    messageQ->deleteFlag = 0x01;
                    goto handleNext;
                  }

                  /* forward data to the pair channel */
                  sendSize = bytes;
                  if ( zPutGetStream( retCode, fp, &messageQ->pairQ->channel, 
                                      ipBuffer, &sendSize, 
                                      &msgTimeout, 0x00, PTP_SEND,
                                      "PTP_Server" ) )
                  {
                    zPTPLog( fp, outLevel, "PTP_Server", 
                             PTP_LOG_LEVEL_INFORMATION, 0x00, 
                             "%s\n", 
                             "Send of message failed will clear" );
                    messageQ->deleteFlag = 0x01;
                    goto handleNext;
                  }

                  /* if the data has all been received reset the the link
                     so can carry on sending data on the same connection
                     without having to reconnect 
                     checks are done here according to the type of application
                     distributor/request reply in the config file.

                     As a distributor will only send and request/reply will
                     send/getreply etc the reset must be done depending
                     on the type of application */

                  if ( messageQ->bytesReceived - 
                     (sizeof( messageQ->header.cHeader ) - 0x01) == 
                      messageQ->size )
                  {

                    if ( distributor || messageQ->processedQuestion )
                    {
                      /* check if processed with cfgDistributor */
                      messageQ->processedQuestion = 0x00;
                      messageQ->p = ( char * ) &messageQ->header.cHeader;
                      messageQ->getSize = 
                        sizeof( messageQ->header.cHeader ) - 0x01;
                      messageQ->bytesReceived = 0x00; 
                      messageQ->type = PTP_NONE;
                      messageQ->status = PTP_IDLE;
                      if ( messageQ->pairQ )
                      {
                        messageQ->pairQ = NULL;
                      }
                    }
                    else
                    {
                      messageQ->processedQuestion = 0x01;
                      if( messageQ->type == PTP_REPLIER )
                      {  
                        messageQ->p = ( char * ) &messageQ->header.cHeader;
                        messageQ->getSize = 
                          sizeof( messageQ->header.cHeader ) - 0x01;
                        messageQ->bytesReceived = 0x00; 
                        messageQ->type = PTP_NONE;
                        messageQ->status = PTP_IDLE;
                      }
                      else
                      {
                        messageQ->bytesReceived = 
                          sizeof( messageQ->header.cHeader ) - 0x01;
                        messageQ->getSize = messageQ->size = 0x02;
                      }
                    }
                  } 
                }
              }
            } /* If data arived on socket */

handleNext:; /* if the deleteFlag bit is set then disconnect the channel,
                remove the link in the list and decrement connection counter */
                
            if ( messageQ->deleteFlag ) 
            {
              connectionCount--;
              FD_CLR ( messageQ->channel, &allSet );

              zPTPLog( fp, outLevel, "", PTP_LOG_LEVEL_NOTHING, 0x00, 
                       "Connection will disconnect on channel %d\n", 
                       messageQ->channel );

              zCleanup ( fp, outLevel, 0x01, &messageQ->channel , 0x00 );

              saveQ = messageQ;

              if ( messageQ = messageQ->prevQ )
              {
                zDelQ( &lastQ, messageQ->nextQ );
                continue;
              }
              else
              {
                zDelQ( &lastQ, saveQ );
                messageQ = NULL;
                continue;
              }

            }
            messageQ = messageQ->prevQ;
          } /* For */
        } /* While */

        /* Server has been shutdown or exceeded error count */
        zPTPLog( fp, outLevel, "", PTP_LOG_LEVEL_NOTHING, 
                 0x00, "*** Server closing connection ***\n" );

        /* disconnect and shutdown all connections freeing link in list */
        for ( messageQ=lastQ; messageQ; messageQ=lastQ ) 
        {
          zCleanup ( fp, outLevel, 0x01, &messageQ->channel, 0x00 );
          zDelQ( &lastQ, messageQ );
        } /* For */

        /* disconnect gatekeeper socket */
        zCleanup ( fp, outLevel, 0x00, &listenSockFd, 0x01 );

        zPTPLog( fp, outLevel, "", PTP_LOG_LEVEL_NOTHING, 0x00, 
                 "*** Server closing connection ***\n" );

        zTime( timeBuf, sizeof( timeBuf ) );
        zPTPLog( fp, outLevel, "", PTP_LOG_LEVEL_NOTHING, 0x00, 
                 "** Server ended %s **\n", timeBuf );

        /* if log was a file close it */
        if ( fp!=PTP_CONSOLE && fp!=NULL ) 
        {
          fclose( fp );
        }

        return;
}

/******************************************
 * PTP_SendRequest API function           *
 * sends a request to a replier           *
 * retCode PTP_INTERFACE_SUCESS           *
 *         PTP_INTERFACE_FAILURE          *
 * returns void                           *
 ******************************************/

PTPAPI PTP_SendRequest( 
        long *retCode,        /* retCode */
        PTP_SOCKET *sockFd,   /* connected socket channel */
        const char *buffer,   /* data to be sent */
        const long *buffSize, /* size in bytes of data to be sent */ 
        const long *timeout,  /* timeout for sending data */
        const long *language, /* language of requester */ 
        const long *encrypt ) /* 1=encrypt, 0= do not encrypt */
{
        zSend( retCode, sockFd, buffer, buffSize, timeout, language, encrypt, 
               PTP_MSG_FUNC_REQUESTER );
}

/******************************************
 * PTP_SendSafeRequest API function       *
 * saves message to disk for a            *
 * distributor to send to a replier       *
 * retCode PTP_INTERFACE_SUCESS           *
 *         PTP_INTERFACE_FAILURE          *
 * returns void                           *
 ******************************************/
PTPAPI PTP_SendSafeRequest( 
        long *retCode,        /* retCode */
        PTP_SOCKET *sockFd,   /* connected socket channel */
        const char *path,     /* file path for data to be saved in */
        const char *buffer,   /* data to be sent */
        const long *buffSize, /* size in bytes of data to be sent */ 
        const long *timeout,  /* timeout ( not used at present ) */
        const long *language, /* language of requester */
        const long *encrypt ) /* 1=encrypt, 0= do not encrypt */
{
        zWriteMessageToDisk( retCode, path, buffer, 
                             buffSize, language, encrypt );
}

/******************************************
 * PTP_SendReply API function             *
 * sends answer to a requester            *
 * retCode PTP_INTERFACE_SUCESS           *
 *         PTP_INTERFACE_FAILURE          *
 * returns void                           *
 ******************************************/
PTPAPI PTP_SendReply( 
        long *retCode,        /* retCode */
        PTP_SOCKET *sockFd,   /* connected socket channel */
        const char *buffer,   /* data to be sent */
        const long *buffSize, /* size in bytes of data to be sent */ 
        const long *timeout,  /* timeout ( not used at present ) */
        const long *language, /* language of requester */
        const long *encrypt ) /* 1=encrypt, 0= do not encrypt */
{
        zSend( retCode, sockFd, buffer, buffSize, timeout, language, 
               encrypt, PTP_MSG_FUNC_REPLIER );
}

/******************************************
 * PTP_Connect API function               *
 * connects to a gatekeeper               *
 * failover handling performed in         *
 * zConnect                               *
 * retCode PTP_INTERFACE_SUCESS           *
 *         PTP_INTERFACE_FAILURE          *
 * returns void                           *
 ******************************************/
PTPAPI PTP_Connect(    
        long *retCode,              /* retCode */
        const char *host,           /* primary gatekeeper host name */
        const long *port,           /* primary gatekeeper port number */
        const char *failoverHost,   /* failover gatekeeper host name */
        const long *failoverPort,   /* failover gatekeeper port number */
        PTP_SOCKET *sockFd,         /* connected socket channel */
        const long *timeout,        /* timeout ( not used at present ) */
        long *failed )              /* failed on previous call variable */

{
        zConnect( retCode, host, port, failoverHost, failoverPort, sockFd, 
                  timeout, failed );
}

/******************************************
 * PTP_ReceiveRequest API function        *
 * receives data from a requester         *
 * retCode PTP_INTERFACE_SUCESS           *
 *         PTP_INTERFACE_FAILURE          *
 * returns void                           *
 ******************************************/
PTPAPI PTP_ReceiveRequest( 
        long *retCode,              /* retCode */
        PTP_SOCKET *sockFd,         /* connected socket channel */
        char *buffer,               /* data to be sent */
        long *buffSize,             /* size in bytes of data received */ 
        const long *maxBuffSize,    /* max bytes that can receive */
        const long *timeout,        /* timeout */
        long *language )            /* language of requester */
{
        zRecv( retCode, sockFd, buffer, buffSize, 
               maxBuffSize, timeout, language );
}

/******************************************
 * PTP_ReceiveReply API function          *
 * receives data from a replier           *
 * retCode PTP_INTERFACE_SUCESS           *
 *         PTP_INTERFACE_FAILURE          *
 * returns void                           *
 ******************************************/
PTPAPI PTP_ReceiveReply( 
        long *retCode,              /* retCode */
        PTP_SOCKET *sockFd,         /* connected socket channel */
        char *buffer,               /* data to be sent */
        long *buffSize,             /* size in bytes of data received */ 
        const long *maxBuffSize,    /* max bytes that can receive */
        const long *timeout,        /* timeout */
        long *language )            /* language of requester */
{
        zReceiveAnswer( retCode, sockFd, buffer, buffSize, maxBuffSize, 
                        timeout,
                        language ); 
}

/******************************************
 * PTP_Shutdown API function              *
 * shutdowns a gatekeeper on localhost    *
 * the config file containes the port to  *
 * shutdown                               *
 * retCode PTP_INTERFACE_SUCESS           *
 *         PTP_INTERFACE_FAILURE          *
 * returns void                           *
 ******************************************/
PTPAPI PTP_Shutdown( 
        long *retCode,                 /* retCode */
        const long *timeout,           /* timeout for operation */
        const char *configLocation )   /* config file path location and name */
{

        char cfgTCPPort[PTP_PARAM_LINE]        = "TCP/IP port\0";
        char host[] = "localhost";
        long port;

        if ( zConfig ( 0x01, configLocation, cfgTCPPort ) 
           == PTP_EXIT_FAILURE )        
        {
          *retCode = PTP_INTERFACE_FAILURE;
          return;
        }

        port = atol( cfgTCPPort );
        zSendFunction( retCode, host, &port, timeout, 
                       PTP_MSG_FUNC_SHUTDOWN, 0x00 );
}

/******************************************
 * PTP_Cleanup API function               *
 * shutdowns and closes a connect socket  *
 * retCode PTP_INTERFACE_SUCESS           *
 *         PTP_INTERFACE_FAILURE          *
 * returns void                           *
 ******************************************/
PTPAPI PTP_Cleanup(
        PTP_SOCKET *sockFd ) /* connected socket channel */
{
        /* if there is a connection disconnect it */
        if ( *sockFd )
        {
          zCleanup( PTP_CONSOLE, 0x03, 0x01, sockFd, 0x00 );
        }
}

/******************************************
 * PTP_ParseFileHeader API function       *
 * reads the header from a file           *
 * saved in PTP_SendSafeRequest and       *
 * this function is used by the           *
 * distributor process and validates the  *
 * file to be processed                   *
 * receives the details of the header.    *
 * retCode PTP_INTERFACE_SUCESS           *
 *         PTP_INTERFACE_FAILURE          *
 * returns void                           *
 ******************************************/
PTPAPI PTP_ParseFileHeader(
        long *retCode,              /* retCode */
        const char *fileLocation,   /* full file path and name */
        long *headerSize,           /* header size */
        long *function,             /* function */
        long *buffSize,             /* data size in header */
        long *language,             /* language */
        long *encrypt )             /* encryted flag */
{
        FILE *fp;
        union ptpHeader details;
        char cHeaderSize;
        *retCode = PTP_INTERFACE_FAILURE;

        if ( !(fp = fopen( fileLocation, "rb" )) )
        {
          zPTPLog( PTP_CONSOLE, 0x03, "PTP_ParseFileHeader", 
                   PTP_LOG_LEVEL_ERROR,
                   0x00, 
                   "%s %s\nSource: %s V%s , Compiled: %s %s Line: %d\n",
                   "failed to open file to parse",
                   fileLocation, 
                   __FILE__, PTP_VER, __DATE__, __TIME__, __LINE__ ); 
          return;

        }


        if ( !(fread( &cHeaderSize, 0x01, 0x01, fp )) )
        {
          zPTPLog( PTP_CONSOLE, 0x03, "PTP_ParseFileHeader", 
                   PTP_LOG_LEVEL_ERROR,
                   0x00, 
                   "%s %s\nSource: %s V%s , Compiled: %s %s Line: %d\n",
                   "error reading first byte[header size]",
                   fileLocation, 
                   __FILE__, PTP_VER, __DATE__, __TIME__, __LINE__ ); 
                  return;
        }
        if ( ferror( fp ) )
        {
          zPTPLog( PTP_CONSOLE, 0x03, "PTP_ParseFileHeader", 
                   PTP_LOG_LEVEL_ERROR,
                   0x00, 
                   "%s %s\nSource: %s V%s , Compiled: %s %s Line: %d\n",
                   "error reading first byte in file to parse",
                   fileLocation, 
                   __FILE__, PTP_VER, __DATE__, __TIME__, __LINE__ ); 
                  
          return;
        }

        if ( feof( fp ) )
        {
          zPTPLog( PTP_CONSOLE, 0x03, "PTP_ParseFileHeader", 
                   PTP_LOG_LEVEL_ERROR,
                   0x00, 
                   "%s %s\nSource: %s V%s , Compiled: %s %s Line: %d\n",
                   "eof in reading first byte in file to parse",
                   fileLocation, 
                   __FILE__, PTP_VER, __DATE__, __TIME__, __LINE__ ); 
                  
          return;
        }

        if ( cHeaderSize != sizeof( details ) )
        {
          zPTPLog( PTP_CONSOLE, 0x03, "PTP_ParseFileHeader", 
                   PTP_LOG_LEVEL_ERROR,
                   0x00, 
                   "%s %s\nSource: %s V%s , Compiled: %s %s Line: %d\n",
                "header size mismatch / version mismatch reading file to parse",
                   fileLocation, 
                   __FILE__, PTP_VER, __DATE__, __TIME__, __LINE__ ); 
                  return;
        }

        rewind( fp );

        if ( !(fread( details.cHeader, sizeof( details ) - 0x01, 0x01, fp )) )
        {
          zPTPLog( PTP_CONSOLE, 0x03, "PTP_ParseFileHeader", 
                   PTP_LOG_LEVEL_ERROR,
                   0x00, 
                   "%s %s\nSource: %s V%s , Compiled: %s %s Line: %d\n",
                   "error reading file to parse",
                   fileLocation, 
                   __FILE__, PTP_VER, __DATE__, __TIME__, __LINE__ ); 
                  return;
        }
        if ( ferror( fp ) )
        {
          zPTPLog( PTP_CONSOLE, 0x03, "PTP_ParseFileHeader", 
                   PTP_LOG_LEVEL_ERROR,
                   0x00, 
                   "%s %s\nSource: %s V%s , Compiled: %s %s Line: %d\n",
                   "error reading file to parse",
                   fileLocation, 
                   __FILE__, PTP_VER, __DATE__, __TIME__, __LINE__ ); 
                  
          return;
        }

        if ( feof( fp ) )
        {
          zPTPLog( PTP_CONSOLE, 0x03, "PTP_ParseFileHeader", 
                   PTP_LOG_LEVEL_ERROR,
                   0x00, 
                   "%s %s\nSource: %s V%s , Compiled: %s %s Line: %d\n",
                   "eof in reading file to parse",
                   fileLocation, 
                   __FILE__, PTP_VER, __DATE__, __TIME__, __LINE__ ); 
                  
          return;
        }

        if ( fclose( fp ) )
        {
          zPTPLog( 
                PTP_CONSOLE, 0x03, "PTP_ParseFileHeader", 
                PTP_LOG_LEVEL_ERROR, 0x00, 
                "%s %s\nSource: %s V%s , Compiled: %s %s Line: %d\n", 
                "Error closing file to parse with filename", 
                fileLocation, 
                __FILE__, PTP_VER, __DATE__, __TIME__, __LINE__ 
              );
          return;
        }

        *headerSize = ( long ) details.u.ptpIdentifier;
        *function = ( long ) details.u.function;
        *buffSize = atol( details.u.cSize );
        *language = ( long ) details.u.language;
        *encrypt = ( long ) details.u.encrypt;

        *retCode = PTP_INTERFACE_SUCCESS;
                
}

/******************************************
 * PTP_GetHeaderSize API function         *
 * returns the size of ptpHeader          *
 * returns void                           *
 ******************************************/
PTPAPI PTP_GetHeaderSize(
        long *headerSize )  /* header size address */
{
        *headerSize = sizeof( union ptpHeader );
}
