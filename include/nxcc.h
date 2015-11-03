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
 * Cluster error codes
 */
#define NXCC_RCC_SUCCESS         0
#define NXCC_RCC_INVALID_NODE    1
#define NXCC_RCC_TIMEOUT         2
#define NXCC_RCC_COMM_FAILURE    3
#define NXCC_RCC_NOT_MASTER      4
#define NXCC_RCC_INVALID_REQUEST 5

/**
 * Cluster node states
 */
enum ClusterNodeState
{
   CLUSTER_NODE_DOWN = 0,
   CLUSTER_NODE_CONNECTED = 1,
   CLUSTER_NODE_SYNC = 2,
   CLUSTER_NODE_UP = 3
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
   virtual void onNodeUp(UINT32 nodeId);
   virtual void onNodeDisconnect(UINT32 nodeId);
   virtual void onShutdown();
   
   virtual bool onMessage(NXCPMessage *msg, UINT32 sourceNodeId);
};

/**
 * API functions
 */
bool LIBNXCC_EXPORTABLE ClusterInit(Config *config, const TCHAR *section, ClusterEventHandler *eventHandler);
bool LIBNXCC_EXPORTABLE ClusterJoin();
void LIBNXCC_EXPORTABLE ClusterSetRunning();
void LIBNXCC_EXPORTABLE ClusterShutdown();

void LIBNXCC_EXPORTABLE ClusterSetDebugCallback(void (*cb)(int, const TCHAR *, va_list));

UINT32 LIBNXCC_EXPORTABLE ClusterGetLocalNodeId();
UINT32 LIBNXCC_EXPORTABLE ClusterGetMasterNodeId();
bool LIBNXCC_EXPORTABLE ClusterIsMasterNode();
bool LIBNXCC_EXPORTABLE ClusterIsSyncNeeded();
bool LIBNXCC_EXPORTABLE ClusterAllNodesConnected();

void LIBNXCC_EXPORTABLE ClusterNotify(NXCPMessage *msg);
void LIBNXCC_EXPORTABLE ClusterNotify(INT16 code);
void LIBNXCC_EXPORTABLE ClusterDirectNotify(UINT32 nodeId, INT16 code);
int LIBNXCC_EXPORTABLE ClusterSendCommand(NXCPMessage *msg);
UINT32 LIBNXCC_EXPORTABLE ClusterSendDirectCommand(UINT32 nodeId, NXCPMessage *msg);
NXCPMessage LIBNXCC_EXPORTABLE *ClusterSendDirectCommandEx(UINT32 nodeId, NXCPMessage *msg);
void LIBNXCC_EXPORTABLE ClusterSendResponse(UINT32 nodeId, UINT32 requestId, UINT32 rcc);
void LIBNXCC_EXPORTABLE ClusterSendResponseEx(UINT32 nodeId, UINT32 requestId, NXCPMessage *msg);

#endif
