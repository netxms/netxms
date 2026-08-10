/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Victor Kirhenshtein
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
package org.netxms.client.snmp;

import java.net.InetAddress;
import org.netxms.base.NXCPMessage;

/**
 * Configuration of additional SNMP agent on a node - alternative SNMP endpoint with its own port,
 * credentials, and optionally IP address, referenced by name from data collection items.
 */
public class SnmpAgentConfiguration
{
   private String name;
   private InetAddress address;
   private int port;
   private SnmpVersion version;
   private String authName;
   private String authPassword;
   private String privPassword;
   private int authMethod;
   private int privMethod;
   private String contextName;

   /**
    * Default constructor.
    */
   public SnmpAgentConfiguration()
   {
      name = "";
      address = null;
      port = 161;
      version = SnmpVersion.V2C;
      authName = "";
      authPassword = "";
      privPassword = "";
      authMethod = 0;
      privMethod = 0;
      contextName = "";
   }

   /**
    * Copy constructor.
    *
    * @param src source object
    */
   public SnmpAgentConfiguration(SnmpAgentConfiguration src)
   {
      name = src.name;
      address = src.address;
      port = src.port;
      version = src.version;
      authName = src.authName;
      authPassword = src.authPassword;
      privPassword = src.privPassword;
      authMethod = src.authMethod;
      privMethod = src.privMethod;
      contextName = src.contextName;
   }

   /**
    * Create agent configuration from data in NXCP message.
    *
    * @param msg NXCP message
    * @param baseId base field ID
    */
   public SnmpAgentConfiguration(NXCPMessage msg, long baseId)
   {
      name = msg.getFieldAsString(baseId);
      address = msg.getFieldAsInetAddress(baseId + 1);
      port = msg.getFieldAsInt32(baseId + 2);
      version = SnmpVersion.getByValue(msg.getFieldAsInt32(baseId + 3));
      authName = msg.getFieldAsString(baseId + 4);
      authPassword = msg.getFieldAsString(baseId + 5);
      privPassword = msg.getFieldAsString(baseId + 6);
      int methods = msg.getFieldAsInt32(baseId + 7);
      authMethod = methods & 0xFF;
      privMethod = methods >> 8;
      contextName = msg.getFieldAsString(baseId + 8);
   }

   /**
    * Fill NXCP message with agent configuration data.
    *
    * @param msg NXCP message
    * @param baseId base field ID
    */
   public void fillMessage(NXCPMessage msg, long baseId)
   {
      msg.setField(baseId, name);
      if (address != null)
         msg.setField(baseId + 1, address);
      msg.setFieldInt16(baseId + 2, port);
      msg.setFieldInt16(baseId + 3, version.getValue());
      msg.setField(baseId + 4, authName);
      msg.setField(baseId + 5, authPassword);
      msg.setField(baseId + 6, privPassword);
      msg.setFieldInt16(baseId + 7, authMethod | (privMethod << 8));
      msg.setField(baseId + 8, contextName);
   }

   /**
    * @return the name
    */
   public String getName()
   {
      return name;
   }

   /**
    * @param name the name to set
    */
   public void setName(String name)
   {
      this.name = name;
   }

   /**
    * Get IP address of the agent. Null means node's primary IP address.
    *
    * @return IP address of the agent or null
    */
   public InetAddress getAddress()
   {
      return address;
   }

   /**
    * Set IP address of the agent. Null means node's primary IP address.
    *
    * @param address IP address of the agent or null
    */
   public void setAddress(InetAddress address)
   {
      this.address = address;
   }

   /**
    * @return the port
    */
   public int getPort()
   {
      return port;
   }

   /**
    * @param port the port to set
    */
   public void setPort(int port)
   {
      this.port = port;
   }

   /**
    * @return the version
    */
   public SnmpVersion getVersion()
   {
      return version;
   }

   /**
    * @param version the version to set
    */
   public void setVersion(SnmpVersion version)
   {
      this.version = version;
   }

   /**
    * Get authentication object (community string for SNMP v1/v2c or user name for SNMP v3).
    *
    * @return the authName
    */
   public String getAuthName()
   {
      return authName;
   }

   /**
    * @param authName the authName to set
    */
   public void setAuthName(String authName)
   {
      this.authName = authName;
   }

   /**
    * @return the authPassword
    */
   public String getAuthPassword()
   {
      return authPassword;
   }

   /**
    * @param authPassword the authPassword to set
    */
   public void setAuthPassword(String authPassword)
   {
      this.authPassword = authPassword;
   }

   /**
    * @return the privPassword
    */
   public String getPrivPassword()
   {
      return privPassword;
   }

   /**
    * @param privPassword the privPassword to set
    */
   public void setPrivPassword(String privPassword)
   {
      this.privPassword = privPassword;
   }

   /**
    * @return the authMethod
    */
   public int getAuthMethod()
   {
      return authMethod;
   }

   /**
    * @param authMethod the authMethod to set
    */
   public void setAuthMethod(int authMethod)
   {
      this.authMethod = authMethod;
   }

   /**
    * @return the privMethod
    */
   public int getPrivMethod()
   {
      return privMethod;
   }

   /**
    * @param privMethod the privMethod to set
    */
   public void setPrivMethod(int privMethod)
   {
      this.privMethod = privMethod;
   }

   /**
    * @return the contextName
    */
   public String getContextName()
   {
      return contextName;
   }

   /**
    * @param contextName the contextName to set
    */
   public void setContextName(String contextName)
   {
      this.contextName = contextName;
   }
}
