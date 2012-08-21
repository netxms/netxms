/*
** nxflowd - NetXMS Flow Collector Daemon
** Copyright (c) 2009 Raden Solutions
*/

#ifndef _nxflowd_h_
#define _nxflowd_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nms_threads.h>
#include <nxdbapi.h>
#include <ipfix.h>
#include <ipfix_col.h>
#include <mpoll.h>

#include "resource.h"
#include "messages.h"


#ifdef _WIN32
#define NXFLOWD_SERVICE_NAME     _T("nxflowd")
#define NXFLOWD_EVENT_SOURCE     _T("NetXMS NetFlow/IPFIX Collector")
#define NXFLOWD_SYSLOG_NAME      NXFLOWD_EVENT_SOURCE
#else
#define NXFLOWD_SYSLOG_NAME      _T("nxflowd")
#endif

#define IPFIX_DEFAULT_PORT       4739


//
// Application flags
//

#define AF_DAEMON          0x00000001
#define AF_DEBUG           0x00000002
#define AF_USE_SYSLOG      0x00000004
#define AF_LOG_SQL_ERRORS  0x00000008
#define AF_SHUTDOWN        0x01000000


//
// Functions
//

bool Initialize();
void Shutdown();
void Main();

bool StartCollector();
void WaitForCollectorThread();

void DbgPrintf(int level, const TCHAR *format, ...);
void DbgPrintf2(int level, const TCHAR *format, va_list args);

#ifdef _WIN32
void InitService();
void InstallFlowCollectorService(const TCHAR *pszExecName);
void RemoveFlowCollectorService();
void StartFlowCollectorService();
void StopFlowCollectorService();
void InstallEventSource(const TCHAR *pszPath);
void RemoveEventSource();

#endif


//
// Global variables
//

extern DWORD g_flags;
extern TCHAR g_listenAddress[];
extern DWORD g_tcpPort;
extern DWORD g_udpPort;
extern TCHAR g_configFile[];
extern TCHAR g_logFile[];
extern int g_debugLevel;
extern DB_HANDLE g_dbConnection;

#endif
