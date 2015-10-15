package org.netxms.ui.eclipse.snmp.views.helpers;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Map;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.server.ServerVariable;
import org.netxms.client.snmp.SnmpUsmCredential;

public class SnmpConfig
{
   private List<String> communities;
   private List<SnmpUsmCredential> usmCredentials;
   private List<String> ports;

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
    * @param session communication session to use
    * @return SNMP configuration
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public static SnmpConfig load(NXCSession session) throws NXCException, IOException
   {
      SnmpConfig config = new SnmpConfig();
      
      config.communities = session.getSnmpCommunities();
      config.usmCredentials = session.getSnmpUsmCredentials();
      Map<String, ServerVariable> variables = session.getServerVariables();
      ServerVariable v = variables.get("SNMPPorts");
      config.ports = parsePorts(v != null ? v.getValue() : "" );

      return config;
   }
   
   /**
    * 
    * @param portList
    */
   public static List<String> parsePorts(String portList)
   {
      String[] arr = portList.split(",");
      List<String> list = new ArrayList<String>(Arrays.asList(arr));
      return list;      
   }
   
   public String parsePorts() 
   {
      StringBuilder str = new StringBuilder();
      for(int i = 0; i < ports.size(); i++)
      {
         str.append(ports.get(i));
         if(i != ports.size() - 1)
         {
            str.append(",");            
         }
      }
      return str.toString();
   }
   /**
    * Save SNMP configuration on server. This method calls communication
    * API directly, so it should not be called from UI thread.
    * 
    * @params session communication session to use
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void save(NXCSession session) throws NXCException, IOException
   {
      session.updateSnmpCommunities(communities);
      session.updateSnmpUsmCredentials(usmCredentials);
      session.setServerVariable("SNMPPorts", parsePorts());
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
    * @return the ports
    */
   public List<String> getPorts()
   {
      return ports;
   }
   
   /**
    * @param communities the communities to set
    */
   public void setPorts(List<String> ports)
   {
      this.ports = ports;
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
