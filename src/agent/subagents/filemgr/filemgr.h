
#include <nms_common.h>
#include <nms_agent.h>
#include <nxclapi.h>
#include <nxstat.h>


/**
 * File monitoring
 */

struct MONITORED_FILE
{
   TCHAR fileName[MAX_PATH];
   int monitoringCount;
};

struct FollowData
{
   const TCHAR *pszFile;
   const TCHAR *fileId;
   long offset;
	UINT32 serverAddress;
	AbstractCommSession *session;
};

class MonitoredFileList
{
private:
   MUTEX m_mutex;
   ObjectArray<MONITORED_FILE>  m_monitoredFiles;
   MONITORED_FILE* m_newFile;

public:
   MonitoredFileList();
   ~MonitoredFileList();
   void addMonitoringFile(const TCHAR *fileName);
   bool checkFileMonitored(const TCHAR *fileName);
   bool removeMonitoringFile(const TCHAR *fileName);

private:
   void Lock();
   void Unlock();
};

extern MonitoredFileList g_monitorFileList;

THREAD_RESULT THREAD_CALL SendFileUpdatesOverNXCP(void *arg);
