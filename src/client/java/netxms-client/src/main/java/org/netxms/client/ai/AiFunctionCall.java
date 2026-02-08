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
 * AI assistant function call notification
 */
public class AiFunctionCall
{
   private long chatId;
   private String functionName;

   /**
    * Create function call object from NXCP message.
    *
    * @param msg NXCP message
    */
   public AiFunctionCall(NXCPMessage msg)
   {
      chatId = msg.getFieldAsInt64(NXCPCodes.VID_CHAT_ID);
      functionName = msg.getFieldAsString(NXCPCodes.VID_AI_FUNCTION_NAME);
   }

   /**
    * Get chat ID this function call belongs to.
    *
    * @return chat ID
    */
   public long getChatId()
   {
      return chatId;
   }

   /**
    * Get function name.
    *
    * @return function name
    */
   public String getFunctionName()
   {
      return functionName;
   }

   /**
    * Get user-friendly display name for the function.
    * Converts function names like "get-node-details" to "Getting node details"
    *
    * @return display name
    */
   public String getDisplayName()
   {
      if (functionName == null || functionName.isEmpty())
         return "Processing";

      // Convert snake_case and kebab-case to readable text
      String readable = functionName.replace('-', ' ').replace('_', ' ');

      // Convert to progressive tense for common verbs
      String lower = readable.toLowerCase();
      if (lower.startsWith("get "))
         return "Getting" + readable.substring(3);
      if (lower.startsWith("read "))
         return "Reading" + readable.substring(4);
      if (lower.startsWith("find "))
         return "Finding" + readable.substring(4);
      if (lower.startsWith("list "))
         return "Listing" + readable.substring(4);
      if (lower.startsWith("search "))
         return "Searching" + readable.substring(6);
      if (lower.startsWith("analyze "))
         return "Analyzing" + readable.substring(7);
      if (lower.startsWith("query "))
         return "Querying" + readable.substring(5);
      if (lower.startsWith("check "))
         return "Checking" + readable.substring(5);
      if (lower.startsWith("create "))
         return "Creating" + readable.substring(6);
      if (lower.startsWith("update "))
         return "Updating" + readable.substring(6);
      if (lower.startsWith("delete "))
         return "Deleting" + readable.substring(6);
      if (lower.startsWith("send "))
         return "Sending" + readable.substring(4);
      if (lower.startsWith("load "))
         return "Loading" + readable.substring(4);

      // Capitalize first letter for unknown verbs
      return Character.toUpperCase(readable.charAt(0)) + readable.substring(1);
   }
}
