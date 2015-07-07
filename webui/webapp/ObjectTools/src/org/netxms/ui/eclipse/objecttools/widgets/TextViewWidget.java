package org.netxms.ui.eclipse.objecttools.widgets;

import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.Arrays;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.jobs.Job;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.commands.ActionHandler;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.Text;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IViewPart;
import org.eclipse.ui.handlers.IHandlerService;
import org.netxms.client.AgentFile;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objecttools.Activator;
import org.netxms.ui.eclipse.objecttools.Messages;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.widgets.CompositeWithMessageBar;

public class TextViewWidget extends CompositeWithMessageBar
{
   public static final long MAX_FILE_SIZE = 8192000;  
   
   private Text textViewer;
   private Action actionClear;
   private Action actionScrollLock;
   private String remoteFileName;
   private long nodeId;
   private String fileID;
   private File currentFile;
   private long offset = 0;
   private boolean follow;
   private ConsoleJob monitorJob;
   private ConsoleJob tryToRestartMonitoring;
   private SessionListener listener;
   private IViewPart viewPart;
   private final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
   
   public TextViewWidget(Composite parent, int style, String remoteFileName, long nodeId, IViewPart viewPart)
   {      
      super(parent, style);
      
      this.remoteFileName = remoteFileName;
      this.nodeId = nodeId;
      this.viewPart = viewPart;
      
      textViewer = new Text(getContent(), SWT.H_SCROLL | SWT.V_SCROLL);
      textViewer.setEditable(false);
      textViewer.setData(RWT.CUSTOM_VARIANT, "monospace");
      
      createActions();
      contributeToActionBars();
      createPopupMenu();
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      final IHandlerService handlerService = (IHandlerService)viewPart.getSite().getService(IHandlerService.class);
      
      actionClear = new Action(Messages.get().FileViewer_ClearOutput, SharedIcons.CLEAR_LOG) {
         @Override
         public void run()
         {
            textViewer.setText(""); //$NON-NLS-1$
         }
      };
      actionClear.setActionDefinitionId("org.netxms.ui.eclipse.objecttools.commands.clear_output"); //$NON-NLS-1$
      handlerService.activateHandler(actionClear.getActionDefinitionId(), new ActionHandler(actionClear));

      actionScrollLock = new Action(Messages.get().FileViewer_ScrollLock, Action.AS_CHECK_BOX) { 
         @Override
         public void run()
         {
         }
      };
      actionScrollLock.setImageDescriptor(Activator.getImageDescriptor("icons/scroll_lock.gif")); //$NON-NLS-1$
      actionScrollLock.setChecked(false);
      actionScrollLock.setActionDefinitionId("org.netxms.ui.eclipse.objecttools.commands.scroll_lock"); //$NON-NLS-1$
      handlerService.activateHandler(actionScrollLock.getActionDefinitionId(), new ActionHandler(actionScrollLock));
   }
   
   /**
    * Contribute actions to action bar
    */
   private void contributeToActionBars()
   {
      IActionBars bars = viewPart.getViewSite().getActionBars();
      fillLocalPullDown(bars.getMenuManager());
      fillLocalToolBar(bars.getToolBarManager());
   }

   /**
    * Fill local pull-down menu
    * 
    * @param manager
    *           Menu manager for pull-down menu
    */
   private void fillLocalPullDown(IMenuManager manager)
   {
      manager.add(actionClear);
      manager.add(actionScrollLock);
   }

