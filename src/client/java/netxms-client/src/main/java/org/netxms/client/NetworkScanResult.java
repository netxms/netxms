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
package org.netxms.client;

import java.net.InetAddress;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.snmp.SnmpVersion;

/**
 * One host's accumulated state from an interactive network range scan.
 * Multiple updates may be delivered for the same address as additional probes
 * detect more protocols; each update reflects the full cumulative state.
 */
public class NetworkScanResult
{
   // Request flags (passed to NXCSession.scanNetworkRange)
   public static final int PROBE_AGENT = 0x0001;
   public static final int PROBE_SNMP = 0x0002;
   public static final int REPORT_UNREACHABLE = 0x0004;
   public static final int RESOLVE_HOSTNAMES = 0x0008;
   public static final int PROBE_MODBUS = 0x0010;
   public static final int PROBE_ETHERNET_IP = 0x0020;

   // Per-host result flags (carried in VID_FLAGS)
   private static final int HOST_REACHABLE = 0x0001;
   private static final int HAS_AGENT = 0x0002;
   private static final int HAS_SNMP = 0x0004;
   private static final int HAS_TCP_PORT_OPEN = 0x0008;
   private static final int HAS_MODBUS = 0x0010;
   private static final int HAS_ETHERNET_IP = 0x0020;

   private final InetAddress address;
   private final int flags;
   private final long rtt;
   private final long nodeId;
   private final int agentPort;
   private final SnmpVersion snmpVersion;
   private final int[] openTcpPorts;

   /**
    * Parse from CMD_RANGE_SCAN_RESULT message.
    *
    * @param msg incoming message
    */
   protected NetworkScanResult(NXCPMessage msg)
   {
      address = msg.getFieldAsInetAddress(NXCPCodes.VID_IP_ADDRESS);
      flags = msg.getFieldAsInt32(NXCPCodes.VID_FLAGS);
      rtt = msg.getFieldAsInt64(NXCPCodes.VID_RESPONSE_TIME);
      nodeId = msg.getFieldAsInt64(NXCPCodes.VID_NODE_ID);
      agentPort = msg.getFieldAsInt32(NXCPCodes.VID_AGENT_PORT);
      if ((flags & HAS_SNMP) != 0)
         snmpVersion = SnmpVersion.getByValue(msg.getFieldAsInt32(NXCPCodes.VID_SNMP_VERSION));
      else
         snmpVersion = null;
      int portCount = msg.getFieldAsInt32(NXCPCodes.VID_NUM_ELEMENTS);
      openTcpPorts = new int[portCount];
      long fieldId = NXCPCodes.VID_ELEMENT_LIST_BASE;
      for(int i = 0; i < portCount; i++)
         openTcpPorts[i] = msg.getFieldAsInt32(fieldId++);
   }

   /**
    * @return IP address of the scanned host
    */
   public InetAddress getAddress()
   {
      return address;
   }

   /**
    * @return ICMP round-trip time in milliseconds, or 0 if host did not respond to ICMP
    */
   public long getRtt()
   {
      return rtt;
   }

   /**
    * @return ID of existing node object with this IP address, or 0 if the address is not managed yet
    */
   public long getNodeId()
   {
      return nodeId;
   }

   /**
    * @return TCP port on which a NetXMS agent was detected, or 0 if no agent was detected
    */
   public int getAgentPort()
   {
      return agentPort;
   }

   /**
    * @return SNMP version detected on the host, or null if no SNMP agent was detected
    */
   public SnmpVersion getSnmpVersion()
   {
      return snmpVersion;
   }

   /**
    * @return list of open TCP ports detected on the host (from the user-supplied port list and the agent probe)
    */
   public int[] getOpenTcpPorts()
   {
      return openTcpPorts;
   }

   /**
    * @return true if the host responded to ICMP ping
    */
   public boolean isIcmpReachable()
   {
      return (flags & HOST_REACHABLE) != 0;
   }

   /**
    * @return true if a NetXMS agent was detected on the host
    */
   public boolean isAgentDetected()
   {
      return (flags & HAS_AGENT) != 0;
   }

   /**
    * @return true if an SNMP agent was detected on the host
    */
   public boolean isSnmpDetected()
   {
      return (flags & HAS_SNMP) != 0;
   }

   /**
    * @return true if a Modbus/TCP endpoint was detected on the host
    */
   public boolean isModbusDetected()
   {
      return (flags & HAS_MODBUS) != 0;
   }

   /**
    * @return true if an EtherNet/IP endpoint was detected on the host
    */
   public boolean isEtherNetIpDetected()
   {
      return (flags & HAS_ETHERNET_IP) != 0;
   }

   /**
    * @return true if at least one TCP port from the user-supplied list was found open
    */
   public boolean hasOpenTcpPorts()
   {
      return (flags & HAS_TCP_PORT_OPEN) != 0;
   }

   /**
    * @return raw result flags bitmask
    */
   public int getFlags()
   {
      return flags;
   }
}
