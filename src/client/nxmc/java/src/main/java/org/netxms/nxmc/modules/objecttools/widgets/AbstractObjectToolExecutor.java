/**
 * NetXMS - open source network management system
 * Copyright (C) 2020-2022 Raden Soultions
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
package org.netxms.nxmc.modules.objecttools.widgets;

import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.action.ToolBarManager;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.FontData;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.ToolBar;
import org.netxms.client.NXCSession;
import org.netxms.client.TextOutputAdapter;
import org.netxms.client.TextOutputListener;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.TextConsole;
import org.netxms.nxmc.base.widgets.TextConsole.IOConsoleOutputStream;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.ObjectContext;
import org.netxms.nxmc.modules.objecttools.widgets.helpers.ExecutorStateChangeListener;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * Base class for object tool execution widget
 */
public abstract class AbstractObjectToolExecutor extends Composite
{   
   private I18n i18n = LocalizationHelper.getI18n(AbstractObjectToolExecutor.class);
   private static final Logger logger = LoggerFactory.getLogger(AbstractObjectToolExecutor.class);
   
   protected final ObjectContext objectContext;
   protected final CommonContext objectToolInfo;
   protected TextConsole console;
   protected ToolBarManager toolBarManager;
   protected ToolBar toolBar;
   protected long streamId = 0;
   protected NXCSession session;

   private TextOutputListener outputListener;
   private Font headerFont;
   private final ActionSet actions;
   private Set<ExecutorStateChangeListener> stateChangeListeners = new HashSet<ExecutorStateChangeListener>();
   private String failureReason = null;
   private boolean running = false;
   private boolean restartEnabled = false;
   private boolean terminateEnabled = false;
   private boolean autoScroll = true;
   private ExecutorJob job;

