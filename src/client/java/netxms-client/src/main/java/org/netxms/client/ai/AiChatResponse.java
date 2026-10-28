/**
 * NetXMS - open source network management system
 * Copyright (C) 2013-2026 Raden Solutions
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
package org.netxms.client.ai;

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * Response from AI assistant within chat
 */
public class AiChatResponse
{
   private String text;
   private AiContextUsage contextUsage;

   /**
    * Create chat response object from NXCP message.
    *
    * @param msg NXCP message
    */
   public AiChatResponse(NXCPMessage msg)
   {
      text = msg.getFieldAsString(NXCPCodes.VID_MESSAGE);
      contextUsage = AiContextUsage.createFromMessage(msg);
   }

   /**
    * Get response text.
    *
    * @return response text
    */
   public String getText()
   {
      return text;
   }

   /**
    * Get context window usage after this response.
    *
    * @return context window usage or null if not provided by server
    */
   public AiContextUsage getContextUsage()
   {
      return contextUsage;
   }
}
