;/****************************************************************************
; Messages for NetXMS Core Agent
;****************************************************************************/
;
;#ifndef _messages_h_
;#define _messages_h_
;

MessageIdTypedef=DWORD

MessageId=1
SymbolicName=MSG_STARTING_SERVER_CORE
Language=English
Starting NetXMS Reporting Server core
.

MessageId=
SymbolicName=MSG_SERVER_STOPPED
Language=English
NetXMS Reporting Server stopped
.

MessageId=
SymbolicName=MSG_CANNOT_FIND_JRE
Language=English
Cannot find suitable Java runtime environment
.

MessageId=
SymbolicName=MSG_CANNOT_LOAD_JVM
Language=English
Unable to load JVM: %1
.

MessageId=
SymbolicName=MSG_MISSING_STARTUP_METHOD
Language=English
Cannot find server startup method
.

MessageId=
SymbolicName=MSG_MISSING_SERVER_CLASS
Language=English
Cannot find server class
.

MessageId=
SymbolicName=MSG_CREATE_JVM_FAILED
Language=English
Cannot create Java VM
.

MessageId=
SymbolicName=MSG_NO_JVM_ENTRY_POINT
Language=English
JNI_CreateJavaVM entry point not found
.

;#endif