   /**
    * Constructor for object tool executor. Executor control will be created hidden.
    * 
    * @param parent
    * @param style
    * @param options
    */
   public AbstractObjectToolExecutor(Composite parent, final ObjectContext objectContext, final ActionSet actions, final CommonContext objectToolInfo)
   {
      super(parent, SWT.NONE);
      this.objectContext = objectContext;
      this.actions = actions;
      this.session = Registry.getSession();
      this.objectToolInfo = objectToolInfo;

      setVisible(false);

      FontData fd = getFont().getFontData()[0];
      fd.setStyle(fd.getStyle() | SWT.BOLD);
      headerFont = new Font(getDisplay(), fd);
      addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            headerFont.dispose();
         }
      });

      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.INNER_SPACING;
      layout.horizontalSpacing = WidgetHelper.INNER_SPACING;
      layout.marginTop = 0;
      layout.marginBottom = 0;
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.numColumns = 3;
      setLayout(layout);
      
      Label objectName = new Label(this, SWT.LEFT);
      objectName.setFont(headerFont);
      objectName.setText(objectContext.object.getObjectName());
      GridData gd = new GridData();
      gd.verticalAlignment = SWT.CENTER;
      gd.horizontalAlignment = SWT.CENTER;
      gd.horizontalIndent = WidgetHelper.DIALOG_SPACING;
      objectName.setLayoutData(gd);

      Label separator = new Label(this, SWT.SEPARATOR | SWT.VERTICAL);
      gd = new GridData();
      gd.verticalAlignment = SWT.FILL;
      gd.horizontalAlignment = SWT.CENTER;
      gd.horizontalIndent = WidgetHelper.DIALOG_SPACING;
      gd.heightHint = 5;
      separator.setLayoutData(gd);

      toolBar = new ToolBar(this, SWT.FLAT | SWT.HORIZONTAL);
      gd = new GridData();
      gd.verticalAlignment = SWT.CENTER;
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalIndent = WidgetHelper.DIALOG_SPACING;
      toolBar.setLayoutData(gd);
      toolBarManager = new ToolBarManager(toolBar);

      console = new TextConsole(this, SWT.NONE);
      console.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            actions.actionCopy.setEnabled(console.canCopy());
         }
      });
      console.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true, 3, 1));

      fillToolBar();
      createPopupMenu();

      toolBarManager.update(true);

      addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            if (job != null)
               job.removeWidget(AbstractObjectToolExecutor.this);
         }
      });

      outputListener = new TextOutputAdapter() {
         @Override
         public void messageReceived(String text)
         {
            job.writeOutput(text);
         }

         @Override
         public void setStreamId(long streamId)
         {
            AbstractObjectToolExecutor.this.streamId = streamId;
         }
      };
      setRunning(false);
   }

   /**
    * Write text to outputs
    * @param text
    */
   protected void writeOutput(String text)
   {
      job.writeOutput(text);
   }

   /**
    * Get output listener.
    *
    * @return output listener
    */
   protected TextOutputListener getOutputListener()
   {
      return outputListener;
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
      Menu menu = menuMgr.createContextMenu(console.getConsoleControl());
      console.getConsoleControl().setMenu(menu);
   }

   /**
    * Fill context menu
    * 
    * @param mgr Menu manager
    */
   private void fillContextMenu(final IMenuManager manager)
   {
      if (isTerminateSupported())
         manager.add(actions.actionTerminate);
      manager.add(actions.actionRestart);
      manager.add(new Separator());
      manager.add(actions.actionClear);
      manager.add(actions.actionScrollLock);
      manager.add(new Separator());
      manager.add(actions.actionSelectAll);
      manager.add(actions.actionCopy);
   }

   /**
    * Create toolbar items
    */
   private void fillToolBar()
   {
      if (isTerminateSupported())
         toolBarManager.add(actions.actionTerminate);
      toolBarManager.add(actions.actionRestart);
      toolBarManager.add(new Separator());
      toolBarManager.add(actions.actionClear);
      toolBarManager.add(actions.actionScrollLock);
   }

   /**
    * Enable/disable "restart" action
    * 
    * @param enabled true to enable action
    */
   protected void enableRestart(boolean enabled)
   {
      restartEnabled = enabled;
      actions.actionRestart.setEnabled(enabled);
   }

   /**
    * Enable/disable "terminate" action
    * 
    * @param enabled true to enable action
    */
   protected void enableTerminate(boolean enabled)
   {
      terminateEnabled = enabled;
      actions.actionTerminate.setEnabled(enabled);
   }

   /**
    * Execute command
    * 
    * @param result 
    */
   public final void execute()
   {
      if (isRunning())
      {
         MessageDialog.openError(Display.getCurrent().getActiveShell(), "Error", "Command already running!");
         return;
      }

      failureReason = null;
      setRunning(true);
      job = new ExecutorJob(String.format(i18n.tr("Execute action on node %s"), objectContext.object.getObjectName()), this, console.newOutputStream());
      job.setUser(false);
      job.setSystem(true);
      job.start();
   }
   
   /**
    * Re execute tool 
    */
   public final void reExecute() 
   {     
      if ((objectToolInfo.tool.getFlags() & ObjectTool.ASK_CONFIRMATION) != 0)
      {
         new Job(String.format(i18n.tr("Expand confirmation message for node %s"), objectContext.object.getObjectName()), null) {
            
            @Override
            protected void run(IProgressMonitor monitor) throws Exception
            {
               List<String> textToExpand = new ArrayList<String>();
               textToExpand.add(objectToolInfo.tool.getConfirmationText());
               final List<String> expandedText = session.substituteMacros(objectContext, textToExpand, objectToolInfo.inputValues);
               runInUIThread(new Runnable() {                  
                  @Override
                  public void run()
                  {
                     if (MessageDialogHelper.openQuestion(Registry.getMainWindowShell(), i18n.tr("Confirm Tool Execution"), expandedText.get(0)))
                     {
                        AbstractObjectToolExecutor.this.execute();
                     }                     
                  }
               });
            }
            
            @Override
            protected String getErrorMessage()
            {
               return String.format(i18n.tr("Failed to expand confirmation message for node %s"), objectContext.object.getObjectName());
            }
         }.start();
      }
      else
      {
         execute();
      }      
   }

   /**
    * Terminate running command. Default implementation does nothing.
    */
   public void terminate()
   {
   }

   /**
    * Do actual tool execution (called by execute() inside background job).
    *
    * @param display current display
    * @throws Exception on any error
    */
   protected abstract void executeInternal(Display display) throws Exception;

   /**
    * Check if "terminate" action is supported. Default implementation returns false.
    * 
    * @return true if "terminate" action is supported
    */
   protected boolean isTerminateSupported()
   {
      return false;
   }

   /**
    * Get running state.
    * 
    * @return current running state
    */
   public boolean isRunning()
   {
      return running;
   }

   /**
    * Set running state.
    * 
    * @param running new running state
    */
   protected void setRunning(boolean running)
   {
      this.running = running;
      enableRestart(!running);
      enableTerminate(isTerminateSupported() && running);
      for(ExecutorStateChangeListener l : stateChangeListeners)
         l.runningStateChanged(running);
   }

   /**
    * Check if tool execution failed.
    *
    * @return true if tool execution failed
    */
   public boolean isFailed()
   {
      return failureReason != null;
   }

   /**
    * Get failure reason.
    *
    * @return failure reason
    */
   public String getFailureReason()
   {
      return failureReason;
   }

   /**
    * Select all text in output console
    */
   public void selectAll()
   {
      console.selectAll();
   }

   /**
    * Copy selected output text to clipboard
    */
   public void copyOutput()
   {
      console.copy();
   }

   /**
    * Clear output
    */
   public void clearOutput()
   {
      console.clear();
   }

   /**
    * Enable/disable automatic scrolling.
    *
    * @param enabled true to enable
    */
   public void setAutoScroll(boolean enabled)
   {
      autoScroll = enabled;
      console.setAutoScroll(enabled);
   }

   /**
    * Hide this executor
    */
   public void hide()
   {
      setVisible(false);
   }

   /**
    * Show this executor and place it on top of Z-order
    */
   public void show()
   {
      actions.actionCopy.setEnabled(console.canCopy());
      actions.actionRestart.setEnabled(restartEnabled);
      actions.actionTerminate.setEnabled(terminateEnabled);
      actions.actionScrollLock.setChecked(!autoScroll);
      setVisible(true);
      moveAbove(null);
   }

   /**
    * Get associated object
    *
    * @return
    */
   public AbstractObject getObject()
   {
      return objectContext.object;
   }

   /**
    * Add state change listener.
    * 
    * @param listener state change listener
    */
   public void addStateChangeListener(ExecutorStateChangeListener listener)
   {
      stateChangeListeners.add(listener);
   }

   /**
    * Remove state change listener.
    * 
    * @param listener state change listener
    */
   public void removeStateChangeListener(ExecutorStateChangeListener listener)
   {
      stateChangeListeners.remove(listener);
   }

   /**
    * Context 
    * 
    * @return context
    */
   public ObjectContext getContext()
   {
      return objectContext;
   }

   /**
    * Actions to be performed on view clone
    * 
    * @param origin
    */
   public void onClone(AbstractObjectToolExecutor origin)
   {
      console.setText(origin.console.getText());
      setRunning(origin.isRunning());
      if (origin.isRunning())
      {
         streamId = origin.streamId;
         job = origin.job;
         job.registerWidget(this, console.newOutputStream());
      }
   }

   /**
    * Object tool executor job
    */
   private final class ExecutorJob extends Job
   {
      private HashMap<AbstractObjectToolExecutor, IOConsoleOutputStream> executorWidgetList = new HashMap<>();
      
      /**
       * Job constructor
       * 
       * @param name
       * @param executorWidget
       */
      public ExecutorJob(String name, AbstractObjectToolExecutor executorWidget, IOConsoleOutputStream out)
      {
         super(name, null);
         synchronized(executorWidgetList)
         {
            executorWidgetList.put(executorWidget, out);
         }
      }
      
      /**
       * Write output to all views
       * @param text
       */
      public void writeOutput(String text)
      {
         try
         {
            synchronized(executorWidgetList)
            {
               for (IOConsoleOutputStream o : executorWidgetList.values())
               {
                  o.write(text);
               }
            }
         }
         catch(IOException e)
         {
         }
      }

      /**
       * @param executorWidget
       */
      public void registerWidget(AbstractObjectToolExecutor executorWidget, IOConsoleOutputStream out)
      {
         synchronized(executorWidgetList)
         {
            executorWidgetList.put(executorWidget, out);        
         }
      }
      
      /**
       * @param executorWidget
       */
      public void removeWidget(AbstractObjectToolExecutor executorWidget)
      {
         synchronized(executorWidgetList)
         {
            IOConsoleOutputStream out = executorWidgetList.remove(executorWidget);     
            try
            {
               out.close();
            }
            catch(IOException e)
            {
            }
            if (executorWidgetList.isEmpty() && isRunning())
               executorWidget.terminate();
         }
      }

      /**
       * @see org.netxms.nxmc.base.jobs.Job#getErrorMessage()
       */
      @Override
      protected String getErrorMessage()
      {
         return String.format(i18n.tr("Cannot execute action on node %s"), objectContext.object.getObjectName());
      }

      /**
       * @see org.netxms.nxmc.base.jobs.Job#run(org.eclipse.core.runtime.IProgressMonitor)
       */
      @Override
      protected void run(IProgressMonitor monitor) throws Exception
      {         
         try
         {
            executeInternal(getDisplay());
         }
         catch(Exception e)
         {
            logger.error("Error executing object tool", e);
            failureReason = e.getLocalizedMessage();
            if (failureReason.isEmpty())
               failureReason = "Internal error - " + e.getClass().getName();
         }
         
         try
         {
            synchronized(executorWidgetList)
            {
               for (IOConsoleOutputStream o : executorWidgetList.values())
               {
                  o.close();
               }
            }
         }
         catch(IOException ex)
         {
         }
      }

      /**
       * @see org.netxms.nxmc.base.jobs.Job#jobFinalize()
       */
      @Override
      protected void jobFinalize()
      {
         runInUIThread(new Runnable() {
            @Override
            public void run()
            {
               synchronized(executorWidgetList)
               {
                  for (AbstractObjectToolExecutor e : executorWidgetList.keySet())
                     e.setRunning(false);
               }
            }
         });
      }
   }

   /**
    * Action set for executor
    */
   public static class ActionSet
   {
      public Action actionClear;
      public Action actionScrollLock;
      public Action actionCopy;
      public Action actionSelectAll;
      public Action actionRestart;
      public Action actionTerminate;
   }
   
   /**
    * Common context fields
    */
   public static class CommonContext
   {
      public final ObjectTool tool;
      public Map<String, String> inputValues;
      public final List<String> maskedFields;
      
      public CommonContext(final ObjectTool tool, final Map<String, String> inputValues, final List<String> maskedFields)
      {
         this.tool = tool;
         this.inputValues = inputValues;
         this.maskedFields = maskedFields;
      }

      public CommonContext(CommonContext objectToolInfo)
      {
         this.tool = objectToolInfo.tool;
         this.inputValues = objectToolInfo.inputValues;
         this.maskedFields = objectToolInfo.maskedFields;
      }
   }
}
