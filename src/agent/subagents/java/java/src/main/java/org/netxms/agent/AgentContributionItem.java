package org.netxms.agent;

/**
 * Generic agent contribution item interface
 */
public interface AgentContributionItem
{
   /**
    * Get contribution item's name
    * 
    * @return
    */
   public String getName();

   /**
    * Get contribution item's description
    * 
    * @return
    */
   public String getDescription();

}
