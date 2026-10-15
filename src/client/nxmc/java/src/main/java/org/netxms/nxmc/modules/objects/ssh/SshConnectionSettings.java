/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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
package org.netxms.nxmc.modules.objects.ssh;

import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractNode;

/**
 * Settings for SSH terminal session selected in connect dialog.
 */
public class SshConnectionSettings
{
   /**
    * Session transport mode
    */
   public enum Mode
   {
      /** SSH session is established by node's SSH proxy agent and relayed by server */
      PROXY,
      /** SSH client runs in the console and connects over TCP proxy channel */
      DIRECT
   }

   private Mode mode = Mode.PROXY;
   private String login = null; // null means "use credentials stored in node properties" (proxy mode only)
   private String password = "";
   private int keyId = 0; // server-side key pair ID (proxy mode)
   private String keyFile = ""; // local private key file (direct mode)
   private int port = 22;

   /**
    * Create settings with defaults derived from node properties.
    *
    * @param node target node
    */
   public SshConnectionSettings(AbstractNode node)
   {
      port = (node.getSshPort() > 0) ? node.getSshPort() : 22;
      mode = ((node.getCapabilities() & AbstractNode.NC_IS_SSH) != 0) ? Mode.PROXY : Mode.DIRECT;
   }

   /**
    * Create transport for these settings.
    *
    * @param session client session
    * @param node target node
    * @param hostKeyVerifier host key verifier for direct mode
    * @return transport object (not connected yet)
    */
   public TerminalTransport createTransport(NXCSession session, AbstractNode node, HostKeyVerifier hostKeyVerifier)
   {
      if (mode == Mode.DIRECT)
         return new DirectSshTransport(session, node, port, login, password, keyFile, hostKeyVerifier);
      return new ProxySshTransport(session, node.getObjectId(), login, password, keyId);
   }

   /**
    * @return the mode
    */
   public Mode getMode()
   {
      return mode;
   }

   /**
    * @param mode the mode to set
    */
   public void setMode(Mode mode)
   {
      this.mode = mode;
   }

   /**
    * @return login name or null if stored node credentials should be used
    */
   public String getLogin()
   {
      return login;
   }

   /**
    * @param login login name or null to use stored node credentials
    */
   public void setLogin(String login)
   {
      this.login = login;
   }

   /**
    * @return the password
    */
   public String getPassword()
   {
      return password;
   }

   /**
    * @param password the password to set
    */
   public void setPassword(String password)
   {
      this.password = password;
   }

   /**
    * @return the keyId
    */
   public int getKeyId()
   {
      return keyId;
   }

   /**
    * @param keyId the keyId to set
    */
   public void setKeyId(int keyId)
   {
      this.keyId = keyId;
   }

   /**
    * @return the keyFile
    */
   public String getKeyFile()
   {
      return keyFile;
   }

   /**
    * @param keyFile the keyFile to set
    */
   public void setKeyFile(String keyFile)
   {
      this.keyFile = keyFile;
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
}
