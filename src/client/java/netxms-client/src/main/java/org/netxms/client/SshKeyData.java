/**
 * NetXMS - open source network management system
 * Copyright (C) 2021 Raden Solutions
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
package org.netxms.client;

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * SSH key data
 */
public class SshKeyData
{
   private int id;
   private String name;
   private String publicKey;
   private String privateKey;
   
   /**
    * Default constructor
    */
   public SshKeyData()
   {
      id = 0;
      name = "";
      publicKey = null;
      privateKey = null;
   }
   
   /**
    * Copy constructor
    */
   public SshKeyData(SshKeyData src)
   {
      id = src.id;
      name = src.name;
      publicKey = src.publicKey;
      privateKey = src.privateKey;
   }
   
   /**
    * Create object form NXCP message
    * 
    * @param msg message
    * @param baseID object field base id
    */
   public SshKeyData(NXCPMessage msg, long baseID)
   {
      id = msg.getFieldAsInt32(baseID++);
      name = msg.getFieldAsString(baseID++);
      publicKey = msg.getFieldAsString(baseID++);
      privateKey = null;       
   }
   
   /**
    * Fill NXCP message
    * 
    * @param msg message
    * @param baseID
    */
   public void fillMessage(NXCPMessage msg)
   {
      msg.setFieldInt32(NXCPCodes.VID_SSH_KEY_ID, id);
      msg.setField(NXCPCodes.VID_NAME, name);
      if (privateKey != null)
      {
         msg.setField(NXCPCodes.VID_PUBLIC_KEY, publicKey);
         msg.setField(NXCPCodes.VID_PRIVATE_KEY, privateKey);
      }
   }
   
   /**
    * @return the id
    */
   public int getId()
   {
      return id;
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
    * @return the publicCrtificate
    */
   public String getPublicKey()
   {
      return publicKey != null ? publicKey : "";
   }
   
   /**
    * @param publicKey the publicKey to set
    */
   public void setPublicKey(String publicKey)
   {
      this.publicKey = publicKey;
   }

   /**
    * @param privateKey the privateKey to set
    */
   public void setPrivateKey(String privateKey)
   {
      this.privateKey = privateKey;
   }

   /**
    * @return the privateCertificate
    */
   public String getPrivateKey()
   {
      return privateKey != null ? privateKey : "";
   }  
}
