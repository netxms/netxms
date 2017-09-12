#ifndef _nxproc_h_
#define _nxproc_h_

#include <nms_common.h>

/**
 * Max pipe name length
 */
#define MAX_PIPE_NAME_LEN     128

/**
 * Named pipe class
 */
class LIBNETXMS_EXPORTABLE NamedPipe
{
   friend class NamedPipeListener;

private:
   TCHAR m_name[MAX_PIPE_NAME_LEN];
   HPIPE m_handle;
   MUTEX m_writeLock;

   NamedPipe(const TCHAR *name, HPIPE handle);

public:
   ~NamedPipe();

   const TCHAR *name() const { return m_name; }
   HPIPE handle() { return m_handle; }

   bool write(const void *data, size_t size);

   static NamedPipe *connect(const TCHAR *name, UINT32 timeout = 1000);
};

/**
 * Client request handler
 */
typedef void (*NamedPipeRequestHandler)(NamedPipe *pipe, void *userArg);

/**
 * Named pipe listener (server)
 */
class LIBNETXMS_EXPORTABLE NamedPipeListener
{
private:
   TCHAR m_name[MAX_PIPE_NAME_LEN];
   HPIPE m_handle;
   NamedPipeRequestHandler m_reqHandler;
   void *m_userArg;
   THREAD m_serverThread;
   bool m_stop;

   void serverThread();
   static THREAD_RESULT THREAD_CALL serverThreadStarter(void *arg);

   NamedPipeListener(const TCHAR *name, HPIPE handle, NamedPipeRequestHandler reqHandler, void *userArg);

public:
   ~NamedPipeListener();

   const TCHAR *name() const { return m_name; }

   void start();
   void stop();

   static NamedPipeListener *create(const TCHAR *name, NamedPipeRequestHandler reqHandler, void *userArg);
};

#endif   /* _nxproc_h_ */
