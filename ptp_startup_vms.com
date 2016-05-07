$! CMS REPLACEMENT HISTORY, Element PTP_STARTUP.COM
$! *8    22-SEP-2003 14:40:36 RD2JK ""
$! *7    18-SEP-2003 16:12:48 RD2JK ""
$! *6    18-SEP-2003 15:14:22 RD2JK "Added functionality"
$! *5    18-AUG-2003 09:38:19 RD2JK ""
$! *4     1-AUG-2003 11:37:58 RD2JK "Added error handling"
$! *3    31-JUL-2003 13:31:38 RD2JK "Removed queue will not work, use a startup file for each node."
$! *2    31-JUL-2003 11:33:51 RD2JK "Added queue so can startup processes on different nodes in a cluster"
$! *1    11-JUL-2003 10:44:57 RD2JK "POINTTPOINT STARTUP/SHUTDOWN DCL COMMAND FILE"
$! CMS REPLACEMENT HISTORY, Element PTP_STARTUP.COM
$  save_verify  =  F$VERIFY(0,0)	 ! <save verification + set none>
$!******************************************************************************
$!
$! 	Filename:	PTP_STARTUP.COM
$!
$!      Created:        James Kyburz Maginet
$!      Date:		2003-02-13
$!
$!	Description:	Restarts PointTPoint processes
$!      
$!      Parameter:      P1 Optional "SHUTDOWN" to just shutdown
$!                      otherwise all active processes will
$!                      be stopped and new processes will be
$!                      created
$!
$!                      P1 Optional "CREATE" to just recreate COM
$!                      files for startup and shutdown
$!                      indended when creating new Gatekeepers and
$!                      Distributors.
$!
$!                      P1 Optional "CLEANUP" if this is set then
$!                         the following parameters are also needed :-
$!                         P2 Log retained value for log files
$!                         P3 Optional - if this is set log file
$!                            older than today less this value 
$!                            will be deleted
$!                         P4 Time to run                        
$!                         P5 Optional - <Default=N> Y = Resubmit
$!                            
$!                         If P5 is "Y" then
$!                         Cleanup will resubmit command file to run at the 
$!                         the time specified by P4 on the same queue, it is 
$!                         designed to clear down log files, reduce large 
$!                         dir file sizes (distributor only) and restart 
$!                         PointTPoint.
$!                     
$!******************************************************************************
$! The error handler takes two arguments
$! P1 = procedure name
$! P2 = error message
$ ON CONTROL_Y THEN GOTO CTRL_Y
$ ON ERROR THEN GOTO REPORT_ERROR
$ IF P1 .EQS. "CLEANUP"
$ THEN
$   IF P5 .EQS. "Y" 
$   THEN 
$     queueName = F$GETQUI("DISPLAY_QUEUE","QUEUE_NAME","*","THIS_JOB")
$     SUBMIT/QUEUE='queueName' -
        PTP_COM:PTP_STARTUP.COM -
        /PARAM=("''P1'","''P2'","''P3'","''P4'","''P5'") -
        /NOPRINT/KEEP/NOTIFY/AFTER="TOMORROW+''P4'" -
        /LOG=PTP_LOG:
$   ENDIF
$ ENDIF
$!
$ w:=WRITE SYS$OUTPUT F$FAO
$!
$! Open the main configFile for reading
$! This file will hold all the systems GateKeeper and Distributors
$ OPEN/READ configfile PTP_DAT:PTP_STARTUP_'F$GETSYI("NODENAME")'.INI
$ LOOP:
$ READ/END=EOF configfile configrec
$ IF F$EXTRACT( 0, 1, configrec ) .EQS. "!" -
  THEN GOTO LOOP
$ GOSUB PARSE_LINE		! Get distributor/gatekeeper information
$ IF P1 .NES. "SHUTDOWN" THEN GOSUB CREATE_COMMAND  ! Create COM files
$ IF P1 .NES. "CREATE" THEN GOSUB STOP_PROCESS      ! Stop process if active
$ IF P1 .NES. "SHUTDOWN" .AND. -
     P1 .NES. "CREATE" THEN GOSUB CREATE_PROCESS    ! Create if not in shutdown
