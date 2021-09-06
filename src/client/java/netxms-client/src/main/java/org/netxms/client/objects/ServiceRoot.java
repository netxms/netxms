/**
 * 
 */
package org.netxms.client.objects;

import org.netxms.base.NXCPMessage;
import org.netxms.client.NXCSession;

/**
 * Service root object
 */
public class ServiceRoot extends GenericObject
{
	/**
    * Create from NXCP message.
    *
    * @param msg NXCP message
    * @param session owning client session
	 */
	public ServiceRoot(final NXCPMessage msg, final NXCSession session)
	{
		super(msg, session);
	}

	/**
	 * @see org.netxms.client.objects.AbstractObject#getObjectClassName()
	 */
	@Override
	public String getObjectClassName()
	{
		return "ServiceRoot";
	}

   /**
    * @see org.netxms.client.objects.AbstractObject#isAlarmsVisible()
    */
   @Override
   public boolean isAlarmsVisible()
   {
      return true;
   }
}
