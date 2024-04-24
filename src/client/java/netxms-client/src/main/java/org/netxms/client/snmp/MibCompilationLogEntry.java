/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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
package org.netxms.client.snmp;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * MIB compilation entry log
 */
public class MibCompilationLogEntry
{   
   private static Logger logger = LoggerFactory.getLogger(MibCompilationLogEntry.class);

   private MessageType type;
   private int errorCode;
   private String fileName;
   private String text;

   /**
    * Constructor
    */
   private MibCompilationLogEntry()
   {
      type = null;
      errorCode = 0;
      fileName = null;
      text = null;
   }

   /**
    * Create MIB compilation entry 
    * 
    * @param output log entry line
    * @return log entry class
    */
   public static MibCompilationLogEntry createEntry(String output)
   {
      int separator = output.indexOf(":");
      if (separator == -1)
         return null;

      MessageType type = MessageType.getValue(output.substring(0, separator));
      if (type == null)
         return null;

      output = output.substring(separator + 1);
      MibCompilationLogEntry entry = new MibCompilationLogEntry();
      entry.type = type;
      switch (type)
      {
         case STATE:
            String[] items = output.split(":");
            if (items.length < 2)
               return null;
            try 
            {
               entry.errorCode = Integer.parseInt(items[0]);
            }
            catch (NumberFormatException e)
            {
               logger.debug("Failed to parse error code: " + items[0]);
            }
            entry.text = items[1];
            break;
         case STAGE:
            entry.text = output;
            break;
         case FILE:
            entry.fileName = output;
            break;
         case WARNING:
         case ERROR:
            int end = output.indexOf(":");
            if (end == -1)
               return null;
            try 
            {
               entry.errorCode = Integer.parseInt(output.substring(0, end));
            }
            catch (NumberFormatException e)
            {
               logger.debug("Failed to parse error code: " + output.substring(0, end));
            }
            int start = output.indexOf(":{");
            end = output.indexOf("}:");
            if (start == -1 || end == -1)
               return null;
            entry.fileName = output.substring(start + 2, end);
            entry.text = output.substring(end + 2);
            break;
      }  
      return entry;      
   }

   /**
    * MIB compiler message types
    */
   public enum MessageType
   {
      STATE("S"), STAGE("P"), FILE("F"), WARNING("W"), ERROR("E");

      private String symbol;
      
      MessageType(String string)
      {
         this.symbol = string;
      }

      public static MessageType getValue(String text)
      {
         for(MessageType type : MessageType.values())
         {
            if (type.symbol.equalsIgnoreCase(text))
            {
               return type;
            }
         }
         return null;
      }
   }

   /**
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "MibCompilationLogEntry [type=" + type + ", errorCode=" + errorCode + ", fileName=" + fileName + ", text=" + text + "]";
   }

   /**
    * @return the type
    */
   public boolean isErrorInformation()
   {
      return type == MessageType.ERROR || type == MessageType.WARNING;
   }

   /**
    * @return the type
    */
   public MessageType getType()
   {
      return type;
   }

   /**
    * @return the errorCode
    */
   public int getErrorCode()
   {
      return errorCode;
   }

   /**
    * @return the fileName
    */
   public String getFileName()
   {
      return fileName;
   }

   /**
    * @return the text
    */
   public String getText()
   {
      return text;
   }   
}
