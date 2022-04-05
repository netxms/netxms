package org.netxms.ui.eclipse.snmp.views.helpers;

import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.SshCredential;
import org.netxms.client.snmp.SnmpUsmCredential;

public class NetworkConfig
{
   // Global SNMP config flag
   public static int NETWORK_CONFIG_GLOBAL = -1;
   public static int ALL_ZONES = -2;

   // Configuration type flags
   public static final int COMMUNITIES     = 0x01;
   public static final int USM             = 0x02;
   public static final int SNMP_PORTS      = 0x04;
   public static final int AGENT_SECRETS   = 0x08;
   public static final int AGENT_PORTS     = 0x10;
   public static final int SSH_CREDENTIALS = 0x20;
   public static final int SSH_PORTS       = 0x40;
   public static final int ALL_CONFIGS     = 0x7F; 

   private Map<Integer, List<String>> communities = new HashMap<Integer, List<String>>();
   private Map<Integer, List<SnmpUsmCredential>> usmCredentials = new HashMap<Integer, List<SnmpUsmCredential>>();
   private Map<Integer, List<String>> sharedSecrets = new HashMap<Integer, List<String>>();
   private Map<Integer, List<SshCredential>> sshCredentials = new HashMap<Integer, List<SshCredential>>();

   private Map<Integer, List<Integer>> snmpPorts = new HashMap<Integer, List<Integer>>();
   private Map<Integer, List<Integer>> agentPorts = new HashMap<Integer, List<Integer>>();
   private Map<Integer, List<Integer>> sshPorts = new HashMap<Integer, List<Integer>>();

   private NXCSession session;
   private Map<Integer, Integer> changedConfig = new HashMap<Integer, Integer>();

   /**
    * Create object
    */
   public NetworkConfig(NXCSession session)
   {
      this.session = session;
   }

   private void setPorts(int type, Map<Integer, List<Integer>> ports)
   {
      switch(type)
      {
         case SNMP_PORTS:
            snmpPorts = ports;
            break;
         case AGENT_PORTS:
            agentPorts = ports;
            break;
         case SSH_PORTS:
            sshPorts = ports;
            break;
      }
   }
   
   private Map<Integer, List<Integer>> getPorts(int type)
   {
      switch(type)
      {
         case SNMP_PORTS:
            return snmpPorts;
         case AGENT_PORTS:
            return agentPorts;
         case SSH_PORTS:
            return sshPorts;
      }
      return null;
   }

   /**
    * Load exact configuration part 
    * 
    * @param configId id of configuration objects
    */
   public void load(int configId, int zoneUIN) throws NXCException, IOException
   {
      if ((configId & COMMUNITIES) > 0)
      {
         if (ALL_ZONES == zoneUIN)
         {
            communities = session.getSnmpCommunities();
         }
         else
         {
            communities.put(zoneUIN, session.getSnmpCommunities(zoneUIN));              
         }    
      }

      if ((configId & USM) > 0)
      {
         if (ALL_ZONES == zoneUIN)
         {
            usmCredentials = session.getSnmpUsmCredentials();
         }
         else
         {
            usmCredentials.put(zoneUIN, session.getSnmpUsmCredentials(zoneUIN));              
         } 
      }

      if ((configId & SNMP_PORTS) > 0)
      {
         if (ALL_ZONES == zoneUIN)
         {
            setPorts(SNMP_PORTS, session.getWellKnownPorts("snmp"));
         }
         else
         {
            getPorts(SNMP_PORTS).put(zoneUIN, session.getWellKnownPorts(zoneUIN, "snmp"));
         }
      }

      if ((configId & AGENT_SECRETS) > 0)
      {
         if (ALL_ZONES == zoneUIN)
         {
            sharedSecrets = session.getAgentSharedSecrets();
         }
         else
         {
            sharedSecrets.put(zoneUIN, session.getAgentSharedSecrets(zoneUIN));            
         }  
      }

      if ((configId & AGENT_PORTS) > 0)
      {
         if (ALL_ZONES == zoneUIN)
         {
            setPorts(AGENT_PORTS, session.getWellKnownPorts("agent"));
         }
         else
         {
            getPorts(AGENT_PORTS).put(zoneUIN, session.getWellKnownPorts(zoneUIN, "agent"));
         }
      }

      if ((configId & SSH_CREDENTIALS) > 0)
      {
         if (ALL_ZONES == zoneUIN)
         {
            sshCredentials = session.getSshCredentials();
         }
         else
         {
            sshCredentials.put(zoneUIN, session.getSshCredentials(zoneUIN));
         }
      }

      if ((configId & SSH_PORTS) > 0)
      {
         if (ALL_ZONES == zoneUIN)
         {
            setPorts(SSH_PORTS, session.getWellKnownPorts("ssh"));
         }
         else
         {
            getPorts(SSH_PORTS).put(zoneUIN, session.getWellKnownPorts(zoneUIN, "ssh"));
         }
      }
   }

   public boolean isChanged(int configId, int zoneUIN)
   {
      return (changedConfig.getOrDefault(zoneUIN, 0) & configId) > 0;
   }

