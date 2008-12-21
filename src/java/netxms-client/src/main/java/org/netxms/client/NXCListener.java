/**
 * NetXMS client library notification listener
 */
package org.netxms.client;

/**
 * @author victor
 *
 */
abstract public class NXCListener
{
	public NXCListener()
	{
	}
	
	abstract public void notificationHandler(NXCNotification n);
}
