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

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.InetAddress;
import java.net.Socket;
import org.netxms.client.NXCSession;
import org.netxms.client.TcpProxy;
import org.netxms.client.objects.AbstractNode;
import org.netxms.nxmc.Registry;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import com.jcraft.jsch.ChannelShell;
import com.jcraft.jsch.JSch;
import com.jcraft.jsch.JSchException;
import com.jcraft.jsch.Session;
import com.jcraft.jsch.SocketFactory;
import com.jcraft.jsch.UIKeyboardInteractive;
import com.jcraft.jsch.UserInfo;

/**
 * Terminal transport with SSH client running in the console, connected to the node over TCP proxy channel. SSH session is
 * end-to-end encrypted between console and node; the proxy agent only forwards TCP stream. Host keys are kept in file
 * "known_hosts" in the console state directory: unknown keys are confirmed via host key verifier, changed keys are
 * rejected.
 */
public class DirectSshTransport implements TerminalTransport
{
   private static final Logger logger = LoggerFactory.getLogger(DirectSshTransport.class);

   private static final int CONNECT_TIMEOUT = 30000;

   private final NXCSession session;
   private final AbstractNode node;
   private final int port;
   private final String login;
   private final String password;
   private final String keyFile;
   private final HostKeyVerifier hostKeyVerifier;
   private TcpProxy proxy = null;
   private Session sshSession = null;
   private ChannelShell channel = null;
   private InputStream inputStream = null;
   private OutputStream outputStream = null;
   private String hostKeyError = null;

