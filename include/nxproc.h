#ifndef _nxproc_h_
#define _nxproc_h_

#include <nms_common.h>

/**
 * Max pipe name length
 */
#define MAX_PIPE_NAME_LEN     128

/**
 * Sub-process command codes
 */
#define SPC_EXIT              0x0001
#define SPC_REQUEST_COMPLETED 0x0002
#define SPC_USER              0x0100   /* base ID for user messages */

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
   TCHAR m_user[64];

   NamedPipe(const TCHAR *name, HPIPE handle, const TCHAR *user);

public:
   ~NamedPipe();

   const TCHAR *name() const { return m_name; }
   HPIPE handle() { return m_handle; }
   const TCHAR *user();

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
   TCHAR m_user[64];

   void serverThread();
   static THREAD_RESULT THREAD_CALL serverThreadStarter(void *arg);

   NamedPipeListener(const TCHAR *name, HPIPE handle, NamedPipeRequestHandler reqHandler, void *userArg, const TCHAR *user);

public:
   ~NamedPipeListener();

   const TCHAR *name() const { return m_name; }

   void start();
   void stop();

   static NamedPipeListener *create(const TCHAR *name, NamedPipeRequestHandler reqHandler, void *userArg, const TCHAR *user = NULL);
};

/**
 * Process executor
 */
class LIBNETXMS_EXPORTABLE ProcessExecutor
{
private:
   UINT32 m_streamId;
   THREAD m_outputThread;
#ifdef _WIN32
   HANDLE m_phandle;
   HANDLE m_pipe;
#else
   pid_t m_pid;
   int m_pipe[2];
#endif
   bool m_running;

protected:
   TCHAR *m_cmd;
   bool m_sendOutput;

#ifdef _WIN32
   HANDLE getOutputPipe() { return m_pipe; }
#else
   int getOutputPipe() { return m_pipe[0]; }
#endif

   static THREAD_RESULT THREAD_CALL readOutput(void *arg);
#ifndef _WIN32
   static THREAD_RESULT THREAD_CALL waitForProcess(void *arg);
#endif

   virtual void onOutput(const char *text);
   virtual void endOfOutput();

public:
   ProcessExecutor(const TCHAR *cmd);
   virtual ~ProcessExecutor();

   UINT32 getStreamId() const { return m_streamId; }
   const TCHAR *getCommand() const { return m_cmd; }
   pid_t getProcessId();

   virtual bool execute();
   virtual void stop();

   bool isRunning();
   bool waitForCompletion(UINT32 timeout);
};

/**
 * Sub-process request handler
 */
typedef NXCPMessage* (*SubProcessRequestHandler)(NXCPMessage *);

/**
 * Sub-process state
 */
enum SubProcessState
{
   SP_INIT = 0,
   SP_RUNNING = 1,
   SP_STOPPED = 2
};

/**
 * Sub-process executor
 */
class LIBNETXMS_EXPORTABLE SubProcessExecutor : public ProcessExecutor
{
private:
   NamedPipe *m_pipe;
   SubProcessState m_state;

   static ObjectArray<SubProcessExecutor> *m_registry;
   static Mutex m_registryLock;
   static THREAD m_monitorThread;

   static THREAD_RESULT THREAD_CALL monitorThread(void *arg);

public:
   SubProcessExecutor(const TCHAR *name);
   virtual ~SubProcessExecutor();

   SubProcessState getState() const { return m_state; }
};

#endif   /* _nxproc_h_ */
