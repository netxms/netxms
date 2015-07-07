/**
 * 
 */
package org.netxms.ui.eclipse.objecttools.api;

import java.io.IOException;
import java.net.URLEncoder;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.Platform;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.PlatformUI;
import org.netxms.client.AgentFile;
import org.netxms.client.NXCSession;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objecttools.Activator;
import org.netxms.ui.eclipse.objecttools.Messages;
import org.netxms.ui.eclipse.objecttools.views.AgentActionResults;
import org.netxms.ui.eclipse.objecttools.views.BrowserView;
import org.netxms.ui.eclipse.objecttools.views.FileViewer;
import org.netxms.ui.eclipse.objecttools.views.LocalCommandResults;
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
            message = message.replace("%OBJECT_IP_ADDR%", node.object.getPrimaryIP().getHostAddress()); //$NON-NLS-1$
            message = message.replace("%OBJECT_NAME%", node.object.getObjectName()); //$NON-NLS-1$
            message = message.replace("%OBJECT_ID%", Long.toString(node.object.getObjectId())); //$NON-NLS-1$
            message = message.replace("%USERNAME%", ConsoleSharedData.getSession().getUserName()); //$NON-NLS-1$
         }
         else
         {
            message = message.replace("%OBJECT_IP_ADDR%", Messages.get().ObjectToolsDynamicMenu_MultipleNodes); //$NON-NLS-1$
            message = message.replace("%OBJECT_NAME%", Messages.get().ObjectToolsDynamicMenu_MultipleNodes); //$NON-NLS-1$
            message = message.replace("%OBJECT_ID%", Messages.get().ObjectToolsDynamicMenu_MultipleNodes); //$NON-NLS-1$
            message = message.replace("%USERNAME%", ConsoleSharedData.getSession().getUserName()); //$NON-NLS-1$
         }
         if (!MessageDialogHelper.openQuestion(PlatformUI.getWorkbench().getActiveWorkbenchWindow().getShell(), 
               Messages.get().ObjectToolsDynamicMenu_ConfirmExec, message))
            return;
      }
      
      for(NodeInfo n : nodes)
         executeOnNode(n, tool);
   }
   
   /**
    * Execute object tool on single node
    * 
    * @param node
    * @param tool
    */
   private static void executeOnNode(final NodeInfo node, final ObjectTool tool)
   {
      switch(tool.getType())
      {
         case ObjectTool.TYPE_INTERNAL:
            executeInternalTool(node, tool);
            break;
         case ObjectTool.TYPE_LOCAL_COMMAND:
            executeLocalCommand(node, tool);
            break;
         case ObjectTool.TYPE_SERVER_COMMAND:
            executeServerCommand(node, tool);
            break;
         case ObjectTool.TYPE_ACTION:
            executeAgentAction(node, tool);
            break;
         case ObjectTool.TYPE_TABLE_AGENT:
         case ObjectTool.TYPE_TABLE_SNMP:
            executeTableTool(node, tool);
            break;
         case ObjectTool.TYPE_URL:
            openURL(node, tool);
            break;
         case ObjectTool.TYPE_FILE_DOWNLOAD:
            executeFileDownload(node, tool);
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
    */
   private static void executeAgentAction(final NodeInfo node, final ObjectTool tool)
   {
      final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
      final String action = substituteMacros(tool.getData(), node);
      
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
    */
   private static void executeServerCommand(final NodeInfo node, final ObjectTool tool)
   {
      final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
      new ConsoleJob(Messages.get().ObjectToolsDynamicMenu_ExecuteServerCmd, null, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.executeServerCommand(node.object.getObjectId(), tool.getData());
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
   
   /**
    * Execute local command
    * 
    * @param node
    * @param tool
    */
   private static void executeLocalCommand(final NodeInfo node, final ObjectTool tool)
   {
      String command = substituteMacros(tool.getData(), node);
      
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
    */
   private static void executeFileDownload(final NodeInfo node, final ObjectTool tool)
   {
      final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
      String[] parameters = tool.getData().split("\u007F"); //$NON-NLS-1$
      
      final String fileName = parameters[0];
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
    */
   private static void openURL(final NodeInfo node, final ObjectTool tool)
   {
      final String url = substituteMacros(tool.getData(), node);
      
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
    * @return
    */
   private static String substituteMacros(String s, NodeInfo node)
   {
      StringBuilder sb = new StringBuilder();
      
      char[] src = s.toCharArray();
      for(int i = 0; i < s.length(); i++)
      {
         if (src[i] == '%')
         {
            StringBuilder p = new StringBuilder();
            for(i++; src[i] != '%' && i < s.length(); i++)
               p.append(src[i]);
            if (p.length() == 0)    // %%
            {
               sb.append('%');
            }
            else
            {
               String name = p.toString();
               if (name.equals("OBJECT_IP_ADDR")) //$NON-NLS-1$
               {
                  sb.append(node.object.getPrimaryIP().getHostAddress());
               }
               else if (name.equals("OBJECT_NAME")) //$NON-NLS-1$
               {
                  sb.append(node.object.getObjectName());
               }
               else if (name.equals("OBJECT_ID")) //$NON-NLS-1$
               {
                  sb.append(node.object.getObjectId());
               }
               else if (name.equals("ALARM_ID")) //$NON-NLS-1$
               {
                  if (node.alarm != null)
                     sb.append(node.alarm.getId());
               }
               else if (name.equals("ALARM_MESSAGE")) //$NON-NLS-1$
               {
                  if (node.alarm != null)
                     sb.append(node.alarm.getMessage());
               }
               else if (name.equals("ALARM_SEVERITY")) //$NON-NLS-1$
               {
                  if (node.alarm != null)
                     sb.append(node.alarm.getCurrentSeverity());
               }
               else if (name.equals("ALARM_SEVERITY_TEXT")) //$NON-NLS-1$
               {
                  if (node.alarm != null)
                     sb.append(StatusDisplayInfo.getStatusText(node.alarm.getCurrentSeverity()));
               }
               else if (name.equals("ALARM_STATE")) //$NON-NLS-1$
               {
                  if (node.alarm != null)
                     sb.append(node.alarm.getState());
               }
               else if (name.equals("USERNAME")) //$NON-NLS-1$
               {
                  sb.append(ConsoleSharedData.getSession().getUserName());
               }
               else
               {
                  String custAttr = node.object.getCustomAttributes().get(name);
                  if (custAttr != null)
                     sb.append(custAttr);
               }
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
