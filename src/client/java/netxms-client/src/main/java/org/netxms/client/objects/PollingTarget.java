package org.netxms.client.objects;

import org.netxms.client.constants.AgentCacheMode;

/**
 * Common interface for all objects that can be polled
 */
public interface PollingTarget
{
   /**
    * Get object ID.
    * 
    * @return object ID
    */
   public long getObjectId();

   /**
    * Get object name
    * 
    * @return object name
    */
   public Object getObjectName();

   /**
    * Get ifXTable usage policy.
    * 
    * @return ifXTable usage policy
    */
   public int getIfXTablePolicy();

   /**
    * Get agent cache mode.
    * 
    * @return agent cache mode
    */
   public AgentCacheMode getAgentCacheMode();

   /**
    * Get object flags.
    * 
    * @return object flags
    */
   public int getFlags();

   /**
    * Get poller node ID.
    * 
    * @return poller node ID or 0 if object cannot have poler node
    */
   public long getPollerNodeId();

   /**
    * Identify if this object can have NetXMS agent.
    * 
    * @return true if this object can have NetXMS agent
    */
   public boolean canHaveAgent();

   /**
    * Identify if this object can have network interfaces.
    * 
    * @return true if this object can have network interfaces
    */
   public boolean canHaveInterfaces();

   /**
    * Identify if this object can have poller node property.
    * 
    * @return true if this object can have poller node property
    */
   public boolean canHavePollerNode();

   /**
    * Identify if this object can use EtherNet/IP for communications.
    * 
    * @return true if this object can use EtherNet/IP for communications
    */
   public boolean canUseEtherNetIP();
}
