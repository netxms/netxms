#include <nms_common.h>
#include <nms_util.h>
#include <nxcc.h>
#include <testtools.h>

static MUTEX cbLock = MutexCreate();
static UINT32 s_nodeId;

static void DebugCallback(int level, const TCHAR *format, va_list args)
{
   MutexLock(cbLock);
   _vtprintf(format, args);
   _tprintf(_T("\n"));
   MutexUnlock(cbLock);
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

   virtual bool onMessage(NXCPMessage *msg, UINT32 sourceNodeId)
   {
      if (msg->getCode() == 111)
      {
         ClusterSendResponse(sourceNodeId, msg->getId(), NXCC_RCC_SUCCESS);
      }
      return false;
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
   UINT32 rcc = ClusterSendDirectCommand(s_nodeId == 1 ? 2 : 1, &msg);
   _tprintf(_T("TestDirectCommand: rcc=%d\n"), rcc);
}

/**
 * main()
 */
int main(int argc, char *argv[])
{
   if (argc < 2)
   {
      _tprintf(_T("Please specify node ID\n"));
      return 1;
   }

#ifdef _WIN32
   WSADATA wsaData;
   WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

   s_nodeId = strtoul(argv[1], NULL, 0);

   Config *config = new Config();
   config->setValue(_T("/CLUSTER/NodeId"), s_nodeId);
   config->setValue(_T("/CLUSTER/PeerNode"), (s_nodeId == 1) ? _T("2:127.0.0.1") : _T("1:127.0.0.1"));

   ClusterSetDebugCallback(DebugCallback);
   ClusterInit(config, _T("CLUSTER"), new EventHandler());

   ClusterJoin();
   _tprintf(_T("CLUSTER RUNNING\n"));

   ThreadSleep(1);

   TestCommand();
   TestDirectCommand();

   ThreadSleep(60);

   ClusterShutdown();
   _tprintf(_T("CLUSTER SHUTDOWN\n"));

   return 0;
}
