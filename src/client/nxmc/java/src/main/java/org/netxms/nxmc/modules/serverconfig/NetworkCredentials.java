/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.nxmc.modules.serverconfig;

import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.SSHCredentials;
import org.netxms.client.snmp.SnmpUsmCredential;

/**
 * Class that contains all parts of network connectivity configuration used mainly for network discovery (port numbers, logins, etc.) 
 */
public class NetworkCredentials
{
   // Global SNMP config flag
   public static int NETWORK_CONFIG_GLOBAL = -1;
   public static int ALL_ZONES = -2;

   // Configuration type flags
   public static final int SNMP_COMMUNITIES     = 0x01;
   public static final int SNMP_USM_CREDENTIALS = 0x02;
   public static final int SNMP_PORTS           = 0x04;
   public static final int AGENT_SECRETS        = 0x08;
   public static final int AGENT_PORTS          = 0x10;
   public static final int SSH_CREDENTIALS      = 0x20;
   public static final int SSH_PORTS            = 0x40;
   public static final int EVERYTHING           = 0x7F; 

   private Map<Integer, List<String>> communities = new HashMap<Integer, List<String>>();
   private Map<Integer, List<SnmpUsmCredential>> usmCredentials = new HashMap<Integer, List<SnmpUsmCredential>>();
   private Map<Integer, List<String>> sharedSecrets = new HashMap<Integer, List<String>>();
   private Map<Integer, List<SSHCredentials>> sshCredentials = new HashMap<Integer, List<SSHCredentials>>();

   private Map<Integer, List<Integer>> snmpPorts = new HashMap<Integer, List<Integer>>();
   private Map<Integer, List<Integer>> agentPorts = new HashMap<Integer, List<Integer>>();
   private Map<Integer, List<Integer>> sshPorts = new HashMap<Integer, List<Integer>>();

   private NXCSession session;
   private Map<Integer, Integer> changedConfig = new HashMap<Integer, Integer>();

   /**
    * Create object
    */
   public NetworkCredentials(NXCSession session)
   {
      this.session = session;
   }

   /**
    * Load exact configuration part 
    * 
    * @param configId id of configuration objects
    */
   public void load(int configId, int zoneUIN) throws NXCException, IOException
   {
      if ((configId & SNMP_COMMUNITIES) != 0)
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

      if ((configId & SNMP_USM_CREDENTIALS) != 0)
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

      if ((configId & SNMP_PORTS) != 0)
      {
         if (ALL_ZONES == zoneUIN)
         {
            snmpPorts = session.getWellKnownPorts("snmp");
         }
         else
         {
            snmpPorts.put(zoneUIN, session.getWellKnownPorts(zoneUIN, "snmp"));
         }
      }

      if ((configId & AGENT_SECRETS) != 0)
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

      if ((configId & AGENT_PORTS) != 0)
      {
         if (ALL_ZONES == zoneUIN)
         {
            agentPorts = session.getWellKnownPorts("agent");
         }
         else
         {
            agentPorts.put(zoneUIN, session.getWellKnownPorts(zoneUIN, "agent"));
         }
      }

      if ((configId & SSH_CREDENTIALS) != 0)
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

      if ((configId & SSH_PORTS) != 0)
      {
         if (ALL_ZONES == zoneUIN)
         {
            sshPorts = session.getWellKnownPorts("ssh");
         }
         else
         {
            sshPorts.put(zoneUIN, session.getWellKnownPorts(zoneUIN, "ssh"));
         }
      }
   }

   /**
    * Check if specific configuration part is changed for given zone.
    *
    * @param type configuration type
    * @param zoneUIN zone UIN
    * @return true if specific configuration part is changed for given zone
    */
   public boolean isChanged(int type, int zoneUIN)
   {
      return (changedConfig.getOrDefault(zoneUIN, 0) & type) > 0;
   }

   /**
    * Save SNMP configuration on server. This method calls communication API directly, so it should not be called from UI thread.
    * 
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void save() throws NXCException, IOException
   {
      for (Entry<Integer, Integer> value : changedConfig.entrySet())
         if ((value.getValue() & SNMP_COMMUNITIES) > 0)
         {
            session.updateSnmpCommunities(value.getKey(), communities.get(value.getKey()));
         }

      for (Entry<Integer, Integer> value : changedConfig.entrySet())
         if ((value.getValue() & SNMP_USM_CREDENTIALS) > 0)
         {
            session.updateSnmpUsmCredentials(value.getKey(), usmCredentials.get(value.getKey()));
         }
      
      for (Entry<Integer, Integer> value : changedConfig.entrySet())
         if ((value.getValue() & SNMP_PORTS) > 0)
         {
            session.updateWellKnownPorts(value.getKey(), "snmp", snmpPorts.get(value.getKey()));
         }
      
      for (Entry<Integer, Integer> value : changedConfig.entrySet())
         if ((value.getValue() & AGENT_SECRETS) > 0)
         {
            session.updateAgentSharedSecrets(value.getKey(), sharedSecrets.get(value.getKey()));
         }
      
      for(Entry<Integer, Integer> value : changedConfig.entrySet())
         if ((value.getValue() & AGENT_PORTS) > 0)
         {
            session.updateWellKnownPorts(value.getKey(), "agent", agentPorts.get(value.getKey()));
         }

      for(Entry<Integer, Integer> value : changedConfig.entrySet())
         if ((value.getValue() & SSH_CREDENTIALS) > 0)
         {
            session.updateSshCredentials(value.getKey(), sshCredentials.get(value.getKey()));
         }

      for(Entry<Integer, Integer> value : changedConfig.entrySet())
         if ((value.getValue() & SSH_PORTS) > 0)
         {
            session.updateWellKnownPorts(value.getKey(), "ssh", sshPorts.get(value.getKey()));
         }

      changedConfig.clear();
   }

   /**
    * Provide id of updated configuration
    * 
    * @param id
    */
   public void setConfigUpdate(int zoneUIN, int id)
   {
      changedConfig.put(zoneUIN, changedConfig.getOrDefault((int)zoneUIN, 0) | id);
   }

