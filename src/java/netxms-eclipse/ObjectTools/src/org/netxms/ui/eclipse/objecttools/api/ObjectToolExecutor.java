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
package org.netxms.ui.eclipse.objecttools.api;

import java.net.URLEncoder;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.UUID;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.Platform;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.PlatformUI;
import org.netxms.client.AgentFileData;
import org.netxms.client.InputField;
import org.netxms.client.NXCSession;
import org.netxms.client.ProgressListener;
import org.netxms.client.constants.InputFieldType;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.ui.eclipse.filemanager.views.AgentFileViewer;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectbrowser.dialogs.InputFieldEntryDialog;
import org.netxms.ui.eclipse.objects.ObjectContext;
import org.netxms.ui.eclipse.objecttools.Activator;
import org.netxms.ui.eclipse.objecttools.Messages;
import org.netxms.ui.eclipse.objecttools.TcpPortForwarder;
import org.netxms.ui.eclipse.objecttools.dialogs.ToolExecutionStatusDialog;
import org.netxms.ui.eclipse.objecttools.views.AgentActionResults;
import org.netxms.ui.eclipse.objecttools.views.LocalCommandResults;
import org.netxms.ui.eclipse.objecttools.views.MultiNodeCommandExecutor;
import org.netxms.ui.eclipse.objecttools.views.SSHCommandResults;
import org.netxms.ui.eclipse.objecttools.views.ServerCommandResults;
import org.netxms.ui.eclipse.objecttools.views.ServerScriptResults;
import org.netxms.ui.eclipse.objecttools.views.TableToolResults;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.ExternalWebBrowser;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;

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
    * @param allObjects objects to execution tool on
    * @param tool Object tool
    */
   public static void execute(final Set<ObjectContext> allObjects, final ObjectTool tool)
   {
      ObjectToolHandler handler = ObjectToolsCache.findHandler(tool.getData());
      if ((tool.getToolType() == ObjectTool.TYPE_INTERNAL) && (handler == null))
      {
         Activator.logInfo("Cannot find handler for internal object tool " + tool.getData());
         return;
      }

      // Filter allowed and applicable nodes for execution
      final Set<ObjectContext> objects = new HashSet<ObjectContext>();
      for(ObjectContext n : allObjects)
         if (((tool.getToolType() != ObjectTool.TYPE_INTERNAL) || (n.isNode() && handler.canExecuteOnNode((AbstractNode)n.object, tool))) && tool.isApplicableForObject(n.object))
            objects.add(n);

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
         inputValues = InputFieldEntryDialog.readInputFields(tool.getDisplayName(), fields);
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
      final NXCSession session = ConsoleSharedData.getSession();
      new ConsoleJob(Messages.get().ObjectToolExecutor_JobName, null, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {      
            List<String> expandedText = null;

            if ((tool.getFlags() & ObjectTool.ASK_CONFIRMATION) != 0)
            {
               String message = tool.getConfirmationText();
               if (objects.size() == 1)
               {
                  // Expand message and action for 1 node, otherwise expansion occurs after confirmation
                  List<String> textToExpand = new ArrayList<String>();
                  textToExpand.add(tool.getConfirmationText());
                  if ((tool.getToolType() == ObjectTool.TYPE_URL) || (tool.getToolType() == ObjectTool.TYPE_LOCAL_COMMAND))
                  {
                     textToExpand.add(tool.getData());
                  }
                  ObjectContext node = objects.iterator().next();
                  expandedText = session.substituteMacros(node, textToExpand, inputValues);
                  message = expandedText.remove(0);                  
               }
               else
               {
                  ObjectContext node = objects.iterator().next();
                  message = node.substituteMacrosForMultipleNodes(message, inputValues, getDisplay());
               }

               ConfirmationRunnable runnable = new ConfirmationRunnable(message);
               getDisplay().syncExec(runnable);
               if (!runnable.isConfirmed())
                  return;

               if ((tool.getToolType() == ObjectTool.TYPE_URL) || (tool.getToolType() == ObjectTool.TYPE_LOCAL_COMMAND))
               {
                  expandedText = session.substituteMacros(objects.toArray(new ObjectContext[objects.size()]), tool.getData(), inputValues);
               }
            }
            else
            {
               if ((tool.getToolType() == ObjectTool.TYPE_URL) || (tool.getToolType() == ObjectTool.TYPE_LOCAL_COMMAND))
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
                              MessageDialogHelper.openError(null, Messages.get().ObjectToolExecutor_ErrorTitle, 
                                    String.format(Messages.get().ObjectToolExecutor_ErrorText, fieldName));
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
                  tool.getToolType() == ObjectTool.TYPE_ACTION || tool.getToolType() == ObjectTool.TYPE_SERVER_SCRIPT) && ((tool.getFlags() & ObjectTool.GENERATES_OUTPUT) != 0))
            {
               final List<String> finalExpandedText = expandedText;
               getDisplay().syncExec(new Runnable() {
                  @Override
                  public void run()
                  {
                     executeOnMultipleNodes(objects, tool, inputValues, maskedFields, finalExpandedText);
                  }
               });
            }
            else
            {
               // Create execution status dialog if tools may report success or failure messages and executed on multiple nodes
               final ToolExecutionStatusDialog statusDialog;
               if ((objects.size() > 1) &&
                     (tool.getToolType() == ObjectTool.TYPE_SERVER_COMMAND || tool.getToolType() == ObjectTool.TYPE_ACTION || tool.getToolType() == ObjectTool.TYPE_SERVER_SCRIPT) &&
                     ((tool.getFlags() & ObjectTool.GENERATES_OUTPUT) == 0))
               {
                  ToolExecutionStatusDialog[] container = new ToolExecutionStatusDialog[1];
                  getDisplay().syncExec(new Runnable() {
                     @Override
                     public void run()
                     {
                        ToolExecutionStatusDialog dlg = new ToolExecutionStatusDialog(PlatformUI.getWorkbench().getActiveWorkbenchWindow().getShell(), objects);
                        container[0] = dlg;
                     }
                  });
                  statusDialog = container[0];
                  if (statusDialog != null)
                  {
                     getDisplay().asyncExec(new Runnable() {
                        @Override
                        public void run()
                        {
                           container[0].open();
                        }
                     });
                  }
               }
               else
               {
                  statusDialog = null;
               }

               for(final ObjectContext n : objects)
               {
                  if (tool.getToolType() == ObjectTool.TYPE_URL || tool.getToolType() == ObjectTool.TYPE_LOCAL_COMMAND)
                  {
                     final String data = expandedText.get(i++);
                     getDisplay().syncExec(new Runnable() {
                        @Override
                        public void run()
                        {
                           executeOnNode(n, tool, inputValues, maskedFields, data, statusDialog);
                        }
                     });
                  }
                  else
                  {
                     getDisplay().syncExec(new Runnable() {
                        @Override
                        public void run()
                        {
                           executeOnNode(n, tool, inputValues, maskedFields, null, statusDialog);
                        }
                     });                  
                  }
               }
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return Messages.get().ObjectToolExecutor_PasswordValidationFailed;
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
               confirmed = MessageDialogHelper.openQuestion(PlatformUI.getWorkbench().getActiveWorkbenchWindow().getShell(), 
                     Messages.get().ObjectToolsDynamicMenu_ConfirmExec, message);
            }
            
            boolean isConfirmed()
            {
               return confirmed;
            }
         }         
         
      }.start();
   }
   
   /**
    * Execute object tool on single node
    * 
    * @param node node to execute at
    * @param tool object tool
    * @param inputValues input values
    * @param maskedFields list of input fields to be masked
    * @param expandedToolData expanded tool data
    */
   private static void executeOnNode(final ObjectContext node, final ObjectTool tool, Map<String, String> inputValues,
         List<String> maskedFields, String expandedToolData, ToolExecutionStatusDialog statusDialog)
   {
      switch(tool.getToolType())
      {
         case ObjectTool.TYPE_ACTION:
            executeAgentAction(node, tool, inputValues, maskedFields, statusDialog);
            break;
         case ObjectTool.TYPE_FILE_DOWNLOAD:
            executeFileDownload(node, tool, inputValues);
            break;
         case ObjectTool.TYPE_INTERNAL:
            executeInternalTool(node, tool);
            break;
         case ObjectTool.TYPE_LOCAL_COMMAND:
            executeLocalCommand(node, tool, inputValues, expandedToolData);
            break;
         case ObjectTool.TYPE_SERVER_COMMAND:
            executeServerCommand(node, tool, inputValues, maskedFields, statusDialog);
            break;
         case ObjectTool.TYPE_SSH_COMMAND:
            executeSshCommand(node, tool, inputValues, maskedFields, statusDialog);
            break;
         case ObjectTool.TYPE_SERVER_SCRIPT:
            executeServerScript(node, tool, inputValues, maskedFields, statusDialog);
            break;
         case ObjectTool.TYPE_AGENT_LIST:
         case ObjectTool.TYPE_AGENT_TABLE:
         case ObjectTool.TYPE_SNMP_TABLE:
            executeTableTool(node, tool);
            break;
         case ObjectTool.TYPE_URL:
            openURL(node, tool, expandedToolData);
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
    */
   private static void executeOnMultipleNodes(Set<ObjectContext> nodes, ObjectTool tool, Map<String, String> inputValues, List<String> maskedFields, List<String> expandedToolData)
   {
      final IWorkbenchWindow window = PlatformUI.getWorkbench().getActiveWorkbenchWindow();
      try
      {
         MultiNodeCommandExecutor view = (MultiNodeCommandExecutor)window.getActivePage().showView(MultiNodeCommandExecutor.ID, UUID.randomUUID().toString(), IWorkbenchPage.VIEW_ACTIVATE);
         view.execute(tool, nodes, inputValues, maskedFields, expandedToolData);
      }
      catch(Exception e)
      {
         Activator.logError("Cannot open view " + MultiNodeCommandExecutor.ID, e);
         MessageDialogHelper.openError(window.getShell(), Messages.get().ObjectToolsDynamicMenu_Error, String.format(Messages.get().ObjectToolsDynamicMenu_ErrorOpeningView, e.getLocalizedMessage()));
      }
   }

   /**
    * Execute table tool
    * 
    * @param node
    * @param tool
    */
   private static void executeTableTool(final ObjectContext node, final ObjectTool tool)
   {
      final IWorkbenchWindow window = PlatformUI.getWorkbench().getActiveWorkbenchWindow();
      try
      {
         final IWorkbenchPage page = window.getActivePage();
         final TableToolResults view = (TableToolResults)page.showView(TableToolResults.ID,
               Long.toString(tool.getId()) + "&" + Long.toString(node.object.getObjectId()), IWorkbenchPage.VIEW_ACTIVATE); //$NON-NLS-1$
         view.refreshTable();
      }
      catch(PartInitException e)
      {
         MessageDialogHelper.openError(window.getShell(), Messages.get().ObjectToolsDynamicMenu_Error, String.format(Messages.get().ObjectToolsDynamicMenu_ErrorOpeningView, e.getLocalizedMessage()));
      }
   }
   
   /**
    * Execute agent action.
    *
    * @param node
    * @param tool
    * @param inputValues
    * @param maskedFields
    * @param statusDialog
    */
   private static void executeAgentAction(final ObjectContext node, final ObjectTool tool, final Map<String, String> inputValues, final List<String> maskedFields, final ToolExecutionStatusDialog statusDialog)
   {
      final NXCSession session = ConsoleSharedData.getSession();

      if ((tool.getFlags() & ObjectTool.GENERATES_OUTPUT) == 0)
      {      
         new ConsoleJob(String.format(Messages.get().ObjectToolsDynamicMenu_ExecuteOnNode, node.object.getObjectName()), null, Activator.PLUGIN_ID, null) {
            @Override
            protected String getErrorMessage()
            {
               return String.format(Messages.get().ObjectToolsDynamicMenu_CannotExecuteOnNode, node.object.getObjectName());
            }

            @Override
            protected void runInternal(IProgressMonitor monitor) throws Exception
            {
               try
               {
                  final String action = session.executeActionWithExpansion(node.object.getObjectId(), node.getAlarmId(), tool.getData(), inputValues, maskedFields);
                  if (statusDialog != null)
                  {
                     statusDialog.updateExecutionStatus(node.object.getObjectId(), null);
                  }
                  else if ((tool.getFlags() & ObjectTool.SUPPRESS_SUCCESS_MESSAGE) == 0)
                  {
                     runInUIThread(new Runnable() {
                        @Override
                        public void run()
                        {
                           MessageDialogHelper.openInformation(null, Messages.get().ObjectToolsDynamicMenu_ToolExecution,
                                 String.format(Messages.get().ObjectToolsDynamicMenu_ExecSuccess, action, node.object.getObjectName()));
                        }
                     });
                  }
               }
               catch(Exception e)
               {
                  if (statusDialog != null)
                  {
                     statusDialog.updateExecutionStatus(node.object.getObjectId(), e.getLocalizedMessage());
                  }
                  else
                  {
                     throw e;
                  }
               }
            }
         }.start();
      }
      else
      {
         final String secondaryId = Long.toString(node.object.getObjectId()) + "&" + Long.toString(tool.getId()); //$NON-NLS-1$
         final IWorkbenchWindow window = PlatformUI.getWorkbench().getActiveWorkbenchWindow();
         try
         {
            AgentActionResults view = (AgentActionResults)window.getActivePage().showView(AgentActionResults.ID, secondaryId, IWorkbenchPage.VIEW_ACTIVATE);
            view.executeAction(tool.getData(), node.getAlarmId(), inputValues, maskedFields);
         }
         catch(Exception e)
         {
            MessageDialogHelper.openError(window.getShell(), Messages.get().ObjectToolsDynamicMenu_Error, String.format(Messages.get().ObjectToolsDynamicMenu_ErrorOpeningView, e.getLocalizedMessage()));
         }
      }
   }

   /**
    * Execute server command
    * 
    * @param node
    * @param tool
    * @param inputValues
    * @param maskedFields
    * @param statusDialog
    */
   private static void executeServerCommand(final ObjectContext node, final ObjectTool tool, final Map<String, String> inputValues, final List<String> maskedFields, final ToolExecutionStatusDialog statusDialog)
   {
      final NXCSession session = ConsoleSharedData.getSession();
      if ((tool.getFlags() & ObjectTool.GENERATES_OUTPUT) == 0)
      {
         new ConsoleJob(Messages.get().ObjectToolsDynamicMenu_ExecuteServerCmd, null, Activator.PLUGIN_ID, null) {
            @Override
            protected void runInternal(IProgressMonitor monitor) throws Exception
            {
               try
               {
                  session.executeServerCommand(node.object.getObjectId(), node.getAlarmId(), tool.getData(), inputValues, maskedFields);
                  if (statusDialog != null)
                  {
                     statusDialog.updateExecutionStatus(node.object.getObjectId(), null);
                  }
                  else if ((tool.getFlags() & ObjectTool.SUPPRESS_SUCCESS_MESSAGE) == 0)
                  {
                     runInUIThread(new Runnable() {
                        @Override
                        public void run()
                        {
                           MessageDialogHelper.openInformation(null, Messages.get().ObjectToolsDynamicMenu_Information, Messages.get().ObjectToolsDynamicMenu_ServerCommandExecuted);
                        }
                     });
                  }
               }
               catch(Exception e)
               {
                  if (statusDialog != null)
                  {
                     statusDialog.updateExecutionStatus(node.object.getObjectId(), e.getLocalizedMessage());
                  }
                  else
                  {
                     throw e;
                  }
               }
            }
            
            @Override
            protected String getErrorMessage()
            {
               return Messages.get().ObjectToolsDynamicMenu_ServerCmdExecError;
            }
         }.start();
      }
      else
      {
         final String secondaryId = Long.toString(node.object.getObjectId()) + "&" + Long.toString(tool.getId()); //$NON-NLS-1$
         final IWorkbenchWindow window = PlatformUI.getWorkbench().getActiveWorkbenchWindow();
         try
         {
            ServerCommandResults view = (ServerCommandResults)window.getActivePage().showView(ServerCommandResults.ID, secondaryId, IWorkbenchPage.VIEW_ACTIVATE);
            view.executeCommand(tool.getData(), node.getAlarmId(), inputValues, maskedFields);
         }
         catch(Exception e)
         {
            MessageDialogHelper.openError(window.getShell(), Messages.get().ObjectToolsDynamicMenu_Error, String.format(Messages.get().ObjectToolsDynamicMenu_ErrorOpeningView, e.getLocalizedMessage()));
         }
      }
   }

   /**
    * Execute ssh command
    * 
    * @param node
    * @param tool
    */
   private static void executeSshCommand(final ObjectContext node, final ObjectTool tool, Map<String, String> inputValues, List<String> maskedFields, final ToolExecutionStatusDialog statusDialog)
   {
      final NXCSession session = ConsoleSharedData.getSession();

      if ((tool.getFlags() & ObjectTool.GENERATES_OUTPUT) == 0)
      {
         new ConsoleJob(String.format(Messages.get().ObjectToolsDynamicMenu_ExecuteOnNode, node.object.getObjectName()), null, Activator.PLUGIN_ID, null) {
            @Override
            protected void runInternal(IProgressMonitor monitor) throws Exception
            {
               try
               {
                  session.executeSshCommand(node.object.getObjectId(), node.getAlarmId(), tool.getData(), inputValues, maskedFields, false, null, null);
                  if (statusDialog != null)
                  {
                     statusDialog.updateExecutionStatus(node.object.getObjectId(), null);
                  }
                  else if ((tool.getFlags() & ObjectTool.SUPPRESS_SUCCESS_MESSAGE) == 0)
                  {
                     runInUIThread(new Runnable() {
                        @Override
                        public void run()
                        {
                           MessageDialogHelper.openInformation(null, Messages.get().ObjectToolsDynamicMenu_ToolExecution,
                                 String.format(Messages.get().ObjectToolsDynamicMenu_ExecSuccess, tool.getData(), node.object.getObjectName()));
                        }
                     });
                  }
               }
               catch(Exception e)
               {
                  if (statusDialog != null)
                  {
                     statusDialog.updateExecutionStatus(node.object.getObjectId(), e.getLocalizedMessage());
                  }
                  else
                  {
                     throw e;
                  }
               }
            }

            @Override
            protected String getErrorMessage()
            {
               return String.format(Messages.get().ObjectToolsDynamicMenu_CannotExecuteOnNode, node.object.getObjectName());
            }
         }.start();
      }
      else
      {
         final String secondaryId = Long.toString(node.object.getObjectId()) + "&" + Long.toString(tool.getId()); //$NON-NLS-1$
         final IWorkbenchWindow window = PlatformUI.getWorkbench().getActiveWorkbenchWindow();
         try
         {
            SSHCommandResults view = (SSHCommandResults)window.getActivePage().showView(SSHCommandResults.ID, secondaryId, IWorkbenchPage.VIEW_ACTIVATE);
            view.executeSshCommand(node.getAlarmId(), tool.getData(), inputValues, maskedFields);
         }
         catch(Exception e)
         {
            MessageDialogHelper.openError(window.getShell(), Messages.get().ObjectToolsDynamicMenu_Error, String.format(Messages.get().ObjectToolsDynamicMenu_ErrorOpeningView, e.getLocalizedMessage()));
         }
      }
   }

   /**
    * Execute server script
    * 
    * @param node
    * @param tool
    * @param inputValues 
    */
   private static void executeServerScript(final ObjectContext node, final ObjectTool tool, final Map<String, String> inputValues, List<String> maskedFields, final ToolExecutionStatusDialog statusDialog)
   {
      final NXCSession session = ConsoleSharedData.getSession();
      if ((tool.getFlags() & ObjectTool.GENERATES_OUTPUT) == 0)
      {
         new ConsoleJob("Execute server script", null, Activator.PLUGIN_ID, null) {
            @Override
            protected void runInternal(IProgressMonitor monitor) throws Exception
            {
               try
               {
                  session.executeLibraryScript(node.object.getObjectId(), node.getAlarmId(), tool.getData(), inputValues, maskedFields, null);
                  if (statusDialog != null)
                  {
                     statusDialog.updateExecutionStatus(node.object.getObjectId(), null);
                  }
                  else if ((tool.getFlags() & ObjectTool.SUPPRESS_SUCCESS_MESSAGE) == 0)
                  {
                     runInUIThread(new Runnable() {
                        @Override
                        public void run()
                        {
                           MessageDialogHelper.openInformation(null, Messages.get().ObjectToolsDynamicMenu_Information, Messages.get().ObjectToolsDynamicMenu_ServerScriptExecuted);
                        }
                     });
                  }
               }
               catch(Exception e)
               {
                  if (statusDialog != null)
                  {
                     statusDialog.updateExecutionStatus(node.object.getObjectId(), e.getLocalizedMessage());
                  }
                  else
                  {
                     throw e;
                  }
               }
            }

            @Override
            protected String getErrorMessage()
            {
               return Messages.get().ObjectToolsDynamicMenu_ServerScriptExecError;
            }
         }.start();
      }
      else
      {
         final String secondaryId = Long.toString(node.object.getObjectId()) + "&" + Long.toString(tool.getId()); //$NON-NLS-1$
         final IWorkbenchWindow window = PlatformUI.getWorkbench().getActiveWorkbenchWindow();
         try
         {
            ServerScriptResults view = (ServerScriptResults)window.getActivePage().showView(ServerScriptResults.ID, secondaryId, IWorkbenchPage.VIEW_ACTIVATE);
            view.executeScript(tool.getData(), node.getAlarmId(), inputValues, maskedFields);
         }
         catch(Exception e)
         {
            MessageDialogHelper.openError(window.getShell(), Messages.get().ObjectToolsDynamicMenu_Error, String.format(Messages.get().ObjectToolsDynamicMenu_ErrorOpeningView, e.getLocalizedMessage()));
         }
      }
   }

   /**
    * Execute local command
    * 
    * @param node node to execute at
    * @param tool object tool
    * @param inputValues input values
    * @param command command to execute
    */
   private static void executeLocalCommand(final ObjectContext node, final ObjectTool tool, Map<String, String> inputValues, final String command)
   {      
      if ((tool.getFlags() & ObjectTool.GENERATES_OUTPUT) == 0)
      {
         ConsoleJob job = new ConsoleJob("Execute external command", null, Activator.PLUGIN_ID) {
            @Override
            protected void runInternal(IProgressMonitor monitor) throws Exception
            {
               TcpPortForwarder tcpPortForwarder = null;
               try
               {
                  String commandLine = command;
                  if (node.isNode() && ((tool.getFlags() & ObjectTool.SETUP_TCP_TUNNEL) != 0))
                  {
                     tcpPortForwarder = new TcpPortForwarder(ConsoleSharedData.getSession(), node.object.getObjectId(), tool.getRemotePort(), 0);
                     tcpPortForwarder.setDisplay(getDisplay());
                     tcpPortForwarder.run();
                     commandLine = commandLine.replace("${local-address}", "127.0.0.1").replace("${local-port}", Integer.toString(tcpPortForwarder.getLocalPort()));
                  }

                  Process process;
                  if (Platform.getOS().equals(Platform.OS_WIN32))
                  {
                     commandLine = "CMD.EXE /C START \"NetXMS\" " + ((tcpPortForwarder != null) ? "/WAIT " : "") + commandLine; //$NON-NLS-1$ //$NON-NLS-2$
                     process = Runtime.getRuntime().exec(commandLine);
                  }
                  else
                  {
                     process = Runtime.getRuntime().exec(new String[] { "/bin/sh", "-c", commandLine }); //$NON-NLS-1$ //$NON-NLS-2$
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
               return "Cannot execute external process";
            }
         };
         job.setUser(false);
         job.start();
      }
      else
      {
         final String secondaryId = Long.toString(node.object.getObjectId()) + "&" + Long.toString(tool.getId()) + "&" + //$NON-NLS-1$ //$NON-NLS-2$
               ((node.isNode() && ((tool.getFlags() & ObjectTool.SETUP_TCP_TUNNEL) != 0) ? tool.getRemotePort() : 0));
         final IWorkbenchWindow window = PlatformUI.getWorkbench().getActiveWorkbenchWindow();
         try
         {
            LocalCommandResults view = (LocalCommandResults)window.getActivePage().showView(LocalCommandResults.ID, secondaryId, IWorkbenchPage.VIEW_ACTIVATE);
            view.runCommand(command);
         }
         catch(Exception e)
         {
            MessageDialogHelper.openError(window.getShell(), Messages.get().ObjectToolsDynamicMenu_Error, String.format(Messages.get().ObjectToolsDynamicMenu_ErrorOpeningView, e.getLocalizedMessage()));
         }
      }
   }

   /**
    * @param node
    * @param tool
    * @param inputValues 
    */
   private static void executeFileDownload(final ObjectContext node, final ObjectTool tool, final Map<String, String> inputValues)
   {
      final NXCSession session = ConsoleSharedData.getSession();
      String[] parameters = tool.getData().split("\u007F"); //$NON-NLS-1$

      final String fileName = parameters[0];
      final int maxFileSize = (parameters.length > 0) ? Integer.parseInt(parameters[1]) : 0;
      final boolean follow = (parameters.length > 1) ? parameters[2].equals("true") : false; //$NON-NLS-1$

      ConsoleJob job = new ConsoleJob(Messages.get().ObjectToolsDynamicMenu_DownloadFromAgent, null, Activator.PLUGIN_ID) {
         @Override
         protected String getErrorMessage()
         {
            return String.format(Messages.get().ObjectToolsDynamicMenu_DownloadError, fileName, node.object.getObjectName());
         }

         @Override
         protected void runInternal(final IProgressMonitor monitor) throws Exception
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
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  try
                  {
                     String secondaryId = Long.toString(node.object.getObjectId()) + "&" + URLEncoder.encode(fileName, "UTF-8"); //$NON-NLS-1$ //$NON-NLS-2$
                     AgentFileViewer.createView(secondaryId, node.object.getObjectId(), file, follow);
                  }
                  catch(Exception e)
                  {
                     final IWorkbenchWindow window = PlatformUI.getWorkbench().getActiveWorkbenchWindow();
                     MessageDialogHelper.openError(window.getShell(), Messages.get().ObjectToolsDynamicMenu_Error, String.format(Messages.get().ObjectToolsDynamicMenu_ErrorOpeningView, e.getLocalizedMessage()));
                     Activator.logError("Cannot open agent file viewer", e);
                  }
               }
            });
         }
      };
      job.start();
   }

   /**
    * @param node
    * @param tool
    */
   private static void executeInternalTool(final ObjectContext node, final ObjectTool tool)
   {
      ObjectToolHandler handler = ObjectToolsCache.findHandler(tool.getData());
      if (handler != null)
      {
         handler.execute((AbstractNode)node.object, tool);
      }
      else
      {
         MessageDialogHelper.openError(PlatformUI.getWorkbench().getActiveWorkbenchWindow().getShell(), Messages.get().ObjectToolsDynamicMenu_Error, Messages.get().ObjectToolsDynamicMenu_HandlerNotDefined);
      }
   }

   /**
    * @param node
    * @param tool
    * @param inputValues 
    */
   private static void openURL(final ObjectContext node, final ObjectTool tool, final String url)
   {
      if (node.isNode() && ((tool.getFlags() & ObjectTool.SETUP_TCP_TUNNEL) != 0))
      {
         final NXCSession session = ConsoleSharedData.getSession();
         ConsoleJob job = new ConsoleJob("Setup TCP port forwarding", null, Activator.PLUGIN_ID) {
            @Override
            protected void runInternal(IProgressMonitor monitor) throws Exception
            {
               TcpPortForwarder tcpPortForwarder = new TcpPortForwarder(session, node.object.getObjectId(), tool.getRemotePort(), 600000); // Close underlying proxy after 10 minutes of inactivity
               tcpPortForwarder.setDisplay(getDisplay());
               tcpPortForwarder.run();
               final String realUrl = url.replace("${local-address}", "127.0.0.1").replace("${local-port}", Integer.toString(tcpPortForwarder.getLocalPort()));
               runInUIThread(() -> {
                  ExternalWebBrowser.open(realUrl);
               });
            }

            @Override
            protected String getErrorMessage()
            {
               return "Cannot setup TCP port forwarding";
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