$!                                                  ! or create mode
$!
$ GOTO LOOP
$!
$ CTRL_Y:
$!
$ IF F$TYPE( ctx ) .EQS. "PROCESS_CONTEXT" THEN - ! Clear memory
         clearcontext = F$CONTEXT ( "PROCESS", ctx, "CANCEL" )
$ CLOSE/NOLOG iniFile
$!
$ EOF:
$!
$ CLOSE/NOLOG configfile
$ CLOSE/NOLOG iniFile
$ save_verify  =  F$VERIFY(save_verify) ! Set verify back to original
$!
$ EXIT $STATUS
$!
$ PARSE_LINE:    ! Parse the record from the config file
$!
$ line           = F$ELEMENT( 1, "[", configrec )
$ type           = F$EDIT( F$ELEMENT( 0, "]", line ), "COLLAPSE" )
$ line           = F$ELEMENT( 2, "[", configrec )
$ processname    = F$EDIT( F$ELEMENT( 0, "]", line ), "COLLAPSE" )
$ line           = F$ELEMENT( 3, "[", configrec )
$ username       = F$EDIT( F$ELEMENT( 0, "]", line ), "COLLAPSE" )
$ line           = F$ELEMENT( 4, "[", configrec )
$ configlocation = F$EDIT( F$ELEMENT( 0, "]", line ), "COLLAPSE" )
$!
$ ptpstartcomfile = "PTP_COM:ptp_''processname'.COM"
$ ptpruncomfile = "PTP_COM:ptp_run_''processname'_DET.COM"
$ ptpstopcomfile = "PTP_COM:ptp_stop_''processname'.COM"
$!
$ RETURN
$!
$!
$ CREATE_COMMAND: ! If START/STOP files do not exist create them
$!
$! Create startup and shutdown command files
$ IF F$SEARCH(ptpstartcomfile) .NES. "" THEN -
  DELETE/NOLOG 'ptpstartcomfile.*
$!
$ i = 0
$CREATE_LOOP:
$ function = "START/STOP"
$ IF i .EQ. 0
$ THEN
$   OPEN/WRITE rec 'ptpstartcomfile                           
$ ELSE
$   IF F$SEARCH( ptpstopcomfile ) .NES. "" THEN -
      DELETE/NOLOG 'ptpstopcomfile.*
$   OPEN/WRITE rec 'ptpstopcomfile
$ ENDIF
$ WRITE rec     "$! Generated by ''F$ENVIRONMENT(""PROCEDURE"")' on ''F$CVTIME()'"
$ WRITE rec     "$ image_ver = F$ENVIRONMENT(""VERIFY_IMAGE"")    ! save image verification"
$ WRITE rec     "$ proc_ver = F$ENVIRONMENT(""VERIFY_PROCEDURE"") ! save procedure verification"
$ WRITE rec     "$ save_prefix = F$ENVIRONMENT(""VERIFY_PREFIX"") ! save prefix"
$ WRITE rec     "$ SET PREFIX ""!8%T""                            ! set prefix"
$ IF I .EQ. 0 
$ THEN
$   WRITE rec     "$ temp = F$VERIFY(1,1)                         ! set verify"
$ ELSE
$   WRITE rec     "$ temp = F$VERIFY(0,0)                         ! set no verify"
$ ENDIF
$ WRITE rec     "$ ON ERROR THEN GOTO REPORT_ERROR              ! error handling"
$ WRITE rec     "$ ptp_exe = F$LOGICAL( ""PTP_EXE"" )"
$ WRITE rec     "$ distributor = ""$"" + PTP_EXE + ""distributor.exe""
$ WRITE rec     "$ gatekeeper  = ""$"" + PTP_EXE + ""gatekeeper.exe""
$ thisfunction = F$ELEMENT(i,"/",function)
$ IF type .EQS. "g" THEN -
$ WRITE rec     "$ gatekeeper  ''thisfunction' ''configlocation'"
$ IF type .EQS. "d" THEN -
$ WRITE rec     "$ distributor ''thisfunction' ''configlocation'"
$ WRITE rec     "$ IF .NOT. $STATUS THEN GOTO REPORT_ERROR"
$ WRITE rec     "$ GOSUB NORMAL     ! set back verify and prefix settings"
$ WRITE rec     "$ EXIT $STATUS"
$ WRITE rec     "$!"
$ WRITE rec     "$REPORT_ERROR:"
$ WRITE rec     "$ error_details = $STATUS
$ WRITE rec     "$ SET NOON"
$ WRITE rec     "$ status = error_details
$ WRITE rec     "$ message = F$MESSAGE(status)
$ WRITE rec     "$ procedure = F$ENVIRONMENT(""PROCEDURE"")"
$ WRITE rec     "$line = F$FAO(""!AS""""!AS"""" """"!AS"""""",""@PTP_COM:PTP_ERROR """ +-
                        ",F$EDIT(""' 'procedure'"",""COLLAPSE""),F$EDIT(""' 'message'"",""COLLAPSE""))"
