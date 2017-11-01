package org.netxms.ui.eclipse.snmp.views.helpers;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.netxms.client.NXCException;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.Zone;
import org.netxms.client.server.ServerVariable;
import org.netxms.client.snmp.SnmpUsmCredential;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.snmp.Activator;

public class SnmpConfig
{
   // Global SNMP config flag
   public static int SNMP_CONFIG_GLOBAL = -1;
   
   private Map<Integer, List<String>> communities;
   private Map<Integer, List<SnmpUsmCredential>> usmCredentials;
   private Map<Integer, List<String>> ports;

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
      config.ports = loadPortConfig(session);

      return config;
   }
   
   private static Map<Integer, List<String>> loadPortConfig(NXCSession session) throws IOException, NXCException
   {
      List<Zone> zones = session.getAllZones();
      Map<Integer, List<String>> ports = new HashMap<Integer, List<String>>();
      for (Zone z : zones)
      {
         if (!z.getSnmpPorts().isEmpty())
            ports.put((int)z.getUIN(), z.getSnmpPorts());
      }
      Map<String, ServerVariable> variables = session.getServerVariables();
      ServerVariable v = variables.get("SNMPPorts"); //$NON-NLS-1$
      parsePorts(v != null ? v.getValue() : "", ports); //$NON-NLS-1$
      
      return ports;
   }
   
   /**
    * 
    * @param portList
    */
   private static void parsePorts(String portList, Map<Integer, List<String>> ports)
   {
      List<String> list = new ArrayList<String>(Arrays.asList(portList.split(",")));
      ports.put(SNMP_CONFIG_GLOBAL, list);      
   }
   
   public String parsePorts() 
   {
      StringBuilder str = new StringBuilder();
      List<String> list = ports.get(SNMP_CONFIG_GLOBAL);
      if (list == null)
         return "";
      
      for(int i = 0; i < list.size(); i++)
      {
         str.append(list.get(i));
         if(i != list.size() - 1)
         {
            str.append(",");             //$NON-NLS-1$
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
      savePortConfig(session);
   }
   
   private void savePortConfig(final NXCSession session) throws IOException, NXCException
   {
      session.setServerVariable("SNMPPorts", parsePorts()); //$NON-NLS-1$
      for(Integer i : ports.keySet())
      {
         if (ports.get(i).isEmpty() || session.findZone(i) == null)
            continue;
         final NXCObjectModificationData data = new NXCObjectModificationData(session.findZone(i).getObjectId());
         data.setSnmpPorts(ports.get(i));
         session.modifyObject(data);
      }
   }
   
   /**
    * @return the communities
    */
   public List<String> getCommunities(long zoneUIN)
   {
      if (communities.containsKey((int)zoneUIN))
         return communities.get((int)zoneUIN);
      else
         return new ArrayList<String>();
   }

   /**
    * @param communities the communities to set
    */
   public void addCommunityString(String communityString, long zoneUIN)
   {
      if (this.communities.containsKey((int)zoneUIN))
         this.communities.get((int)zoneUIN).add(communityString);
      else
      {
         List<String> list = new ArrayList<String>();
         list.add(communityString);
         this.communities.put((int)zoneUIN, list);
      }
   }
   
   /**
    * @return the ports
    */
   public List<String> getPorts(long zoneUIN)
   {
      if (ports.containsKey((int)zoneUIN))
         return ports.get((int)zoneUIN);
      else
         return new ArrayList<String>();
   }
   
   /**
    * @param communities the communities to set
    */
   public void addPort(String port, long zoneUIN)
   {
      if (this.ports.containsKey((int)zoneUIN))
         this.ports.get((int)zoneUIN).add(port);
      else
      {
         List<String> list = new ArrayList<String>();
         list.add(port);
         this.ports.put((int)zoneUIN, list);
      }
   }

   /**
    * @return the usmCredentials
    */
   public List<SnmpUsmCredential> getUsmCredentials(long zoneUIN)
   {
      if (usmCredentials.containsKey((int)zoneUIN))
         return usmCredentials.get((int)zoneUIN);
      else
         return new ArrayList<SnmpUsmCredential>();
   }

   /**
    * @param usmCredentials the usmCredentials to set
    */
   public void addUsmCredentials(SnmpUsmCredential credential, long zoneUIN)
   {
      if (usmCredentials.containsKey((int)zoneUIN))
         usmCredentials.get((int)zoneUIN).add(credential);
      else
      {
         List<SnmpUsmCredential> list = new ArrayList<SnmpUsmCredential>();
         list.add(credential);
         usmCredentials.put((int)zoneUIN, list);
      }
   }   
}
