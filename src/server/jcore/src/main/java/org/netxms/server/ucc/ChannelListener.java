/**
 * 
 */
package org.netxms.server.ucc;

/**
 * Interface for user communication channel listener
 */
public interface ChannelListener
{
   public void messageReceived(String sender, String text);
}