   /**
    * @param zoneUIN the zone which community strings to get
    * @return the communities
    */
   public List<String> getCommunities(int zoneUIN)
   {
      List<String> list = communities.get(zoneUIN);
      return (list != null) ? list : new ArrayList<String>(0);
   }

   /**
    * @param communityString the community string to set
    * @param zoneUIN the zone of the community string
    */
   public void addCommunityString(String communityString, int zoneUIN)
   {
      List<String> list = communities.get(zoneUIN);
      if (list == null)
      {
         list = new ArrayList<String>();
         communities.put(zoneUIN, list);
      }
      list.add(communityString);
   }

   /**
    * @param type port type
    * @param zoneUIN the zone which ports to get
    * @return the ports
    */
   public List<Integer> getPorts(int type, int zoneUIN)
   {
      Map<Integer, List<Integer>> portMap = portMapFromType(type);
      List<Integer> ports = portMap.get(zoneUIN);
      return (ports != null) ? ports : new ArrayList<Integer>(0);
   }

   /**
    * @param port the port to set
    * @param type port type
    * @param zoneUIN the zone of the given port
    */
   public void addPort(int type, Integer port, int zoneUIN)
   {
      Map<Integer, List<Integer>> portMap = portMapFromType(type);
      List<Integer> ports = portMap.get(zoneUIN);
      if (ports == null)
      {
         ports = new ArrayList<Integer>();
         portMap.put(zoneUIN, ports);
      }
      ports.add(port);
   }

   /**
    * Get port map by port type.
    *
    * @param type port type
    * @return port map for given type
    */
   private Map<Integer, List<Integer>> portMapFromType(int type)
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
      throw new IllegalArgumentException("Unknown port type");
   }

   /**
    * @param zoneUIN
    * @return the communities
    */
   public List<SSHCredentials> getSshCredentials(int zoneUIN)
   {
      List<SSHCredentials> credentials = sshCredentials.get(zoneUIN);
      return (credentials != null) ? credentials : new ArrayList<SSHCredentials>(0);
   }

   /**
    * @param credential the usmCredentials to set
    * @param zoneUIN
    */
   public void addSshCredentials(SSHCredentials credential, int zoneUIN)
   {
      if (sshCredentials.containsKey(zoneUIN))
      {
         sshCredentials.get(zoneUIN).add(credential);
      }
      else
      {
         List<SSHCredentials> list = new ArrayList<SSHCredentials>();
         list.add(credential);
         sshCredentials.put(zoneUIN, list);
      }
   }

   /**
    * @return the usmCredentials
    */
   public List<SnmpUsmCredential> getUsmCredentials(int zoneUIN)
   {
      if (usmCredentials.containsKey(zoneUIN))
         return usmCredentials.get(zoneUIN);
      else
         return new ArrayList<SnmpUsmCredential>(0);
   }

   /**
    * @param credential the SnmpUsmCredential to set
    * @param zoneUIN the zone of the given credential
    */
   public void addUsmCredentials(SnmpUsmCredential credential, int zoneUIN)
   {
      if (usmCredentials.containsKey(zoneUIN))
      {
         usmCredentials.get(zoneUIN).add(credential);
      }
      else
      {
         List<SnmpUsmCredential> list = new ArrayList<SnmpUsmCredential>();
         list.add(credential);
         usmCredentials.put(zoneUIN, list);
      }
   } 

   /**
    * @param zoneUIN the zone which credentials to get
    * @return the shared secrets
    */
   public List<String> getSharedSecrets(int zoneUIN)
   {
      if (sharedSecrets.containsKey(zoneUIN))
         return sharedSecrets.get(zoneUIN);
      else
         return new ArrayList<String>();
   }

   /**
    * @param sharedSecret the shared secret to set
    * @param zoneUIN zone UIN
    */
   public void addSharedSecret(String sharedSecret, int zoneUIN)
   {
      if (this.sharedSecrets.containsKey(zoneUIN))
      {
         this.sharedSecrets.get(zoneUIN).add(sharedSecret);
      }
      else
      {
         List<String> list = new ArrayList<String>();
         list.add(sharedSecret);
         this.sharedSecrets.put(zoneUIN, list);
      }
   }
}
