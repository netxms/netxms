/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2015 Victor Kirhenshtein
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

import java.io.IOException;
import java.net.URLEncoder;
import java.util.Arrays;
import java.util.Comparator;
import java.util.HashMap;
import java.util.Map;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.Platform;
import org.eclipse.jface.window.Window;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.PlatformUI;
import org.netxms.base.NXCommon;
import org.netxms.client.AgentFile;
import org.netxms.client.NXCSession;
import org.netxms.client.objecttools.InputField;
import org.netxms.client.objecttools.InputFieldType;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objecttools.Activator;
import org.netxms.ui.eclipse.objecttools.Messages;
import org.netxms.ui.eclipse.objecttools.dialogs.ObjectToolInputDialog;
import org.netxms.ui.eclipse.objecttools.views.AgentActionResults;
import org.netxms.ui.eclipse.objecttools.views.BrowserView;
import org.netxms.ui.eclipse.objecttools.views.FileViewer;
import org.netxms.ui.eclipse.objecttools.views.LocalCommandResults;
import org.netxms.ui.eclipse.objecttools.views.ServerCommandResults;
import org.netxms.ui.eclipse.objecttools.views.TableToolResults;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
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
    * Check if tool is allowed for execution on each node from set
    * 
    * @param tool
    * @param nodes
    * @return
    */
   public static boolean isToolAllowed(ObjectTool tool, Set<NodeInfo> nodes)
   {
      if (tool.getType() != ObjectTool.TYPE_INTERNAL)
         return true;
      
      ObjectToolHandler handler = ObjectToolsCache.findHandler(tool.getData());
      if (handler != null)
      {
         for(NodeInfo n : nodes)
            if (!handler.canExecuteOnNode(n.object, tool))
               return false;
         return true;
      }
      else
      {
         return false;
      }
   }
   
   /**
    * Check if given tool is applicable for all nodes in set
    * 
    * @param tool
    * @param nodes
    * @return
    */
   public static boolean isToolApplicable(ObjectTool tool, Set<NodeInfo> nodes)
   {
      for(NodeInfo n : nodes)
         if (!tool.isApplicableForNode(n.object))
            return false;
      return true;
   }
   
   /**
    * Execute object tool on node set
    * 
    * @param tool Object tool
    */
   public static void execute(final Set<NodeInfo> nodes, final ObjectTool tool)
   {
      if ((tool.getFlags() & ObjectTool.ASK_CONFIRMATION) != 0)
      {
         String message = tool.getConfirmationText();
         if (nodes.size() == 1)
         {
            NodeInfo node = nodes.iterator().next();
            message = substituteMacros(message, node, new HashMap<String, String>(0));
         }
         else
         {
            message = substituteMacros(message, new NodeInfo(null, null), new HashMap<String, String>(0));
         }
         if (!MessageDialogHelper.openQuestion(PlatformUI.getWorkbench().getActiveWorkbenchWindow().getShell(), 
               Messages.get().ObjectToolsDynamicMenu_ConfirmExec, message))
            return;
      }
      
      final Map<String, String> inputValues;
      final InputField[] fields = tool.getInputFields();
      if (fields.length > 0)
      {
         Arrays.sort(fields, new Comparator<InputField>() {
            @Override
            public int compare(InputField f1, InputField f2)
            {
               return f1.getDisplayName().compareToIgnoreCase(f2.getDisplayName());
            }
         });
         inputValues = readInputFields(fields);
         if (inputValues == null)
            return;  // cancelled
      }
      else
      {
         inputValues = new HashMap<String, String>(0);
      }
      
      // Check if password validation needed
      boolean validationNeeded = false;
      for(int i = 0; i < fields.length; i++)
         if (fields[i].getOptions().validatePassword)
         {
            validationNeeded = true;
            break;
         }
      
      if (validationNeeded)
      {
         final NXCSession session = ConsoleSharedData.getSession();
         new ConsoleJob("Validate passwords", null, Activator.PLUGIN_ID, null) {
            @Override
            protected void runInternal(IProgressMonitor monitor) throws Exception
            {
               for(int i = 0; i < fields.length; i++)
               {
                  if ((fields[i].getType() == InputFieldType.PASSWORD) && (fields[i].getOptions().validatePassword))
                  {
                     boolean valid = session.validateUserPassword(inputValues.get(fields[i].getName()));
                     if (!valid)
                     {
                        final String fieldName = fields[i].getDisplayName();
                        getDisplay().syncExec(new Runnable() {
                           @Override
                           public void run()
                           {
                              MessageDialogHelper.openError(null, "Password Validation Failed", 
                                    String.format("Password entered in input field \"%s\" is not valid", fieldName));
                           }
                        });
                        return;
                     }
                  }
               }
               
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     for(NodeInfo n : nodes)
                        executeOnNode(n, tool, inputValues);
                  }
               });
            }
            
            @Override
            protected String getErrorMessage()
            {
               return "Password validation failed";
            }
         }.start();
      }
      else
      {
         for(NodeInfo n : nodes)
            executeOnNode(n, tool, inputValues);
      }
   }
   
   /**
    * Read input fields
    * 
    * @param fields
    * @return
    */
   private static Map<String, String> readInputFields(InputField[] fields)
   {
      ObjectToolInputDialog dlg = new ObjectToolInputDialog(PlatformUI.getWorkbench().getActiveWorkbenchWindow().getShell(), fields);
      if (dlg.open() != Window.OK)
         return null;
      return dlg.getValues();
   }

   /**
    * Execute object tool on single node
    * 
    * @param node
    * @param tool
    * @param inputValues 
    */
   private static void executeOnNode(final NodeInfo node, final ObjectTool tool, Map<String, String> inputValues)
   {
      switch(tool.getType())
      {
         case ObjectTool.TYPE_ACTION:
            executeAgentAction(node, tool, inputValues);
            break;
         case ObjectTool.TYPE_FILE_DOWNLOAD:
            executeFileDownload(node, tool, inputValues);
            break;
         case ObjectTool.TYPE_INTERNAL:
            executeInternalTool(node, tool);
            break;
         case ObjectTool.TYPE_LOCAL_COMMAND:
            executeLocalCommand(node, tool, inputValues);
            break;
         case ObjectTool.TYPE_SERVER_COMMAND:
            executeServerCommand(node, tool, inputValues);
            break;
         case ObjectTool.TYPE_TABLE_AGENT:
         case ObjectTool.TYPE_TABLE_SNMP:
            executeTableTool(node, tool);
            break;
         case ObjectTool.TYPE_URL:
            openURL(node, tool, inputValues);
            break;
      }
   }
   
   /**
    * Execute table tool
    * 
    * @param node
    * @param tool
    */
   private static void executeTableTool(final NodeInfo node, final ObjectTool tool)
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
    * @param node
    * @param tool
    * @param inputValues 
    */
   private static void executeAgentAction(final NodeInfo node, final ObjectTool tool, Map<String, String> inputValues)
   {
      final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
      final String action = substituteMacros(tool.getData(), node, inputValues);
      
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
               session.executeAction(node.object.getObjectId(), action);
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     MessageDialogHelper.openInformation(null, Messages.get().ObjectToolsDynamicMenu_ToolExecution, String.format(Messages.get().ObjectToolsDynamicMenu_ExecSuccess, action, node.object.getObjectName()));
                  }
               });
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
            view.executeAction(action);
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
    */
   private static void executeServerCommand(final NodeInfo node, final ObjectTool tool, final Map<String, String> inputValues)
   {
      final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
      if ((tool.getFlags() & ObjectTool.GENERATES_OUTPUT) == 0)
      {      
         new ConsoleJob(Messages.get().ObjectToolsDynamicMenu_ExecuteServerCmd, null, Activator.PLUGIN_ID, null) {
            @Override
            protected void runInternal(IProgressMonitor monitor) throws Exception
            {
               session.executeServerCommand(node.object.getObjectId(), tool.getData(), inputValues);
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     MessageDialogHelper.openInformation(null, Messages.get().ObjectToolsDynamicMenu_Information, Messages.get().ObjectToolsDynamicMenu_ServerCommandExecuted);
                  }
               });
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
            view.executeCommand(tool.getData(), inputValues);
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
    * @param node
    * @param tool
    * @param inputValues 
    */
   private static void executeLocalCommand(final NodeInfo node, final ObjectTool tool, Map<String, String> inputValues)
   {
      String command = substituteMacros(tool.getData(), node, inputValues);
      
      if ((tool.getFlags() & ObjectTool.GENERATES_OUTPUT) == 0)
      {
         final String os = Platform.getOS();
         
         try
         {
            if (os.equals(Platform.OS_WIN32))
            {
               command = "CMD.EXE /C START \"NetXMS\" " + command; //$NON-NLS-1$
               Runtime.getRuntime().exec(command);
            }
            else
            {
               Runtime.getRuntime().exec(new String[] { "/bin/sh", "-c", command }); //$NON-NLS-1$ //$NON-NLS-2$
            }
         }
         catch(IOException e)
         {
            // TODO Auto-generated catch block
            e.printStackTrace();
         }
      }
      else
      {
         final String secondaryId = Long.toString(node.object.getObjectId()) + "&" + Long.toString(tool.getId()); //$NON-NLS-1$
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
   private static void executeFileDownload(final NodeInfo node, final ObjectTool tool, Map<String, String> inputValues)
   {
      final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
      String[] parameters = tool.getData().split("\u007F"); //$NON-NLS-1$
      
      final String fileName = substituteMacros(parameters[0], node, inputValues);
      final int maxFileSize = Integer.parseInt(parameters[1]);
      final boolean follow = parameters[2].equals("true") ? true : false; //$NON-NLS-1$
      
      ConsoleJob job = new ConsoleJob(Messages.get().ObjectToolsDynamicMenu_DownloadFromAgent, null, Activator.PLUGIN_ID, null) {
         @Override
         protected String getErrorMessage()
         {
            return String.format(Messages.get().ObjectToolsDynamicMenu_DownloadError, fileName, node.object.getObjectName());
         }

         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            final AgentFile file = session.downloadFileFromAgent(node.object.getObjectId(), fileName, maxFileSize, follow);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  final IWorkbenchWindow window = PlatformUI.getWorkbench().getActiveWorkbenchWindow();
                  try
                  {
                     String secondaryId = Long.toString(node.object.getObjectId()) + "&" + URLEncoder.encode(fileName, "UTF-8"); //$NON-NLS-1$ //$NON-NLS-2$
                     FileViewer.createView(window, window.getShell(), file, follow, maxFileSize, secondaryId, node.object.getObjectId());
                  }
                  catch(Exception e)
                  {
                     MessageDialogHelper.openError(window.getShell(), Messages.get().ObjectToolsDynamicMenu_Error, String.format(Messages.get().ObjectToolsDynamicMenu_ErrorOpeningView, e.getLocalizedMessage()));
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
   private static void executeInternalTool(final NodeInfo node, final ObjectTool tool)
   {
      ObjectToolHandler handler = ObjectToolsCache.findHandler(tool.getData());
      if (handler != null)
      {
         handler.execute(node.object, tool);
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
   private static void openURL(final NodeInfo node, final ObjectTool tool, Map<String, String> inputValues)
   {
      final String url = substituteMacros(tool.getData(), node, inputValues);
      
      final String sid = Long.toString(node.object.getObjectId()) + "&" + Long.toString(tool.getId()); //$NON-NLS-1$
      final IWorkbenchWindow window = PlatformUI.getWorkbench().getActiveWorkbenchWindow();
      try
      {
         BrowserView view = (BrowserView)window.getActivePage().showView(BrowserView.ID, sid, IWorkbenchPage.VIEW_ACTIVATE);
         view.openUrl(url);
      }
      catch(PartInitException e)
      {
         MessageDialogHelper.openError(window.getShell(), Messages.get().ObjectToolsDynamicMenu_Error, Messages.get().ObjectToolsDynamicMenu_CannotOpenWebBrowser + e.getLocalizedMessage());
      }
   }
   
   /**
    * Substitute macros in string
    * 
    * @param s
    * @param node
    * @param inputValues 
    * @return
    */
   private static String substituteMacros(String s, NodeInfo node, Map<String, String> inputValues)
   {
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
                  sb.append((node.object != null) ? node.object.getPrimaryIP().getHostAddress() : Messages.get().ObjectToolsDynamicMenu_MultipleNodes);
                  break;
               case 'A':   // alarm message
                  if (node.alarm != null)
                     sb.append(node.alarm.getMessage());
                  break;
               case 'c':
                  if (node.alarm != null)
                     sb.append(node.alarm.getSourceEventCode());
                  break;
               case 'g':
                  sb.append((node.object != null) ? node.object.getGuid().toString() : Messages.get().ObjectToolsDynamicMenu_MultipleNodes);
                  break;
               case 'i':
                  sb.append((node.object != null) ? String.format("0x%08X", node.object.getObjectId()) : Messages.get().ObjectToolsDynamicMenu_MultipleNodes);
                  break;
               case 'I':
                  sb.append((node.object != null) ? Long.toString(node.object.getObjectId()) : Messages.get().ObjectToolsDynamicMenu_MultipleNodes);
                  break;
               case 'm':   // alarm message
                  if (node.alarm != null)
                     sb.append(node.alarm.getMessage());
                  break;
               case 'n':
                  sb.append((node.object != null) ? node.object.getObjectName() : Messages.get().ObjectToolsDynamicMenu_MultipleNodes);
                  break;
               case 'N':
                  if (node.alarm != null)
                     sb.append(ConsoleSharedData.getSession().getEventName(node.alarm.getSourceEventCode()));
                  break;
               case 's':
                  if (node.alarm != null)
                     sb.append(node.alarm.getCurrentSeverity());
                  break;
               case 'S':
                  if (node.alarm != null)
                     sb.append(StatusDisplayInfo.getStatusText(node.alarm.getCurrentSeverity()));
                  break;
               case 'U':
                  sb.append(ConsoleSharedData.getSession().getUserName());
                  break;
               case 'v':
                  sb.append(NXCommon.VERSION);
                  break;
               case 'y':   // alarm state
                  if (node.alarm != null)
                     sb.append(node.alarm.getState());
                  break;
               case 'Y':   // alarm ID
                  if (node.alarm != null)
                     sb.append(node.alarm.getId());
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
                  if ((node.object != null) && (attr.length() > 0))
                  {
                     String value = node.object.getCustomAttributes().get(attr.toString());
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
}
