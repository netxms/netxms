/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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
import java.util.Date;
import org.netxms.base.NXCPMessage;

/**
 * User session (as reported by agent)
 */
public class UserSession
{
   private int id;
   private String loginName;
   private String terminal;
   private boolean connected;
   private String clientName;
   private InetAddress clientAddress;
   private int displayWidth;
   private int displayHeight;
   private int displayColorDepth;
   private Date connectTime;
   private Date loginTime;
   private int idleTime;
   private long agentPID;
   private int agentType;

   /**
    * Create session information object from NXCP message.
    * 
    * @param msg NXCP message
    * @param baseId base field ID
    */
   protected UserSession(NXCPMessage msg, long baseId)
   {
      id = msg.getFieldAsInt32(baseId);
      loginName = msg.getFieldAsString(baseId + 1);
      terminal = msg.getFieldAsString(baseId + 2);
      connected = msg.getFieldAsBoolean(baseId + 3);
      clientName = msg.getFieldAsString(baseId + 4);
      clientAddress = msg.getFieldAsInetAddress(baseId + 5);
      displayWidth = msg.getFieldAsInt32(baseId + 6);
      displayHeight = msg.getFieldAsInt32(baseId + 7);
      displayColorDepth = msg.getFieldAsInt32(baseId + 8);
      connectTime = msg.getFieldAsDate(baseId + 9);
      loginTime = msg.getFieldAsDate(baseId + 10);
      idleTime = msg.getFieldAsInt32(baseId + 11);
      agentPID = msg.getFieldAsInt64(baseId + 12);
      agentType = msg.getFieldAsInt32(baseId + 13);
   }

   /**
    * @return the id
    */
   public int getId()
   {
      return id;
   }

   /**
    * @return the loginName
    */
   public String getLoginName()
   {
      return loginName;
   }

   /**
    * @return the terminal
    */
   public String getTerminal()
   {
      return terminal;
   }

   /**
    * @return the connected
    */
   public boolean isConnected()
   {
      return connected;
   }

   /**
    * @return the clientName
    */
   public String getClientName()
   {
      return clientName;
   }

   /**
    * @return the clientAddress
    */
   public InetAddress getClientAddress()
   {
      return clientAddress;
   }

   /**
    * @return the displayWidth
    */
   public int getDisplayWidth()
   {
      return displayWidth;
   }

   /**
    * @return the displayHeight
    */
   public int getDisplayHeight()
   {
      return displayHeight;
   }

   /**
    * @return the displayColorDepth
    */
   public int getDisplayColorDepth()
   {
      return displayColorDepth;
   }

   /**
    * Get textual representation of display parameters (WxHxC)
    *
    * @return textual representation of display parameters (WxHxC) or empty string
    */
   public String getDisplayDescription()
   {
      if ((displayHeight <= 0) || (displayWidth <= 0) || (displayColorDepth <= 0))
         return "";
      return displayWidth + "x" + displayHeight + "x" + displayColorDepth;
   }

   /**
    * @return the connectTime
    */
   public Date getConnectTime()
   {
      return connectTime;
   }

   /**
    * @return the loginTime
    */
   public Date getLoginTime()
   {
      return loginTime;
   }

   /**
    * @return the idleTime
    */
   public int getIdleTime()
   {
      return idleTime;
   }

   /**
    * @return the agentPID
    */
   public long getAgentPID()
   {
      return agentPID;
   }

   /**
    * @return the agentType
    */
   public int getAgentType()
   {
      return agentType;
   }
}