$ WRITE rec     "$'line"
$ WRITE rec     "$!"
$ WRITE rec     "$ EXIT $STATUS"
$ WRITE rec     "$!"
$ WRITE rec     "$NORMAL:"
$ WRITE rec     "$!"
$ WRITE rec     "$ temp = F$VERIFY(image_ver,proc_ver)    ! set back verify"
$ WRITE rec      F$FAO("$ SET PREFIX ""!AS""            !! set back prefix ",F$EDIT("' 'save_prefix'","COLLAPSE"))
$ WRITE rec     "$ RETURN"
$ CLOSE/NOLOG rec
$ i = i + 1
$ IF i .LT. 2 THEN GOTO CREATE_LOOP
$!
$! Create Run command file
$ IF F$SEARCH( ptpruncomfile ) .NES. "" THEN -
  DELETE/NOLOG 'ptpruncomfile.*
$!
$ OPEN/WRITE rec 'ptpruncomfile
$ WRITE rec "$ RUN SYS$SYSTEM:LOGINOUT -            " ! Create detached process
$ WRITE rec "  /DETACHED -                          " ! Run as detched
$ WRITE rec "  /UIC = ''username' -                 " ! UIC of process
$ WRITE rec "  /INPUT=''ptpstartcomfile' -          " ! Input command file
$ WRITE rec "  /OUTPUT=PTP_LOG:''processname'.LOG - " ! SYS$OUTPUT
$ WRITE rec "  /ERROR=PTP_LOG:''processname'.ERR -  " ! SYS$ERROR
$ WRITE rec "  /PROCESS_NAME=''processname' -       " ! Process name
$ WRITE rec "  /WORKING_SET=1500 -                  " ! Max qouta
$ WRITE rec "  /MAXIMUM_WORKING_SET=3000 -          " ! Max qouta
$ WRITE rec "  /EXTENT=500 -                        " ! Max qouta
$ WRITE rec "  /SUB=1                               " ! Max qouta
$ WRITE rec "$EXIT $STATUS"
$ CLOSE/NOLOG rec
$!
$ RETURN
$!
$!
$ STOP_PROCESS:  ! Stop GateKeeper/Distributor if active
$!
$ GOSUB CHECK_PROCESS
$ IF pid .NES. ""                              
$ THEN
$    W ( "!AS - Stopping !AS ...", F$CVTIME(), processName )
$    SET NOON
$    @'ptpstopcomfile                          
$    SET ON
$ ENDIF
$!
$ IF P1 .EQS. "CLEANUP"
$ THEN
$  GOSUB CLEANUP
$ ENDIF
$!
$ RETURN
$!
$!
$ CREATE_PROCESS:  ! Create GateKeeper or Distributor process
$!
$ GOSUB CHECK_PROCESS
$ CREATE_PROCESS_LOOP:
$ IF pid .NES. ""                               ! Server already running ?
$ THEN
$     WAIT 00:00:01                             ! Wait 1 second
$     GOSUB CHECK_PROCESS                       ! Check process again
$     GOTO CREATE_PROCESS_LOOP                  ! Loop !!!
$    RETURN
$ ENDIF
$!
$ W ("!AS - Creating process !AS ...", F$CVTIME(), processName )
$ SET NOON
$ @'PTPRunComFile                           
$ SET ON
$!
$!
$ RETURN
$!
$!
$ CHECK_PROCESS:                             ! Check if a process exists
$!
$ ctx = 0
$ pid = ""                                      
$ setcontext = F$CONTEXT( "PROCESS", ctx, "PRCNAM", processName, "eql" )
$!
$ IF F$TYPE( ctx ) .EQS. "PROCESS_CONTEXT" THEN -
    pid = F$PID( ctx )
