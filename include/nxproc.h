#ifndef _nxproc_h_
#define _nxproc_h_

#include <nms_common.h>

/**
 * Max pipe name length
 */
#define MAX_PIPE_NAME_LEN     64

class NamedPipe;

/**
 * Client request handler
 */
typedef void (*NamedPipeRequestHandler)(NamedPipe *pipe, void *userArg);

/**
 * Named pipe class
 */
class LIBNETXMS_EXPORTABLE NamedPipe
{
private:
   TCHAR m_name[MAX_PIPE_NAME_LEN];
   HPIPE m_handle;
   bool m_isListener;
   NamedPipeRequestHandler m_reqHandler;
   void *m_userArg;
   THREAD m_serverThread;
   bool m_stop;

   NamedPipe(const TCHAR *name, HPIPE handle, bool listener, NamedPipeRequestHandler reqHandler = NULL, void *userArg = NULL)
   {
      nx_strncpy(m_name, name, MAX_PIPE_NAME_LEN);
      m_handle = handle;
      m_isListener = listener;
      m_reqHandler = reqHandler;
      m_userArg = userArg;
      m_serverThread = INVALID_THREAD_HANDLE;
      m_stop = false;
   }

   void serverThread();
   static THREAD_RESULT THREAD_CALL serverThreadStarter(void *arg);

public:
   ~NamedPipe();

   const TCHAR *name() const { return m_name; }
   HPIPE handle() { return m_handle; }
   bool isListener() const { return m_isListener; }

   void startServer();
   void stopServer();

   static NamedPipe *createListener(const TCHAR *name, NamedPipeRequestHandler reqHandler, void *userArg);
   static NamedPipe *connectTo(const TCHAR *name);
};

#endif   /* _nxproc_h_ */