   /**
    * Save SNMP configuration on server. This method calls communication API directly, so it should not be called from UI thread.
    * 
    * @param session communication session to use
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void save(NXCSession session) throws NXCException, IOException
   {
      for (Entry<Integer, Integer> value : changedConfig.entrySet())
         if ((value.getValue() & COMMUNITIES) > 0)
         {
            session.updateSnmpCommunities(value.getKey(), communities.get(value.getKey()));
         }
      
      for (Entry<Integer, Integer> value : changedConfig.entrySet())
         if ((value.getValue() & USM) > 0)
         {
            session.updateSnmpUsmCredentials(value.getKey(), usmCredentials.get(value.getKey()));
         }
      
      for (Entry<Integer, Integer> value : changedConfig.entrySet())
         if ((value.getValue() & SNMP_PORTS) > 0)
         {
            session.updateWellKnownPorts(value.getKey(), "snmp", getPorts(SNMP_PORTS).get(value.getKey()));
         }
      
      for (Entry<Integer, Integer> value : changedConfig.entrySet())
         if ((value.getValue() & AGENT_SECRETS) > 0)
         {
            session.updateAgentSharedSecrets(value.getKey(), sharedSecrets.get(value.getKey()));
         }
      
      for(Entry<Integer, Integer> value : changedConfig.entrySet())
         if ((value.getValue() & AGENT_PORTS) > 0)
         {
            session.updateWellKnownPorts(value.getKey(), "agent", getPorts(AGENT_PORTS).get(value.getKey()));
         }

      for(Entry<Integer, Integer> value : changedConfig.entrySet())
         if ((value.getValue() & SSH_CREDENTIALS) > 0)
         {
            session.updateSshCredentials(value.getKey(), sshCredentials.get(value.getKey()));
         }

      for(Entry<Integer, Integer> value : changedConfig.entrySet())
         if ((value.getValue() & SSH_PORTS) > 0)
         {
            session.updateWellKnownPorts(value.getKey(), "ssh", getPorts(SSH_PORTS).get(value.getKey()));
         }

      changedConfig.clear();
   }
   
   /**
    * Provide id of updated configuration
    * 
    * @param id
    */
   public void setConfigUpdate(long zoneUIN, int id)
   {
      changedConfig.put((int)zoneUIN, changedConfig.getOrDefault((int)zoneUIN, 0) | id);
   }
   
   /**
    * @param zoneUIN the zone which community strings to get
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
    * @param communityString the community string to set
    * @param zoneUIN the zone of the community string
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
    * @param zoneUIN the zone which ports to get
    * @param type port type
    * @return the ports
    */
   public List<Integer> getPorts(int type, long zoneUIN)
   {
      if (getPorts(type).containsKey((int)zoneUIN))
         return getPorts(type).get((int)zoneUIN);
      else
         return new ArrayList<Integer>();
   }

   /**
    * @param port the port to set
    * @param type port type
    * @param zoneUIN the zone of the given port
    */
   public void addPort(int type, Integer port, long zoneUIN)
   {
      if (getPorts(type).containsKey((int)zoneUIN))
      {
         getPorts(type).get((int)zoneUIN).add(port);
      }
      else
      {
         List<Integer> list = new ArrayList<Integer>();
         list.add(port);
         getPorts(type).put((int)zoneUIN, list);
      }
   }

   /**
    * @param zoneUIN
    * @return the communities
    */
   public List<SshCredential> getSshCredentials(long zoneUIN)
   {
      if (sshCredentials.containsKey((int)zoneUIN))
         return sshCredentials.get((int)zoneUIN);
      else
         return new ArrayList<SshCredential>();
   }

   /**
    * @param credential the usmCredentials to set
    * @param zoneUIN
    */
   public void addSshCredentials(SshCredential credential, long zoneUIN)
   {
      if (sshCredentials.containsKey((int)zoneUIN))
      {
         sshCredentials.get((int)zoneUIN).add(credential);
      }
      else
      {
         List<SshCredential> list = new ArrayList<SshCredential>();
         list.add(credential);
         sshCredentials.put((int)zoneUIN, list);
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
    * @param credential the SnmpUsmCredential to set
    * @param zoneUIN the zone of the given credential
    */
   public void addUsmCredentials(SnmpUsmCredential credential, long zoneUIN)
   {
      if (usmCredentials.containsKey((int)zoneUIN))
      {
         usmCredentials.get((int)zoneUIN).add(credential);
      }
      else
      {
         List<SnmpUsmCredential> list = new ArrayList<SnmpUsmCredential>();
         list.add(credential);
         usmCredentials.put((int)zoneUIN, list);
      }
   } 
   
   /**
    * @param zoneUIN the zone which credentials to get
    * @return the shared secrets
    */
   public List<String> getSharedSecrets(long zoneUIN)
   {
      if (sharedSecrets.containsKey((int)zoneUIN))
         return sharedSecrets.get((int)zoneUIN);
      else
         return new ArrayList<String>();
   }

   /**
    * @param sharedSecret the shared secret to set
    * @param zoneUIN zone UIN
    */
   public void addSharedSecret(String sharedSecret, long zoneUIN)
   {
      if (this.sharedSecrets.containsKey((int)zoneUIN))
      {
         this.sharedSecrets.get((int)zoneUIN).add(sharedSecret);
      }
      else
      {
         List<String> list = new ArrayList<String>();
         list.add(sharedSecret);
         this.sharedSecrets.put((int)zoneUIN, list);
      }
   }
}