$!
$ IF F$TYPE( ctx ) .EQS. "PROCESS_CONTEXT" THEN - ! Clear memory
         clearcontext = F$CONTEXT ( "PROCESS", ctx, "CANCEL" )
$!
$ RETURN
$!
$ CLEANUP:
$!
$ SET NOON    ! Turn of error handling 
$ P2 = P2 + 0
$! If p2 is set purge log files keeping p2 value files
$ IF P2 .GT. 0 THEN PURGE/LOG PTP_LOG:*PTP*.LOG /KEEP = 'P2'
$ P3 = P3 + 0
$! If p3 is set delete log files older than p3 value
$ IF P3 .GT. 0 THEN DELETE/BEFORE="-''P3'-"/LOG PTP_LOG:*PTP*.LOG.* 
$! Truncate DIR file for distributor only
$ IF type .NES. "d"
$ THEN
$   SET ON
$   RETURN
$ ENDIF
$!
$! Get the dir file for the distributor by opening the INI file
$ OPEN/READ/SHARE iniFile 'configLocation'
$ READ_LOOP: READ/END=CLEANUP_ERROR iniFile rec ! Loop until File_Path found
$ IF F$EXTRACT(0,9,rec) .NES. "File_Path" THEN GOTO READ_LOOP
$ rec = F$ELEMENT(1,"'",rec)
$ i = 0
$ dir_open = F$EXTRACT(0,1,F$ELEMENT(1,":",REC))
$ IF dir_open .EQS. "[" 
$ THEN 
$   dir_close = "]"
$ ELSE
$   dir_close = ">"
$ ENDIF
$ IF F$ELEMENT(i,".",rec) .EQS. rec
$ THEN
$   dir_name = F$ELEMENT(1,dir_open,rec) - dir_close
$   dir_full_name = rec - dir_close + ".-" + dir_close + dir_name + ".DIR;"
$ ELSE
$ DOT_LOOP:
$   chr = F$ELEMENT(i,".",rec)
$   IF chr .NES. "." 
$   THEN
$     i = i + 1
$     GOTO DOT_LOOP
$   ENDIF
$   dir_name = F$ELEMENT(i-1,".",rec) - dir_close
$   dir_full_name = rec - dir_close + ".-" + dir_close + dir_name + ".DIR;"
$ ENDIF
$ SET FILE 'dir_full_name' /TRUNC/LOG
$!
$ SET ON
$ CLOSE/NOLOG iniFile
$!
$ RETURN
$!
$ CLEANUP_ERROR: ! Only can be from Read command
$ W ( F$MESSAGE($STATUS) )
$ CLOSE/NOLOG iniFile
$!
$ RETURN
$!
$ REPORT_ERROR: ! Report any errors
$!
$ error_details = $STATUS
$ SET NOON
$ status = error_details
$ message = F$MESSAGE(status)
$ procedure = F$ENVIRONMENT("PROCEDURE")
$ @PTP_COM:PTP_ERROR "''procedure'" "''message'"
$ IF F$TYPE( ctx ) .EQS. "PROCESS_CONTEXT" THEN - ! Clear memory
         clearcontext = F$CONTEXT ( "PROCESS", ctx, "CANCEL" )
$ CLOSE/NOLOG iniFile
$ CLOSE/NOLOG configFile
$ save_verify  =  F$VERIFY(save_verify) ! Set verify back to original
$!
$ EXIT $STATUS
