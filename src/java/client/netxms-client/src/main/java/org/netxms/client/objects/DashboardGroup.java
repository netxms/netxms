package org.netxms.client.objects;

import org.netxms.base.NXCPMessage;
import org.netxms.client.NXCSession;

/**
 * Dashboard group object
 */
public class DashboardGroup extends GenericObject
{

   /**
    * @param msg Message to create object from
    * @param session Associated client session
    */
   public DashboardGroup(NXCPMessage msg, NXCSession session)
   {
      super(msg, session);
   }
   
   /* (non-Javadoc)
    * @see org.netxms.client.objects.AbstractObject#getObjectClassName()
    */
   @Override
   public String getObjectClassName()
   {
      return "DashboardGroup";
   }

}
