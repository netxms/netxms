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

import java.io.InputStream;
import java.io.OutputStream;
import java.nio.channels.ClosedChannelException;
import org.netxms.client.NXCSession;
import org.netxms.client.TcpProxy;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Terminal transport over interactive SSH channel established by node's SSH proxy agent. Terminal type is fixed to
 * "xterm" because both console terminal implementations emulate xterm. Transport is single-use: close() called while
 * connect() is still in progress is remembered, and connect() then releases the channel and throws
 * ClosedChannelException instead of handing it over.
 */
public class ProxySshTransport implements TerminalTransport
{
   private static final Logger logger = LoggerFactory.getLogger(ProxySshTransport.class);

   private final NXCSession session;
   private final long nodeId;
   private final String login;
   private final String password;
   private final int keyId;
   private TcpProxy proxy = null; // guarded by this
   private boolean closed = false; // guarded by this

   /**
    * Create transport.
    *
    * @param session client session
    * @param nodeId target node ID
    * @param login login name or null to use credentials stored in node properties
    * @param password password (ignored if login is null)
    * @param keyId server-side SSH key pair ID or 0 (ignored if login is null)
    */
   public ProxySshTransport(NXCSession session, long nodeId, String login, String password, int keyId)
   {
      this.session = session;
      this.nodeId = nodeId;
      this.login = login;
      this.password = password;
      this.keyId = keyId;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.ssh.TerminalTransport#connect(int, int)
    */
   @Override
   public void connect(int columns, int rows) throws Exception
   {
      TcpProxy p = session.setupSshTerminal(nodeId, login, password, keyId, "xterm", columns, rows);
      synchronized(this)
      {
         if (!closed)
         {
            proxy = p;
            p = null;
         }
      }
      if (p != null)
      {
         p.close();
         throw new ClosedChannelException();
      }
      proxy.setBufferingThresholds(1, 0); // send each keystroke immediately
   }

   /**
    * @see org.netxms.nxmc.modules.objects.ssh.TerminalTransport#getInputStream()
    */
   @Override
   public InputStream getInputStream()
   {
      return proxy.getInputStream();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.ssh.TerminalTransport#getOutputStream()
    */
   @Override
   public OutputStream getOutputStream()
   {
      return proxy.getOutputStream();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.ssh.TerminalTransport#resize(int, int)
    */
   @Override
   public void resize(int columns, int rows)
   {
      TcpProxy p;
      synchronized(this)
      {
         p = proxy;
      }
      if ((p == null) || p.isClosed())
         return;
      try
      {
         session.resizeSshTerminal(p, columns, rows);
      }
      catch(Exception e)
      {
         logger.debug("Cannot resize SSH terminal", e);
      }
   }

   /**
    * @see org.netxms.nxmc.modules.objects.ssh.TerminalTransport#close()
    */
   @Override
   public void close()
   {
      TcpProxy p;
      synchronized(this)
      {
         closed = true;
         p = proxy;
      }
      if (p != null)
         p.close();
   }
}