   /**
    * Create transport.
    *
    * @param session client session
    * @param node target node
    * @param port SSH port on target node
    * @param login login name
    * @param password password, or passphrase of private key if key file is given
    * @param keyFile private key file or null/empty for password authentication
    * @param hostKeyVerifier verifier for unknown host keys
    */
   public DirectSshTransport(NXCSession session, AbstractNode node, int port, String login, String password, String keyFile, HostKeyVerifier hostKeyVerifier)
   {
      this.session = session;
      this.node = node;
      this.port = port;
      this.login = (login != null) ? login : "";
      this.password = (password != null) ? password : "";
      this.keyFile = ((keyFile != null) && !keyFile.trim().isEmpty()) ? keyFile.trim() : null;
      this.hostKeyVerifier = hostKeyVerifier;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.ssh.TerminalTransport#connect(int, int)
    */
   @Override
   public void connect(int columns, int rows) throws Exception
   {
      InetAddress address = node.getPrimaryIP().getAddress();
      if ((address == null) || address.isAnyLocalAddress())
         throw new IOException("Node does not have valid primary IP address");
      String host = address.getHostAddress();

      try
      {
         JSch jsch = new JSch();
         File knownHosts = new File(Registry.getStateDir(), "known_hosts");
         if (!knownHosts.exists())
            knownHosts.createNewFile();
         jsch.setKnownHosts(knownHosts.getAbsolutePath());
         if (keyFile != null)
            jsch.addIdentity(keyFile, password);

         proxy = session.setupTcpProxy(node.getObjectId(), port);
         proxy.setBufferingThresholds(1, 0); // SSH layer does its own packetization

         sshSession = jsch.getSession(login, host, port);
         sshSession.setSocketFactory(new ProxySocketFactory(proxy));
         sshSession.setConfig("StrictHostKeyChecking", "ask");
         sshSession.setConfig("PreferredAuthentications", (keyFile != null) ? "publickey,password,keyboard-interactive" : "password,keyboard-interactive,publickey");
         if (keyFile == null)
            sshSession.setPassword(password);
         sshSession.setUserInfo(new ConsoleUserInfo());
         sshSession.connect(CONNECT_TIMEOUT);

         channel = (ChannelShell)sshSession.openChannel("shell");
         channel.setPtyType("xterm", columns, rows, 0, 0);
         inputStream = channel.getInputStream();
         outputStream = channel.getOutputStream();
         channel.connect(CONNECT_TIMEOUT);
         logger.debug("Direct SSH session established with {}:{} as {}", host, port, login);
      }
      catch(Exception e)
      {
         close();
         if (hostKeyError != null)
            throw new IOException(hostKeyError);
         if (e instanceof JSchException)
            throw new IOException(e.getMessage(), e);
         throw e;
      }
   }

   /**
    * @see org.netxms.nxmc.modules.objects.ssh.TerminalTransport#getInputStream()
    */
   @Override
   public InputStream getInputStream()
   {
      return inputStream;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.ssh.TerminalTransport#getOutputStream()
    */
   @Override
   public OutputStream getOutputStream()
   {
      return outputStream;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.ssh.TerminalTransport#resize(int, int)
    */
   @Override
   public void resize(int columns, int rows)
   {
      ChannelShell c = channel;
      if ((c != null) && c.isConnected())
         c.setPtySize(columns, rows, 0, 0);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.ssh.TerminalTransport#close()
    */
   @Override
   public void close()
   {
      if (channel != null)
      {
         channel.disconnect();
         channel = null;
      }
      if (sshSession != null)
      {
         sshSession.disconnect();
         sshSession = null;
      }
      if (proxy != null)
      {
         proxy.close();
         proxy = null;
      }
   }

   /**
    * Socket factory supplying TCP proxy streams instead of a real socket. The socket object handed to JSch is a
    * placeholder that never touches the network; all stream and option calls are intercepted.
    */
   private static class ProxySocketFactory implements SocketFactory
   {
      private final TcpProxy proxy;

      ProxySocketFactory(TcpProxy proxy)
      {
         this.proxy = proxy;
      }

      @Override
      public Socket createSocket(String host, int port) throws IOException
      {
         return new ProxySocket(proxy);
      }

      @Override
      public InputStream getInputStream(Socket socket) throws IOException
      {
         return proxy.getInputStream();
      }

      @Override
      public OutputStream getOutputStream(Socket socket) throws IOException
      {
         return proxy.getOutputStream();
      }
   }

   /**
    * Placeholder socket backed by TCP proxy channel
    */
   private static class ProxySocket extends Socket
   {
      private final TcpProxy proxy;

      ProxySocket(TcpProxy proxy)
      {
         this.proxy = proxy;
      }

      @Override
      public InputStream getInputStream() throws IOException
      {
         return proxy.getInputStream();
      }

      @Override
      public OutputStream getOutputStream() throws IOException
      {
         return proxy.getOutputStream();
      }

      @Override
      public synchronized void setSoTimeout(int timeout)
      {
      }

      @Override
      public void setTcpNoDelay(boolean on)
      {
      }

      @Override
      public void setKeepAlive(boolean on)
      {
      }

      @Override
      public boolean isConnected()
      {
         return !proxy.isClosed();
      }

      @Override
      public boolean isClosed()
      {
         return proxy.isClosed();
      }

      @Override
      public synchronized void close()
      {
         proxy.close();
      }
   }

   /**
    * User interaction for JSch: answers password prompts from settings and routes host key questions to the verifier.
    */
   private class ConsoleUserInfo implements UserInfo, UIKeyboardInteractive
   {
      @Override
      public String getPassphrase()
      {
         return password;
      }

      @Override
      public String getPassword()
      {
         return password;
      }

      @Override
      public boolean promptPassword(String message)
      {
         return true;
      }

      @Override
      public boolean promptPassphrase(String message)
      {
         return true;
      }

      @Override
      public boolean promptYesNo(String message)
      {
         if (message.contains("HAS CHANGED"))
         {
            hostKeyError = "Host key of " + node.getPrimaryIP().getHostAddress() + " has changed; remove the old key from " +
                  new File(Registry.getStateDir(), "known_hosts").getAbsolutePath() + " if the change is expected";
            logger.warn(hostKeyError);
            return false;
         }
         if (!hostKeyVerifier.confirmUnknownHostKey(message))
         {
            hostKeyError = "Host key rejected by user";
            return false;
         }
         return true;
      }

      @Override
      public void showMessage(String message)
      {
         logger.info("SSH server message: {}", message);
      }

      @Override
      public String[] promptKeyboardInteractive(String destination, String name, String instruction, String[] prompt, boolean[] echo)
      {
         if (prompt.length != 1)
            return null;
         return new String[] { password };
      }
   }
}
