package org.netxms.tcpproxy;

import java.net.InetAddress;
import java.net.UnknownHostException;

/**
 * Immutable value object representing a single -L forwarding rule.
 */
public class ForwardingRule
{
   private final int localPort;
   private final String proxyNode;
   private final InetAddress remoteAddress;
   private final int remotePort;

   private ForwardingRule(int localPort, String proxyNode, InetAddress remoteAddress, int remotePort)
   {
      this.localPort = localPort;
      this.proxyNode = proxyNode;
      this.remoteAddress = remoteAddress;
      this.remotePort = remotePort;
   }

   /**
    * Parse a forwarding rule specification in the format localPort:proxyNode:remoteAddr:remotePort
    *
    * @param spec rule specification string
    * @return parsed forwarding rule
    * @throws IllegalArgumentException if the specification is malformed
    */
   public static ForwardingRule parse(String spec)
   {
      String[] parts = spec.split(":");
      if (parts.length != 4)
         throw new IllegalArgumentException("Invalid forwarding rule \"" + spec + "\": expected format localPort:proxyNode:remoteAddr:remotePort");

      int localPort;
      try
      {
         localPort = Integer.parseInt(parts[0]);
      }
      catch(NumberFormatException e)
      {
         throw new IllegalArgumentException("Invalid local port \"" + parts[0] + "\" in forwarding rule \"" + spec + "\"");
      }
      if (localPort < 1 || localPort > 65535)
         throw new IllegalArgumentException("Local port " + localPort + " out of range (1-65535) in forwarding rule \"" + spec + "\"");

      String proxyNode = parts[1];
      if (proxyNode.isEmpty())
         throw new IllegalArgumentException("Empty proxy node name in forwarding rule \"" + spec + "\"");

      InetAddress remoteAddress;
      try
      {
         remoteAddress = InetAddress.getByName(parts[2]);
      }
      catch(UnknownHostException e)
      {
         throw new IllegalArgumentException("Cannot resolve remote address \"" + parts[2] + "\" in forwarding rule \"" + spec + "\"");
      }

      int remotePort;
      try
      {
         remotePort = Integer.parseInt(parts[3]);
      }
      catch(NumberFormatException e)
      {
         throw new IllegalArgumentException("Invalid remote port \"" + parts[3] + "\" in forwarding rule \"" + spec + "\"");
      }
      if (remotePort < 1 || remotePort > 65535)
         throw new IllegalArgumentException("Remote port " + remotePort + " out of range (1-65535) in forwarding rule \"" + spec + "\"");

      return new ForwardingRule(localPort, proxyNode, remoteAddress, remotePort);
   }

   public int getLocalPort()
   {
      return localPort;
   }

   public String getProxyNode()
   {
      return proxyNode;
   }

   public InetAddress getRemoteAddress()
   {
      return remoteAddress;
   }

   public int getRemotePort()
   {
      return remotePort;
   }

   @Override
   public String toString()
   {
      return localPort + ":" + proxyNode + ":" + remoteAddress.getHostAddress() + ":" + remotePort;
   }
}
