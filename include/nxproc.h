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
#ifdef _WIN32
   HANDLE m_stopEvent;
#endif

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
   bool m_started;
   bool m_running;

protected:
   TCHAR *m_cmd;
   bool m_shellExec;
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
   ProcessExecutor(const TCHAR *cmd, bool shellExec = true);
   virtual ~ProcessExecutor();

   UINT32 getStreamId() const { return m_streamId; }
   const TCHAR *getCommand() const { return m_cmd; }
   pid_t getProcessId();

   virtual bool execute();
   virtual void stop();

   bool isStarted() const { return m_started; }
   bool isRunning();
   bool waitForCompletion(UINT32 timeout);
};

/**
 * Sub-process request handler
 */
typedef NXCPMessage* (*SubProcessRequestHandler)(UINT16, const void *, size_t);

/**
 * Sub-process state
 */
enum SubProcessState
{
   SP_INIT = 0,
   SP_RUNNING = 1,
   SP_COMM_FAILURE = 2,
   SP_STOPPED = 3
};

/**
 * Maximum length for sub-process name
 */
#define MAX_SUBPROCESS_NAME_LEN  16

class PipeMessageReceiver;
class MsgWaitQueue;

/**
 * Sub-process executor
 */
class LIBNETXMS_EXPORTABLE SubProcessExecutor : public ProcessExecutor
{
private:
   NamedPipe *m_pipe;
   MsgWaitQueue *m_messageQueue;
   THREAD m_receiverThread;
   SubProcessState m_state;
   VolatileCounter m_requestId;
   TCHAR m_name[MAX_SUBPROCESS_NAME_LEN];

   void receiverThread();

   static ObjectArray<SubProcessExecutor> *m_registry;
   static Mutex m_registryLock;
   static THREAD m_monitorThread;
   static CONDITION m_stopCondition;

   static THREAD_RESULT THREAD_CALL monitorThread(void *arg);
   static THREAD_RESULT THREAD_CALL receiverThreadStarter(void *arg);

public:
   SubProcessExecutor(const TCHAR *name, const TCHAR *command);
   virtual ~SubProcessExecutor();

   virtual bool execute();
   virtual void stop();

   bool sendCommand(UINT16 command, const void *data = NULL, size_t dataSize = 0, UINT32 *requestId = NULL);
   bool sendRequest(UINT16 command, const void *data, size_t dataSize, void **response, size_t *rspSize, UINT32 timeout);

   const TCHAR *getName() const { return m_name; }
   SubProcessState getState() const { return m_state; }

   static void shutdown();
};

/**
 * Initialize and run sub-process (should be called from sub-process's main())
 */
int LIBNETXMS_EXPORTABLE SubProcessMain(int argc, char *argv[], SubProcessRequestHandler requestHandler);

#endif   /* _nxproc_h_ */
