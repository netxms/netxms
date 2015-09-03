#include "libnxcc.h"

/**
 * Constructor
 */
ClusterEventHandler::ClusterEventHandler()
{
}

/**
 * Destructor
 */
ClusterEventHandler::~ClusterEventHandler()
{
}

/**
 * Node join handler
 */
void ClusterEventHandler::onNodeJoin(UINT32 nodeId)
{
}

/**
 * Node disconnect handler
 */
void ClusterEventHandler::onNodeDisconnect(UINT32 nodeId)
{
}

/**
 * Shutdown handler
 */
void ClusterEventHandler::onShutdown()
{
}

/**
 * Incoming message handler
 */
void ClusterEventHandler::onMessage(NXCPMessage *msg, UINT32 sourceNodeId)
{
}
