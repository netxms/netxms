/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objecttools;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import org.apache.commons.lang3.SystemUtils;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.window.Window;
import org.netxms.client.AgentFileData;
import org.netxms.client.InputField;
import org.netxms.client.NXCSession;
import org.netxms.client.ProgressListener;
import org.netxms.client.constants.InputFieldType;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.ViewPlacement;
import org.netxms.nxmc.base.widgets.MessageArea;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.filemanager.views.AgentFileViewer;
import org.netxms.nxmc.modules.objects.ObjectContext;
import org.netxms.nxmc.modules.objects.dialogs.InputFieldEntryDialog;
import org.netxms.nxmc.modules.objects.views.SSHCommandResults;
import org.netxms.nxmc.modules.objecttools.views.AgentActionResults;
import org.netxms.nxmc.modules.objecttools.views.LocalCommandResults;
import org.netxms.nxmc.modules.objecttools.views.MultiNodeCommandExecutor;
import org.netxms.nxmc.modules.objecttools.views.ServerCommandResults;
import org.netxms.nxmc.modules.objecttools.views.ServerScriptResults;
import org.netxms.nxmc.modules.objecttools.views.TableToolResults;
import org.netxms.nxmc.services.ObjectToolHandler;
import org.netxms.nxmc.tools.ExternalWebBrowser;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Executor for object tool
 */
public final class ObjectToolExecutor
{
   /**
    * Private constructor to forbid instantiation 
    */
   private ObjectToolExecutor()
   {
   }

   /**
    * Check if tool is allowed for execution on at least one object from set
    * 
    * @param tool
    * @param objects
    * @return
    */
   public static boolean isToolAllowed(ObjectTool tool, Set<ObjectContext> objects)
   {
      if (tool.getToolType() != ObjectTool.TYPE_INTERNAL)
         return true;

      ObjectToolHandler handler = ObjectToolsCache.findHandler(tool.getData());
      if (handler != null)
      {
         for(ObjectContext n : objects)
            if (n.isNode() && handler.canExecuteOnNode((AbstractNode)n.object, tool))
               return true;
         return false;
      }
      else
      {
         return false;
      }
   }

   /**
    * Check if given tool is applicable for at least one object in set
    * 
    * @param tool
    * @param objects
    * @return
    */
   public static boolean isToolApplicable(ObjectTool tool, Set<ObjectContext> objects)
   {
      for(ObjectContext n : objects)
         if (tool.isApplicableForObject(n.object))
            return true;
      return false;
   }