   /**
    * Fill local tool bar
    * 
    * @param manager
    *           Menu manager for local toolbar
    */
   private void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionClear);
      manager.add(actionScrollLock);
   }

   /**
    * Create pop-up menu
    */
   private void createPopupMenu()
   {
      // Create menu manager
      MenuManager menuMgr = new MenuManager();
      menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener(new IMenuListener() {
         public void menuAboutToShow(IMenuManager mgr)
         {
            fillContextMenu(mgr);
         }
      });

      // Create menu
      Menu menu = menuMgr.createContextMenu(textViewer);
      textViewer.setMenu(menu);
   }

   /**
    * Fill context menu
    * 
    * @param mgr Menu manager
    */
   private void fillContextMenu(final IMenuManager manager)
   {
      manager.add(actionClear);
      manager.add(actionScrollLock);
   }
   

   /**
    * @param file
    * @param maxFileSize 
    */
   public void showFile(File file, boolean follow, String id, int maxFileSize, boolean exceedSize)
   {
      currentFile = file;
      fileID = id;
      offset = maxFileSize;      
      setContent(loadFile(currentFile));
      
      this.follow = follow;
      if (follow)
      {
         monitorJob = new ConsoleJob(Messages.get().FileViewer_Download_File_Updates, null, Activator.PLUGIN_ID, null) {
            private boolean continueWork = true;
            
            @Override
            protected void canceling() 
            {
               continueWork = false;
            }
            
            @Override
            protected void runInternal(IProgressMonitor monitor) throws Exception
            {
               while(continueWork)
               {
                  final String s = session.waitForFileTail(fileID, 3000);
                  if (s != null)
                  {
                     runInUIThread(new Runnable() {                  
                        @Override
                        public void run()
                        {
                           if (!textViewer.isDisposed())
                           {
                              appendContent(s);                               
                              
                           }
                        }
                     });
                  }                    
               }
            }
   
            @Override
            protected String getErrorMessage()
            {
               return String.format(Messages.get().ObjectToolsDynamicMenu_DownloadError, remoteFileName, nodeId);
            }
         };
         monitorJob.setUser(false);
         monitorJob.setSystem(true);
         monitorJob.start();
         
         listener = new SessionListener() {
            
            @Override
            public void notificationHandler(SessionNotification n)
            {
               switch(n.getCode())
               {
                  case SessionNotification.FILE_MONITORING_FAILED:    
                     //Check that this is applicable on current file
                     if(nodeId == n.getSubCode())
                        onFileMonitoringFail();
                     break;
               }
            }
         };
         
         session.addListener(listener);
      }
      //Create notification that size exceeded
      if(exceedSize)
      {
         showMessage(CompositeWithMessageBar.INFORMATION, "File is too large. Content will be shown partly.");
      }
   }
   
   private void onFileMonitoringFail()
   {
      tryToRestartMonitoring = new ConsoleJob(Messages.get().FileViewer_RestartFollowingJob, null, Activator.PLUGIN_ID, null) {
         private boolean continueWork = true;
         
         @Override
         protected void canceling() 
         {
            continueWork = false;
         }
         
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            runInUIThread(new Runnable() {                  
               @Override
               public void run()
               {
                  if (!textViewer.isDisposed())
                  {
                     textViewer.setForeground(new Color(getDisplay(), 255, 0, 0));
                     textViewer.append(" \n\n" + //$NON-NLS-1$
                           "----------------------------------------------------------------------\n" + //$NON-NLS-1$
                           Messages.get().FileViewer_NotifyFollowConnectionLost +
                           "\n----------------------------------------------------------------------" + //$NON-NLS-1$
                           "\n");   //$NON-NLS-1$

                  }
               }
            });    
            
            //Try to reconnect in loop every 20 sec.            
            while(continueWork)
            {
               try 
               {
                  final AgentFile file = session.downloadFileFromAgent(nodeId, remoteFileName, offset, follow);
                  
                  //When successfully connected - display notification to client.
                  runInUIThread(new Runnable() {                  
                     @Override
                     public void run()
                     {
                        if (!textViewer.isDisposed())
                        {
                           textViewer.append(
                                 "-------------------------------------------------------------------------------\n" + //$NON-NLS-1$
                                 Messages.get().FileViewer_NotifyFollowConnectionEnabed +
                                 "\n-------------------------------------------------------------------------------" + //$NON-NLS-1$
                                 "\n \n");   //$NON-NLS-1$
                           textViewer.setForeground(null);
                           loadFile(file.getFile());

                        }
                     }
                  });         
                  
                  continueWork = false;
               }
               catch(Exception e)
               {                  
               }
               Thread.sleep(20000);  
            }      
            
         }

         @Override
         protected String getErrorMessage()
         {
            return String.format(Messages.get().ObjectToolsDynamicMenu_DownloadError, remoteFileName, nodeId);
         }
      };
      tryToRestartMonitoring.setUser(false);
      tryToRestartMonitoring.setSystem(true);
      tryToRestartMonitoring.start();      
   }
   
   /* (non-Javadoc)
    * @see org.eclipse.ui.part.WorkbenchPart#dispose()
    */
   @Override
   public void dispose()
   {
      if (follow)
      {
         monitorJob.cancel();
         if(tryToRestartMonitoring != null)
            tryToRestartMonitoring.cancel();
         if(tryToRestartMonitoring == null || tryToRestartMonitoring.getState() != Job.RUNNING)
         {
            final ConsoleJob job = new ConsoleJob(Messages.get().FileViewer_Stop_File_Monitoring, null, Activator.PLUGIN_ID, null) {
               @Override
               protected void runInternal(IProgressMonitor monitor) throws Exception
               {
                  session.cancelFileMonitoring(nodeId, fileID);
               }
               
               @Override
               protected String getErrorMessage()
               {
                  return Messages.get().FileViewer_Cannot_Stop_File_Monitoring;
               }
            };
            job.setUser(false);
            job.setSystem(true);
            job.start();
         }
      }
      session.removeListener(listener);
      super.dispose();
   }
   
   /**
    * @param file
    */
   private String loadFile(File file)
   {
      StringBuilder content = new StringBuilder();
      FileReader reader = null;
      char[] buffer = new char[32768];
      try
      {
         reader = new FileReader(file);
         int size = 0;
         while(size < MAX_FILE_SIZE)
         {
            int count = reader.read(buffer);
            if (count == -1)
               break;
            if (count == buffer.length)
            {
               content.append(buffer);
            }
            else
            {
               content.append(Arrays.copyOf(buffer, count));
            }
            size += count;
         }
      }
      catch(IOException e)
      {
         e.printStackTrace();
      }
      finally
      {
         if (reader != null)
         {
            try
            {
               reader.close();
            }
            catch(IOException e)
            {
            }
         }
      }
      return content.toString();
   }
   
   /**
    * @param s
    */
   private void setContent(String s)
   {
      textViewer.setText(removeEscapeSequences(s));
   }
   
   /**
    * @param s
    */
   private void appendContent(String s)
   {
      textViewer.append(removeEscapeSequences(s));
   }
   
   /**
    * Remove escape sequences from input string
    * 
    * @param s
    * @return
    */
   private static String removeEscapeSequences(String s)
   {
      StringBuilder sb = new StringBuilder();
      for(int i = 0; i < s.length(); i++)
      {
         char ch = s.charAt(i);
         if (ch == 27)
         {
            i++;
            ch = s.charAt(i);
            if (ch == '[')
            {
               for(; i < s.length(); i++)
               {
                  ch = s.charAt(i);
                  if (((ch >= 'A') && (ch <= 'Z')) || ((ch >= 'a') && (ch <= 'z')))
                     break;
               }
            }
            else if ((ch == '(') || (ch == ')'))
            {
               i++;
            }
         }
         else if ((ch >= 32) || (ch == '\r') || (ch == '\n') || (ch == '\t'))
         {
            sb.append(ch);
         }
      }
      return sb.toString();
   }
}
