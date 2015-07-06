package org.netxms.ui.eclipse.snmp.views.helpers;

import java.io.IOException;
import java.util.List;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.snmp.SnmpUsmCredential;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

public class SnmpConfig
{
   private List<String> communities;
   private List<SnmpUsmCredential> usmCredentials;

   /**
    * Create empty object
    */
   private SnmpConfig()
   {
   }
   
   /**
    * Load SNMP configuration from server. This method directly calls
    * communication API, so it should not be called from UI thread.
    * 
    * @return SNMP configuration
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public static SnmpConfig load(NXCSession session) throws NXCException, IOException
   {
      SnmpConfig config = new SnmpConfig();
      
      config.communities = session.getSnmpCommunities();
      config.usmCredentials = session.getSnmpUsmCredentials();
      
      return config;
   }
   
   /**
    * Save SNMP configuration on server. This method calls communication
    * API directly, so it should not be called from UI thread.
    * 
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void save() throws NXCException, IOException
   {
      final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
      
      session.updateSnmpCommunities(communities);
      session.updateSnmpUsmCredentials(usmCredentials);
   }
   
   /**
    * @return the communities
    */
   public List<String> getCommunities()
   {
      return communities;
   }

   /**
    * @param communities the communities to set
    */
   public void setCommunities(List<String> communities)
   {
      this.communities = communities;
   }

   /**
    * @return the usmCredentials
    */
   public List<SnmpUsmCredential> getUsmCredentials()
   {
      return usmCredentials;
   }

   /**
    * @param usmCredentials the usmCredentials to set
    */
   public void setUsmCredentials(List<SnmpUsmCredential> usmCredentials)
   {
      this.usmCredentials = usmCredentials;
   }
   
}
