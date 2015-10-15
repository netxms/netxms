#ifndef _nxcc_h_
#define _nxcc_h_

#ifdef _WIN32
#ifdef LIBNXCC_EXPORTS
#define LIBNXCC_EXPORTABLE __declspec(dllexport)
#else
#define LIBNXCC_EXPORTABLE __declspec(dllimport)
#endif
#else    /* _WIN32 */
#define LIBNXCC_EXPORTABLE
#endif

#include <nms_common.h>
#include <nms_util.h>
#include <nms_threads.h>
#include <nxcpapi.h>
#include <nxconfig.h>

/**
 * Cluster node states
 */
enum ClusterNodeState
{
   CLUSTER_NODE_DOWN = 0,
   CLUSTER_NODE_CONNECTED = 1,
   CLUSTER_NODE_UP = 2
};

/**
 * Cluster node event handler
 */
class LIBNXCC_EXPORTABLE ClusterEventHandler
{
public:
   ClusterEventHandler();
   virtual ~ClusterEventHandler();
   
   virtual void onNodeJoin(UINT32 nodeId);
   virtual void onNodeDisconnect(UINT32 nodeId);
   virtual void onShutdown();
   
   virtual void onMessage(NXCPMessage *msg, UINT32 sourceNodeId);
};

/**
 * API functions
 */
bool LIBNXCC_EXPORTABLE ClusterInit(Config *config, const TCHAR *section, ClusterEventHandler *eventHandler);
bool LIBNXCC_EXPORTABLE ClusterJoin();
void LIBNXCC_EXPORTABLE ClusterShutdown();

void LIBNXCC_EXPORTABLE ClusterSetDebugCallback(void (*cb)(int, const TCHAR *, va_list));

bool LIBNXCC_EXPORTABLE ClusterIsMasterNode();

#endif