   /**
    * Execute object tool on object set
    * 
    * @param allObjects objects that involved in selection
    * @param nodes objects to execution tool on
    * @param tool Object tool
    * @param viewPlacement view placement information
    */
   public static void execute(final Set<ObjectContext> allObjects, Set<ObjectContext> nodes, final ObjectTool tool, final ViewPlacement viewPlacement)
   {
      // Filter allowed and applicable nodes for execution
      I18n i18n = LocalizationHelper.getI18n(ObjectToolExecutor.class);
      final Set<ObjectContext> objects = new HashSet<ObjectContext>();
      ObjectToolHandler handler = ObjectToolsCache.findHandler(tool.getData());
      if ((tool.getToolType() != ObjectTool.TYPE_INTERNAL) || handler != null)
      {
         for(ObjectContext n : nodes)
            if (((tool.getToolType() != ObjectTool.TYPE_INTERNAL) || (n.isNode() && handler.canExecuteOnNode((AbstractNode)n.object, tool))) && tool.isApplicableForObject(n.object))
               objects.add(n);
      }
      else
      {
         return;
      }

      final List<String> maskedFields = new ArrayList<String>();
      final Map<String, String> inputValues;
      final InputField[] fields = tool.getInputFields();
      if (fields.length > 0)
      {
         Arrays.sort(fields, new Comparator<InputField>() {
            @Override
            public int compare(InputField f1, InputField f2)
            {
               return f1.getSequence() - f2.getSequence();
            }
         });
         inputValues = readInputFields(tool.getDisplayName(), fields);
         if (inputValues == null)
            return;  // cancelled
         for (int i = 0; i < fields.length; i++)
         {
            if (fields[i].getType() == InputFieldType.PASSWORD)
            {
               maskedFields.add(fields[i].getName());
            }
         }
      }
      else
      {
         inputValues = new HashMap<String, String>(0);
      }
      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Execute object tool"), null) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {      
            List<String> expandedText = null;

            if (((tool.getFlags() & ObjectTool.ASK_CONFIRMATION) != 0) || (objects.size() >= 10))
            {
               String message = tool.getConfirmationText();
               if (objects.size() == 1)
               {
                  // Expand message and action for 1 node, otherwise expansion occurs after confirmation
                  List<String> textToExpand = new ArrayList<String>();
                  textToExpand.add(tool.getConfirmationText());
                  if ((tool.getToolType() == ObjectTool.TYPE_URL) || (tool.getToolType() == ObjectTool.TYPE_LOCAL_COMMAND && ((tool.getFlags() & ObjectTool.GENERATES_OUTPUT) == 0)))
                  {
                     textToExpand.add(tool.getData());
                  }
                  ObjectContext node = objects.iterator().next();
                  expandedText = session.substituteMacros(node, textToExpand, inputValues);
                  message = expandedText.remove(0);                  
               }
               else
               {
                  if ((tool.getFlags() & ObjectTool.ASK_CONFIRMATION) != 0)
                  {
                     ObjectContext node = objects.iterator().next();
                     message = node.substituteMacrosForMultipleNodes(message, inputValues, getDisplay());
                  }
                  else
                  {
                     // No confirmation required but execution is on big number of objects
                     message = i18n.tr("You attempt to execute tool on {0} objects. Are you sure?", Integer.toString(objects.size()));
                  }

                  if ((tool.getToolType() == ObjectTool.TYPE_URL) || (tool.getToolType() == ObjectTool.TYPE_LOCAL_COMMAND && ((tool.getFlags() & ObjectTool.GENERATES_OUTPUT) == 0)))
                  {
                     expandedText = session.substituteMacros(objects.toArray(new ObjectContext[objects.size()]), tool.getData(), inputValues);
                  }
               }

               ConfirmationRunnable runnable = new ConfirmationRunnable(message);
               getDisplay().syncExec(runnable);
               if (!runnable.isConfirmed())
                  return;
            }
            else
            {
               if ((tool.getToolType() == ObjectTool.TYPE_URL) || (tool.getToolType() == ObjectTool.TYPE_LOCAL_COMMAND && ((tool.getFlags() & ObjectTool.GENERATES_OUTPUT) == 0)))
               {
                  expandedText = session.substituteMacros(objects.toArray(new ObjectContext[objects.size()]), tool.getData(), inputValues);
               }
            }

            // Check if password validation needed
            boolean validationNeeded = false;
            for(int i = 0; i < fields.length; i++)
               if (fields[i].isPasswordValidationNeeded())
               {
                  validationNeeded = true;
                  break;
               }

            if (validationNeeded)
            {
               for(int i = 0; i < fields.length; i++)
               {
                  if ((fields[i].getType() == InputFieldType.PASSWORD) && fields[i].isPasswordValidationNeeded())
                  {
                     boolean valid = session.validateUserPassword(inputValues.get(fields[i].getName()));
                     if (!valid)
                     {
                        final String fieldName = fields[i].getDisplayName();
                        getDisplay().syncExec(new Runnable() {
                           @Override
                           public void run()
                           {
                              MessageDialogHelper.openError(null, i18n.tr("Error"),
                                    String.format(i18n.tr("Password entered in input field \"%s\" is not valid"), fieldName));
                           }
                        });
                        return;
                     }
                  }
               }
            }

            int i = 0;
            if ((objects.size() > 1) &&
                  (tool.getToolType() == ObjectTool.TYPE_LOCAL_COMMAND || tool.getToolType() == ObjectTool.TYPE_SERVER_COMMAND || tool.getToolType() == ObjectTool.TYPE_SSH_COMMAND ||
                  tool.getToolType() == ObjectTool.TYPE_ACTION || tool.getToolType() == ObjectTool.TYPE_SERVER_SCRIPT) && 
                  ((tool.getFlags() & ObjectTool.GENERATES_OUTPUT) != 0 || tool.getToolType() == ObjectTool.TYPE_SSH_COMMAND))
            {
               getDisplay().syncExec(new Runnable() {
                  @Override
                  public void run()
                  {
                     executeOnMultipleNodes(allObjects, objects, tool, inputValues, maskedFields, viewPlacement);
                  }
               });
            }
            else
            {
               for(final ObjectContext n : objects)
               {
                  if (tool.getToolType() == ObjectTool.TYPE_URL || (tool.getToolType() == ObjectTool.TYPE_LOCAL_COMMAND && ((tool.getFlags() & ObjectTool.GENERATES_OUTPUT) == 0)))
                  {
                     final String data = expandedText.get(i++);
                     getDisplay().syncExec(new Runnable() {
                        @Override
                        public void run()
                        {
                           executeOnNode(n, tool, inputValues, maskedFields, data, viewPlacement);
                        }
                     });
                  }
                  else
                  {
                     getDisplay().syncExec(new Runnable() {
                        @Override
                        public void run()
                        {
                           executeOnNode(n, tool, inputValues, maskedFields, null, viewPlacement);
                        }
                     });                  
                  }
               }
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Object tool execution failed");
         }

         class ConfirmationRunnable implements Runnable
         {
            private boolean confirmed;
            private String message;

            public ConfirmationRunnable(String message)
            {
               this.message = message;
            }

            @Override
            public void run()
            {
               confirmed = MessageDialogHelper.openQuestion(Registry.getMainWindowShell(), i18n.tr("Confirm Tool Execution"), message);
            }
            
            boolean isConfirmed()
            {
               return confirmed;
            }
         }         
         
      }.start();
   }

   /**
    * Read input fields
    * 
    * @param title Input dialog title
    * @param fields Input fields to read
    * @return values for input fields
    */
   private static Map<String, String> readInputFields(String title, InputField[] fields)
   {
      InputFieldEntryDialog dlg = new InputFieldEntryDialog(Registry.getMainWindowShell(), title, fields);
      if (dlg.open() != Window.OK)
         return null;
      return dlg.getValues();
   }

   /**
    * Execute object tool on single node
    * 
    * @param node node to execute at
    * @param tool object tool
    * @param inputValues input values
    * @param maskedFields list of input fields to be masked
    * @param expandedToolData expanded tool data
    * @param viewPlacement view placement information
    */
   private static void executeOnNode(final ObjectContext node, final ObjectTool tool, Map<String, String> inputValues,
         List<String> maskedFields, String expandedToolData, final ViewPlacement viewPlacement)
   {
      switch(tool.getToolType())
      {
         case ObjectTool.TYPE_ACTION:
            executeAgentAction(node, tool, inputValues, maskedFields, viewPlacement);
            break;
         case ObjectTool.TYPE_FILE_DOWNLOAD:
            executeFileDownload(node, tool, inputValues, viewPlacement);
            break;
         case ObjectTool.TYPE_INTERNAL:
            executeInternalTool(node, tool, inputValues, viewPlacement);
            break;
         case ObjectTool.TYPE_LOCAL_COMMAND:
            executeLocalCommand(node, tool, inputValues, expandedToolData, viewPlacement);
            break;
         case ObjectTool.TYPE_SERVER_COMMAND:
            executeServerCommand(node, tool, inputValues, maskedFields, viewPlacement);
            break;
         case ObjectTool.TYPE_SSH_COMMAND:
            executeSshCommand(node, tool, inputValues, maskedFields, viewPlacement);
            break;
         case ObjectTool.TYPE_SERVER_SCRIPT:
            executeServerScript(node, tool, inputValues, maskedFields, viewPlacement);
            break;
         case ObjectTool.TYPE_AGENT_LIST:
         case ObjectTool.TYPE_AGENT_TABLE:
         case ObjectTool.TYPE_SNMP_TABLE:
            executeTableTool(node, tool, viewPlacement);
            break;
         case ObjectTool.TYPE_URL:
            openURL(node, tool, expandedToolData, viewPlacement);
            break;
      }
   }

   /**
    * Execute object tool on set of nodes
    *
    * @param nodes set of nodes
    * @param tool tool to execute
    * @param inputValues input values
    * @param maskedFields list of input fields to be masked
    * @param expandedToolData expanded tool data
    * @param viewPlacement view placement information
    */
   private static void executeOnMultipleNodes(Set<ObjectContext> sourceObjects, Set<ObjectContext> nodes, ObjectTool tool, Map<String, String> inputValues,
         List<String> maskedFields, ViewPlacement viewPlacement)
   {
      viewPlacement.openView(new MultiNodeCommandExecutor(tool, sourceObjects, nodes, inputValues, maskedFields));
   }

   /**
    * Execute table tool
    * 
    * @param node
    * @param tool
    */
   private static void executeTableTool(final ObjectContext node, final ObjectTool tool, ViewPlacement viewPlacement)
   {
      viewPlacement.openView(new TableToolResults(node, tool));
   }
   
   /**
    * Execute agent action.
    *
    * @param node
    * @param tool
    * @param inputValues
    * @param maskedFields
    * @param viewPlacement view placement information
    */
   private static void executeAgentAction(final ObjectContext node, final ObjectTool tool, final Map<String, String> inputValues, final List<String> maskedFields, final ViewPlacement viewPlacement)
   {
      final NXCSession session = Registry.getSession();
      I18n i18n = LocalizationHelper.getI18n(ObjectToolExecutor.class);

      if ((tool.getFlags() & ObjectTool.GENERATES_OUTPUT) == 0)
      {
         new Job(String.format(i18n.tr("Execute command on node %s"), node.object.getObjectName()), null, viewPlacement.getMessageAreaHolder()) {
            @Override
            protected String getErrorMessage()
            {
               return String.format(i18n.tr("Cannot execute command on node %s"), node.object.getObjectName());
            }

            @Override
            protected void run(IProgressMonitor monitor) throws Exception
            {
               final String action = session.executeActionWithExpansion(node.object.getObjectId(), node.getAlarmId(), tool.getData(), inputValues, maskedFields);
               if ((tool.getFlags() & ObjectTool.SUPPRESS_SUCCESS_MESSAGE) == 0)
               {
                  runInUIThread(() -> {
                     String message = i18n.tr("Action {0} executed successfully on node {1}", action, node.object.getObjectName());
                     viewPlacement.getMessageAreaHolder().addMessage(MessageArea.SUCCESS, message);
                  });
               }
            }
         }.start();
      }
      else
      {
         viewPlacement.openView(new AgentActionResults(node, tool, inputValues, maskedFields));
      }
   }

   /**
    * Execute server command
    * 
    * @param node
    * @param tool
    * @param inputValues
    * @param maskedFields
    * @param viewPlacement view placement information
    */
   private static void executeServerCommand(final ObjectContext node, final ObjectTool tool, final Map<String, String> inputValues, final List<String> maskedFields, final ViewPlacement viewPlacement)
   {
      final NXCSession session = Registry.getSession();
      I18n i18n = LocalizationHelper.getI18n(ObjectToolExecutor.class);
      if ((tool.getFlags() & ObjectTool.GENERATES_OUTPUT) == 0)
      {      
         new Job(i18n.tr("Executing server command"), null, viewPlacement.getMessageAreaHolder()) {
            @Override
            protected void run(IProgressMonitor monitor) throws Exception
            {
               session.executeServerCommand(node.object.getObjectId(), node.getAlarmId(), tool.getData(), inputValues, maskedFields);
               if ((tool.getFlags() & ObjectTool.SUPPRESS_SUCCESS_MESSAGE) == 0)
               {
                  runInUIThread(() -> {
                     String message = i18n.tr("Server command {0} executed successfully on node {1}", tool.getDisplayName(), node.object.getObjectName());
                     viewPlacement.getMessageAreaHolder().addMessage(MessageArea.SUCCESS, message);
                  });
               }
            }
            
            @Override
            protected String getErrorMessage()
            {
               return String.format(i18n.tr("Cannot execute server command for node %s"), node.object.getObjectName());
            }
         }.start();
      }
      else
      {
         viewPlacement.openView(new ServerCommandResults(node, tool, inputValues, maskedFields));
      }
   }

   /**
    * Execute SSH command
    * 
    * @param node target node
    * @param tool tool information
    * @param inputValues input values provided by user
    * @param viewPlacement view placement information
    */
   private static void executeSshCommand(final ObjectContext node, final ObjectTool tool, final Map<String, String> inputValues, final List<String> maskedFields, final ViewPlacement viewPlacement)
   {
      final NXCSession session = Registry.getSession();
      I18n i18n = LocalizationHelper.getI18n(ObjectToolExecutor.class);

      if ((tool.getFlags() & ObjectTool.GENERATES_OUTPUT) == 0)
      {
         new Job(String.format(i18n.tr("Executing SSH command on node %s"), node.object.getObjectName()), null, viewPlacement.getMessageAreaHolder()) {
            @Override
            protected void run(IProgressMonitor monitor) throws Exception
            {
               try
               {
                  session.executeSshCommand(node.object.getObjectId(), node.getAlarmId(), tool.getData(), inputValues, maskedFields, false, null, null);

                  runInUIThread(new Runnable() {
                     @Override
                     public void run()
                     {
                        viewPlacement.getMessageAreaHolder().addMessage(MessageArea.SUCCESS, String.format(i18n.tr("SSH command %s executed successfully on node %s"), tool.getData(), node.object.getObjectName()));
                     }
                  });
               }
               catch(Exception e)
               {
                  runInUIThread(new Runnable() {
                     @Override
                     public void run()
                     {
                        viewPlacement.getMessageAreaHolder().addMessage(MessageArea.ERROR, String.format(i18n.tr("Filed to execute on node %s: %s"), node.object.getObjectName(), e.getLocalizedMessage()));
                     }
                  });
               }
            }

            @Override
            protected String getErrorMessage()
            {
               return String.format(i18n.tr("Cannot execute SSH command on node %s"), node.object.getObjectName());
            }
         }.start();
      }
      else
      {
         viewPlacement.openView(new SSHCommandResults(node, tool, inputValues, null));
      }
   }

   /**
    * Execute server script
    * 
    * @param node node to execute at
    * @param tool object tool
    * @param inputValues input values
    * @param viewPlacement view placement information
    */
   private static void executeServerScript(final ObjectContext node, final ObjectTool tool, final Map<String, String> inputValues, List<String> maskedFields, ViewPlacement viewPlacement)
   {
      final NXCSession session = Registry.getSession();
      I18n i18n = LocalizationHelper.getI18n(ObjectToolExecutor.class);
      if ((tool.getFlags() & ObjectTool.GENERATES_OUTPUT) == 0)
      {      
         new Job(i18n.tr("Execute server script"), null, viewPlacement.getMessageAreaHolder()) {
            @Override
            protected void run(IProgressMonitor monitor) throws Exception
            {
               session.executeLibraryScript(node.object.getObjectId(), node.getAlarmId(), tool.getData(), inputValues, maskedFields, null);
               if ((tool.getFlags() & ObjectTool.SUPPRESS_SUCCESS_MESSAGE) == 0)
               {
                  runInUIThread(() -> viewPlacement.getMessageAreaHolder().addMessage(MessageArea.SUCCESS,
                        String.format(i18n.tr("Server script executed successfully on node %s"), node.object.getObjectName())));
               }
            }

            @Override
            protected String getErrorMessage()
            {
               return String.format(i18n.tr("Cannot execute server script for node %s"), node.object.getObjectName());
            }
         }.start();
      }
      else
      {
         viewPlacement.openView(new ServerScriptResults(node, tool, inputValues, maskedFields));
      }
   }

   /**
    * Execute local command
    * 
    * @param node node to execute at
    * @param tool object tool
    * @param inputValues input values
    * @param command command to execute
    * @param viewPlacement view placement information
    */
   private static void executeLocalCommand(final ObjectContext node, final ObjectTool tool, Map<String, String> inputValues, final String command, ViewPlacement viewPlacement)
   {      
      I18n i18n = LocalizationHelper.getI18n(ObjectToolExecutor.class);
      if ((tool.getFlags() & ObjectTool.GENERATES_OUTPUT) == 0)
      {
         Job job = new Job(i18n.tr("Execute external command"), null, viewPlacement.getMessageAreaHolder()) {
            @Override
            protected void run(IProgressMonitor monitor) throws Exception
            {
               TcpPortForwarder tcpPortForwarder = null;
               try
               {
                  String commandLine = command;
                  if (node.isNode() && ((tool.getFlags() & ObjectTool.SETUP_TCP_TUNNEL) != 0))
                  {
                     tcpPortForwarder = new TcpPortForwarder(Registry.getSession(), node.object.getObjectId(), tool.getRemotePort(), 0);
                     tcpPortForwarder.setDisplay(getDisplay());
                     tcpPortForwarder.setMessageArea(viewPlacement.getMessageAreaHolder());
                     tcpPortForwarder.run();
                     commandLine = commandLine.replace("${local-address}", "127.0.0.1").replace("${local-port}", Integer.toString(tcpPortForwarder.getLocalPort()));
                  }

                  Process process;
                  if (SystemUtils.IS_OS_WINDOWS)
                  {
                     commandLine = "CMD.EXE /C START \"NetXMS\" " + ((tcpPortForwarder != null) ? "/WAIT " : "") + commandLine;
                     process = Runtime.getRuntime().exec(commandLine);
                  }
                  else
                  {
                     process = Runtime.getRuntime().exec(new String[] { "/bin/sh", "-c", commandLine });
                  }

                  if (tcpPortForwarder != null)
                     process.waitFor();
               }
               finally
               {
                  if (tcpPortForwarder != null)
                     tcpPortForwarder.close();
               }
            }

            @Override
            protected String getErrorMessage()
            {
               return i18n.tr("Cannot start external process");
            }
         };
         job.setUser(false);
         job.start();
      }
      else
      {
         viewPlacement.openView(new LocalCommandResults(node, tool, inputValues, null));
      }
   }

   /**
    * @param node
    * @param tool
    * @param inputValues 
    */
   private static void executeFileDownload(final ObjectContext node, final ObjectTool tool, final Map<String, String> inputValues, ViewPlacement viewPlacement)
   {
      final NXCSession session = Registry.getSession();
      String[] parameters = tool.getData().split("\u007F"); //$NON-NLS-1$

      final String fileName = parameters[0];
      final int maxFileSize = (parameters.length > 0) ? Integer.parseInt(parameters[1]) : 0;
      final boolean follow = (parameters.length > 1) ? parameters[2].equals("true") : false; //$NON-NLS-1$

      Job job = new Job("Download file from agent", null) {
         @Override
         protected String getErrorMessage()
         {
            return String.format("Cannot download file %s from node %s", fileName, node.object.getObjectName());
         }

         @Override
         protected void run(final IProgressMonitor monitor) throws Exception
         {
            final AgentFileData file = session.downloadFileFromAgent(node.object.getObjectId(), fileName, true, node.getAlarmId(), inputValues, maxFileSize, follow, new ProgressListener() {
               @Override
               public void setTotalWorkAmount(long workTotal)
               {
                  monitor.beginTask("Download file " + fileName, (int)workTotal);
               }

               @Override
               public void markProgress(long workDone)
               {
                  monitor.worked((int)workDone);
               }
            });
            runInUIThread(() -> AgentFileViewer.createView(viewPlacement, node.object.getObjectId(), file, maxFileSize, follow, node.contextId));
         }
      };
      job.start();
   }

   /**
    * @param node
    * @param tool
    */
   private static void executeInternalTool(ObjectContext node, ObjectTool tool, Map<String, String> inputValues, ViewPlacement viewPlacement)
   {
      I18n i18n = LocalizationHelper.getI18n(ObjectToolExecutor.class);
      ObjectToolHandler handler = ObjectToolsCache.findHandler(tool.getData());
      if (handler != null)
      {
         handler.execute((AbstractNode)node.object, tool, inputValues, viewPlacement);
      }
      else
      {
         MessageDialogHelper.openError(Registry.getMainWindowShell(), i18n.tr("Error"), i18n.tr("Cannot execute object tool: handler not defined"));
      }
   }

   /**
    * @param node
    * @param tool
    * @param inputValues 
    */
   private static void openURL(final ObjectContext node, final ObjectTool tool, final String url, ViewPlacement viewPlacement)
   {
      I18n i18n = LocalizationHelper.getI18n(ObjectToolExecutor.class);
      if (node.isNode() && ((tool.getFlags() & ObjectTool.SETUP_TCP_TUNNEL) != 0))
      {
         final NXCSession session = Registry.getSession();
         Job job = new Job(i18n.tr("Setup TCP port forwarding"), null, viewPlacement.getMessageAreaHolder()) {
            @Override
            protected void run(IProgressMonitor monitor) throws Exception
            {
               TcpPortForwarder tcpPortForwarder = new TcpPortForwarder(session, node.object.getObjectId(), tool.getRemotePort(), 600000); // Close underlying proxy after 10 minutes of inactivity
               tcpPortForwarder.setDisplay(getDisplay());
               tcpPortForwarder.setMessageArea(viewPlacement.getMessageAreaHolder());
               tcpPortForwarder.run();
               final String realUrl = url.replace("${local-address}", ExternalWebBrowser.getLocalAddress(getDisplay())).replace("${local-port}", Integer.toString(tcpPortForwarder.getLocalPort()));
               runInUIThread(() -> ExternalWebBrowser.open(realUrl));
            }

            @Override
            protected String getErrorMessage()
            {
               return i18n.tr("Cannot setup TCP port forwarding");
            }
         };
         job.setUser(false);
         job.start();
      }
      else
      {
         ExternalWebBrowser.open(url);
      }
   }
}
