package org.netxms.client.objects;

import org.netxms.client.constants.AgentCacheMode;

/**
 * Common interface, for all objects, that can be polled 
 */
public interface PollingTarget
{
   /**
    * @return the objectId
    */
   public long getObjectId();

   public int getIfXTablePolicy();

   public AgentCacheMode getAgentCacheMode();

   public int getFlags();

   public long getPollerNodeId();

   public Object getObjectName();

   public boolean containAgent();

   public boolean containInterfaces();

   public boolean containPollerNode();   
}
