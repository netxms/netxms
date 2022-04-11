/**
 * NetXMS - open source network management system
 * Copyright (C) 2022 Raden Solutions
 * <p>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * <p>
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * <p>
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

package org.netxms.client;

import org.netxms.base.NXCPMessage;

/**
 * SSH credentials object
 */
public class SSHCredentials
{
   private String login;
   private String password;
   private int keyId;

   /**
    * Create new SSH credentials object.
    * 
    * @param login login name
    * @param password password
    * @param keyId SSH key ID (0 if not assigned)
    */
   public SSHCredentials(String login, String password, int keyId)
   {
      this.login = login;
      this.password = password;
      this.keyId = keyId;
   }

   /**
    * Create new SSH credentials object from NXCP message
    * 
    * @param msg NXCP message
    * @param baseId base field ID
    */
   public SSHCredentials(NXCPMessage msg, long baseId)
   {
      login = msg.getFieldAsString(baseId);
      password = msg.getFieldAsString(baseId + 1);
      keyId = msg.getFieldAsInt32(baseId + 2);
   }

   /**
    * Fill NXCP message with object data
    * 
    * @param msg NXCP message
    * @param baseId base field ID
    */
   public void fillMessage(NXCPMessage msg, long baseId)
   {
      msg.setField(baseId, login);
      msg.setField(baseId + 1, password);
      msg.setFieldInt32(baseId + 2, keyId);
   }

   /**
    * @return the login
    */
   public String getLogin()
   {
      return login;
   }

   /**
    * @return the password
    */
   public String getPassword()
   {
      return password;
   }

   /**
    * @return the keyId
    */
   public int getKeyId()
   {
      return keyId;
   }

   /**
    * @param login the login to set
    */
   public void setLogin(String login)
   {
      this.login = login;
   }

   /**
    * @param password the password to set
    */
   public void setPassword(String password)
   {
      this.password = password;
   }

   /**
    * @param keyId the keyId to set
    */
   public void setKeyId(int keyId)
   {
      this.keyId = keyId;
   }

   /**
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "SSHCredentials [login=" + login + ", password=" + password + ", keyId=" + keyId + "]";
   }
}
