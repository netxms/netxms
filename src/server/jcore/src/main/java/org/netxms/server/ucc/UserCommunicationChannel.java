/**
 * 
 */
package org.netxms.server.ucc;

import java.util.HashSet;
import java.util.Set;
import org.netxms.server.ServerConfiguration;

/**
 * Base class for user communication channel implementations
 */
public abstract class UserCommunicationChannel
{
   private String id;
   private String displayName;
   private Set<ChannelListener> listeners = new HashSet<ChannelListener>();
   
   /**
    * Create channel instance
    * 
    * @param id 
    * @param config
    * @return new channel instance 
    */
   public UserCommunicationChannel(String id)
   {
      this.id = id;
   }
   
   /**
    * Initialize channel
    * @throws Exception on error
    */
   public void initialize() throws Exception
   {
      displayName = readConfigurationAsString("DisplayName", id);
   }
   
   /**
    * Get channel driver name
    * 
    * @return channel driver name
    */
   public abstract String getDriverName();
   
   /**
    * Get channel ID
    * 
    * @return channel ID
    */
   public String getId()
   {
      return id;
   }

   /**
    * Get channel display name
    * 
    * @return channel display name
    */
   public String getDisplayName()
   {
      return displayName;
   }

   /**
    * Check if subject field can be used by this channel
    * 
    * @return
    */
   public boolean isSubjectFieldUsed()
   {
      return false;
   }
   
   /**
    * Send message to given recipient
    * 
    * @param recipient recipient's address
    * @param subject message subject (can be null)
    * @param text message text
    * @throws Exception in case of error
    */
   public abstract void send(String recipient, String subject, String text) throws Exception;
   
   /**
    * Shutdown channel 
    */
   public void shutdown()
   {
   }
   
   /**
    * Add message listener
    * 
    * @param listener listener to add
    */
   public void addListener(ChannelListener listener)
   {
      listeners.add(listener);
   }
   
   /**
    * Remove message listener
    * 
    * @param listener listener to remove
    */
   public void removeListener(ChannelListener listener)
   {
      listeners.remove(listener);
   }
   
   /**
    * Notify listeners on incoming message
    * 
    * @param sender message sender address
    * @param text message text
    */
   protected void notifyListeners(String sender, String text)
   {
      for(ChannelListener l : listeners)
         l.messageReceived(sender, text);
   }
   
   /**
    * Helper for reading configuration value as string
    * 
    * @param name configuration variable name (without prefix)
    * @param defaultValue default value
    * @return configuration value
    */
   protected String readConfigurationAsString(String name, String defaultValue)
   {
      return ServerConfiguration.readAsString("UCC." + id + "." + name, defaultValue);
   }
   
   /**
    * Helper for reading configuration value as integer
    * 
    * @param name configuration variable name (without prefix)
    * @param defaultValue default value
    * @return configuration value
    */
   protected int readConfigurationAsInteger(String name, int defaultValue)
   {
      return ServerConfiguration.readAsInteger("UCC." + id + "." + name, defaultValue);
   }
   
   /**
    * Helper for reading configuration value as boolean
    * 
    * @param name configuration variable name (without prefix)
    * @param defaultValue default value
    * @return configuration value
    */
   protected boolean readConfigurationAsBoolean(String name, boolean defaultValue)
   {
      return ServerConfiguration.readAsBoolean("UCC." + id + "." + name, defaultValue);
   }
}
