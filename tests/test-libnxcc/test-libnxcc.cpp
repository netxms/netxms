#include <nms_common.h>
#include <nms_util.h>
#include <nxcc.h>
#include <testtools.h>
#include <netxms-version.h>

NETXMS_EXECUTABLE_HEADER(test-libnxcc)

static Mutex cbLock;
static uint32_t s_nodeId;

static void DebugWriter(const TCHAR *tag, const TCHAR *format, va_list args)
{
   cbLock.lock();
   if (tag != nullptr)
      _tprintf(_T("[DEBUG/%-20s] "), tag);
   else
      _tprintf(_T("[DEBUG%-21s] "), _T(""));
   _vtprintf(format, args);
   _fputtc(_T('\n'), stdout);
   cbLock.unlock();
}

class EventHandler : public ClusterEventHandler
{
public:
   EventHandler() : ClusterEventHandler() { }
   virtual ~EventHandler() { }

   virtual void onNodeJoin(UINT32 nodeId)
   {
      _tprintf(_T("** Node joined: %d\n"), nodeId);
   }

   virtual void onNodeDisconnect(UINT32 nodeId)
   {
      _tprintf(_T("** Node disconnected: %d\n"), nodeId);
   }

   virtual void onShutdown()
   {
      _tprintf(_T("** cluster shutdown\n"));
   }

   virtual ClusterMessageProcessingResult onMessage(NXCPMessage *msg, UINT32 sourceNodeId)
   {
      if (msg->getCode() == 111)
      {
         ClusterSendResponse(sourceNodeId, msg->getId(), NXCC_RCC_SUCCESS);
         return CLUSTER_MSG_PROCESSED;
      }
      return CLUSTER_MSG_IGNORED;
   }
};

static void TestCommand()
{
   NXCPMessage msg;
   msg.setCode(111);
   int e = ClusterSendCommand(&msg);
   _tprintf(_T("TestCommand: %d errors\n"), e);
}

static void TestDirectCommand()
{
   NXCPMessage msg;
   msg.setCode(111);
   uint32_t rcc = ClusterSendDirectCommand(s_nodeId == 1 ? 2 : 1, &msg);
   _tprintf(_T("TestDirectCommand: rcc=%d\n"), rcc);
}

/**
 * main()
 */
int main(int argc, char *argv[])
{
   InitNetXMSProcess(true);

   if (argc < 2)
   {
      _tprintf(_T("Please specify node ID\n"));
      return 1;
   }

#ifdef _WIN32
   WSADATA wsaData;
   WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

   s_nodeId = strtoul(argv[1], nullptr, 0);

   Config *config = new Config();
   config->setValue(_T("/CLUSTER/NodeId"), s_nodeId);
   config->setValue(_T("/CLUSTER/PeerNode"), (s_nodeId == 1) ? _T("2:127.0.0.1") : _T("1:127.0.0.1"));

   nxlog_set_debug_writer(DebugWriter);
   AssertTrue(ClusterInit(config, _T("CLUSTER"), new EventHandler()));

   AssertTrue(ClusterJoin());
   ClusterSetRunning();
   _tprintf(_T("CLUSTER RUNNING\n"));

   ThreadSleep(1);

   TestCommand();
   TestDirectCommand();

   ThreadSleep(60);

   ClusterShutdown();
   _tprintf(_T("CLUSTER SHUTDOWN\n"));

   return 0;
}
