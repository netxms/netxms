package org.netxms.agent;

/**
 * Generic agent contribution item interface
 */
public interface AgentContributionItem
{
   /**
    * Get contribution item's name
    * 
    * @return contribution item's name
    */
   public String getName();

   /**
    * Get contribution item's description
    * 
    * @return contribution item's description
    */
   public String getDescription();

}
