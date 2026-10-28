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
 * LLM context window usage by AI assistant chat
 */
public class AiContextUsage
{
   private long tokens;
   private int window;

   /**
    * Create context usage object from NXCP message.
    *
    * @param msg NXCP message
    * @return context usage object or null if message does not contain context usage information
    */
   public static AiContextUsage createFromMessage(NXCPMessage msg)
   {
      return msg.isFieldPresent(NXCPCodes.VID_AI_CONTEXT_TOKENS) ? new AiContextUsage(msg) : null;
   }

   /**
    * Create context usage object from NXCP message.
    *
    * @param msg NXCP message
    */
   private AiContextUsage(NXCPMessage msg)
   {
      tokens = msg.getFieldAsInt64(NXCPCodes.VID_AI_CONTEXT_TOKENS);
      window = msg.getFieldAsInt32(NXCPCodes.VID_AI_CONTEXT_WINDOW);
   }

   /**
    * Get number of tokens occupied in context window by last LLM call.
    *
    * @return number of tokens
    */
   public long getTokens()
   {
      return tokens;
   }

   /**
    * Get context window size of the model that served last LLM call.
    *
    * @return context window size in tokens or 0 if unknown
    */
   public int getWindow()
   {
      return window;
   }

   /**
    * Check if context window size is known.
    *
    * @return true if context window size is known
    */
   public boolean isWindowKnown()
   {
      return window > 0;
   }

   /**
    * Get context window usage in percents.
    *
    * @return context window usage in percents or -1 if context window size is unknown
    */
   public int getUsagePercentage()
   {
      return (window > 0) ? (int)Math.min(tokens * 100 / window, 100) : -1;
   }

   /**
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "AiContextUsage [tokens=" + tokens + ", window=" + window + "]";
   }
}
