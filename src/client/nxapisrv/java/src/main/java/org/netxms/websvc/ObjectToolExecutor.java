/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2019 Raden Solutions
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
package org.netxms.websvc;

import org.netxms.base.NXCommon;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.client.objecttools.ObjectToolDetails;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Map;

/**
 * Object tool execution thread
 */
public class ObjectToolExecutor extends Thread
{
   private static String MULTIPLE_NODES = "<Multiple nodes>";

   private Logger log = LoggerFactory.getLogger(ObjectToolExecutor.class);
   private ObjectToolDetails details;
   private long objectId;
   private Map<String, String> inputFields;
   private ObjectToolOutputListener listener;
   private NXCSession session;
   private boolean generateOutput;

   /**
    * Create and start new object tool execution thread
    * 
    * @param details
    * @param objectId
    * @param inputFields
    * @param listener
    * @param session
    */
   public ObjectToolExecutor(ObjectToolDetails details, long objectId, Map<String, String> inputFields,
         ObjectToolOutputListener listener, NXCSession session)
   {
      this.details = details;
      this.objectId = objectId;
      this.inputFields = inputFields;
      this.listener = listener;
      this.session = session;
      generateOutput = (details.getFlags() & ObjectTool.GENERATES_OUTPUT) != 0;
      setDaemon(true);
      start();
   }

   /**
    * Create and start new object tool execution thread
    * 
    * @param details
    * @param objectId
    * @param inputFields
    * @param session
    */
   public ObjectToolExecutor(ObjectToolDetails details, long objectId, Map<String, String> inputFields, NXCSession session)
   {
      this.details = details;
      this.objectId = objectId;
      this.inputFields = inputFields;
      this.listener = null;
      this.session = session;
      generateOutput = (details.getFlags() & ObjectTool.GENERATES_OUTPUT) != 0;
      setDaemon(true);
      start();
   }

   /* (non-Javadoc)
    * @see java.lang.Thread#run()
    */
   @Override public void run()
   {
      switch(details.getToolType())
      {
         case ObjectTool.TYPE_ACTION:
            executeAgentAction();
            break;
         /*case ObjectTool.TYPE_FILE_DOWNLOAD:
            executeFileDownload(node, tool, inputValues);
            break;*/
         case ObjectTool.TYPE_SERVER_COMMAND:
            executeServerCommand();
            break;
         /*case ObjectTool.TYPE_SERVER_SCRIPT:
            executeServerScript(node, tool, inputValues);
            break;
         case ObjectTool.TYPE_TABLE_AGENT:
         case ObjectTool.TYPE_TABLE_SNMP:
            executeTableTool(node, tool);
            break;
         case ObjectTool.TYPE_URL:
            openURL(node, tool, inputValues, expandedValue);
            break;*/
      }
      
      if (generateOutput && (listener != null))
      {
         listener.onComplete();
      }
   }

   /**
    * Execute agent action
    */
   private void executeAgentAction()
   {
      String[] parts = splitCommandLine(substituteMacros(details.getData(), inputFields));
      final String action = parts[0];
      final String[] args = Arrays.copyOfRange(parts, 1, parts.length);
      try
      {
         session.executeAction(objectId, action, args, generateOutput, listener, null);
      }
      catch(Exception e)
      {
         log.error(e.getMessage(), e);
      }
   }

   /**
    * Execute server command
    */
   private void executeServerCommand()
   {
      try
      {
         session.executeServerCommand(objectId, details.getData(), inputFields, generateOutput, listener, null);
      }
      catch(Exception e)
      {
         log.error(e.getMessage(), e);
      }
   }

   /**
    * Substitute macros in string
    *
    * @param s
    * @param inputValues
    * @return
    */
   private String substituteMacros(String s, Map<String, String> inputValues)
   {
      AbstractNode object = session.findObjectById(objectId, AbstractNode.class);
      StringBuilder sb = new StringBuilder();

      char[] src = s.toCharArray();
      for(int i = 0; i < s.length(); i++)
      {
         if (src[i] == '%')
         {
            i++;
            if (i == s.length())
               break;   // malformed string

            switch(src[i])
            {
               case 'a':
                  sb.append((object != null) ? object.getPrimaryIP().getHostAddress() : MULTIPLE_NODES);
                  break;
               case 'g':
                  sb.append((object != null) ? object.getGuid().toString() : MULTIPLE_NODES);
                  break;
               case 'i':
                  sb.append((object != null) ? String.format("0x%08X", object.getObjectId()) : MULTIPLE_NODES); //$NON-NLS-1$
                  break;
               case 'I':
                  sb.append((object != null) ? Long.toString(object.getObjectId()) : MULTIPLE_NODES);
                  break;
               case 'n':
                  sb.append((object != null) ? object.getObjectName() : MULTIPLE_NODES);
                  break;
               case 'N':
                  break;
               case 'U':
                  sb.append(session.getUserName());
                  break;
               case 'v':
                  sb.append(NXCommon.VERSION);
                  break;
               case '%':
                  sb.append('%');
                  break;
               case '{':   // object's custom attribute
                  StringBuilder attr = new StringBuilder();
                  for(i++; i < s.length(); i++)
                  {
                     if (src[i] == '}')
                        break;
                     attr.append(src[i]);
                  }
                  if ((object != null) && (attr.length() > 0))
                  {
                     String value = object.getCustomAttributes().get(attr.toString());
                     if (value != null)
                        sb.append(value);
                  }
                  break;
               case '(':   // input field
                  StringBuilder name = new StringBuilder();
                  for(i++; i < s.length(); i++)
                  {
                     if (src[i] == ')')
                        break;
                     name.append(src[i]);
                  }
                  if (name.length() > 0)
                  {
                     String value = inputValues.get(name.toString());
                     if (value != null)
                        sb.append(value);
                  }
                  break;
               default:
                  break;
            }
         }
         else
         {
            sb.append(src[i]);
         }
      }

      return sb.toString();
   }

   /**
    * Split command line into tokens
    *
    * @param input
    * @return
    */
   private static String[] splitCommandLine(String input)
   {
      char[] in = input.toCharArray();
      List<String> args = new ArrayList<String>();

      StringBuilder sb = new StringBuilder();
      int state = 0;
      for(char c : in)
      {
         switch(state)
         {
            case 0: // normal
               if (Character.isSpaceChar(c))
               {
                  args.add(sb.toString());
                  sb = new StringBuilder();
                  state = 3;
               }
               else if (c == '"')
               {
                  state = 1;
               }
               else if (c == '\'')
               {
                  state = 2;
               }
               else
               {
                  sb.append(c);
               }
               break;
            case 1: // double quoted string
               if (c == '"')
               {
                  state = 0;
               }
               else
               {
                  sb.append(c);
               }
               break;
            case 2: // single quoted string
               if (c == '\'')
               {
                  state = 0;
               }
               else
               {
                  sb.append(c);
               }
               break;
            case 3: // skip
               if (!Character.isSpaceChar(c))
               {
                  if (c == '"')
                  {
                     state = 1;
                  }
                  else if (c == '\'')
                  {
                     state = 2;
                  }
                  else
                  {
                     sb.append(c);
                     state = 0;
                  }
               }
               break;
         }
      }
      if (state != 3)
         args.add(sb.toString());

      return args.toArray(new String[args.size()]);
   }
}
