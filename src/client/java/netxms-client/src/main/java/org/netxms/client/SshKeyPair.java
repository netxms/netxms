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
 * SSH key pair
 */
public class SshKeyPair
{
   private int id;
   private String name;
   private String publicKey;
   private String privateKey;

   /**
    * Default constructor
    */
   public SshKeyPair()
   {
      id = 0;
      name = "";
      publicKey = null;
      privateKey = null;
   }

   /**
    * Copy constructor
    * 
    * @param src source object
    */
   public SshKeyPair(SshKeyPair src)
   {
      id = src.id;
      name = src.name;
      publicKey = src.publicKey;
      privateKey = src.privateKey;
   }

   /**
    * Create from NXCP message
    * 
    * @param msg NXCP message
    * @param baseId base field ID
    */
   public SshKeyPair(NXCPMessage msg, long baseId)
   {
      id = msg.getFieldAsInt32(baseId++);
      name = msg.getFieldAsString(baseId++);
      publicKey = msg.getFieldAsString(baseId++);
      privateKey = null;       
   }

   /**
    * Fill NXCP message
    * 
    * @param msg NXCP message
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
    * Get key ID.
    *
    * @return key ID
    */
   public int getId()
   {
      return id;
   }

   /**
    * Get key name.
    *
    * @return key name
    */
   public String getName()
   {
      return name;
   }

   /**
    * Set key name.
    *
    * @param name new key name
    */
   public void setName(String name)
   {
      this.name = name;
   }

   /**
    * Get public part of this key pair.
    *
    * @return public key in SSH encoding or empty string if not set
    */
   public String getPublicKey()
   {
      return publicKey != null ? publicKey : "";
   }

   /**
    * Get private part of this key pair.
    *
    * @return private key in SSH encoding or empty string if not set
    */
   public String getPrivateKey()
   {
      return privateKey != null ? privateKey : "";
   }

   /**
    * Set public and private keys.
    * 
    * @param publicKey new public key
    * @param privateKey new private key
    */
   public void setKeys(String publicKey, String privateKey)
   {
      this.publicKey = publicKey;
      this.privateKey = privateKey;
   }
}
