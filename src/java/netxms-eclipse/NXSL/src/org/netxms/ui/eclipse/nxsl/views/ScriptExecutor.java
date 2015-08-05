/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.nxsl.views;

import java.io.IOException;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.commands.ActionHandler;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.SashForm;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.ISaveablePart2;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.console.IOConsoleOutputStream;
import org.eclipse.ui.contexts.IContextService;
import org.eclipse.ui.forms.widgets.Form;
import org.eclipse.ui.forms.widgets.FormToolkit;
import org.eclipse.ui.forms.widgets.Section;
import org.eclipse.ui.handlers.IHandlerService;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.Script;
import org.netxms.client.TextOutputListener;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.nxsl.Activator;
import org.netxms.ui.eclipse.nxsl.Messages;
import org.netxms.ui.eclipse.nxsl.dialogs.CreateScriptDialog;
import org.netxms.ui.eclipse.nxsl.dialogs.SaveScriptDialog;
import org.netxms.ui.eclipse.nxsl.widgets.ScriptEditor;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.TextConsole;

/**
 * Sored on server agent's configuration editor
 */
public class ScriptExecutor extends ViewPart implements ISaveablePart2, TextOutputListener
{
   public static final String ID = "org.netxms.ui.eclipse.nxsl.views.ScriptExecutor"; //$NON-NLS-1$

   private NXCSession session;
   private boolean modified = false;
   private long objectId;

   private Form form;
   private Combo scriptCombo;
   private ScriptEditor scriptEditor;
   private TextConsole output;
   private IOConsoleOutputStream consoleOutputStream;
   private Action actionSave;
   private Action actionSaveAs;
   private Action actionClear;
   private Action actionClearOutput;
   private Action actionReload;
   private Action actionExecute;
   private List<Script> library;
   private int previousSelection = -1;

   private boolean allowSaveAs = false;
   
   /* (non-Javadoc)
    * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
    */
   @Override
   public void init(IViewSite site) throws PartInitException
   {
      super.init(site);

      session = ConsoleSharedData.getSession();
      objectId = Long.parseLong(site.getSecondaryId());

      setPartName(String.format(Messages.get().ScriptExecutor_PartName, session.getObjectName(objectId)));
   }

   /* (non-Javadoc)
    * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createPartControl(Composite parent)
   {
      parent.setLayout(new FillLayout());

      /**** Script executor form ****/
      FormToolkit toolkit = new FormToolkit(getSite().getShell().getDisplay());

      Composite formContainer = new Composite(parent, SWT.NONE);
      formContainer.setLayout(new FillLayout());      
      
      form = toolkit.createForm(formContainer);
      form.setText(Messages.get().ScriptExecutor_Noname);
      
      GridLayout layout = new GridLayout();
      layout.verticalSpacing = 8;
      form.getBody().setLayout(layout);

      /**** Script list dropdown ****/
      scriptCombo = WidgetHelper.createLabeledCombo(form.getBody(), SWT.READ_ONLY, Messages.get().ScriptExecutor_LibScript, WidgetHelper.DEFAULT_LAYOUT_DATA, toolkit);
      updateScriptList(null); 
      scriptCombo.addSelectionListener( new SelectionListener() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            if (modified)
            {
               if (saveIfRequired(true))
                  return;
            }           
            getScriptContent();        
            previousSelection = scriptCombo.getSelectionIndex();
         }
         
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }
      });
      
      SashForm splitter = new SashForm(form.getBody(), SWT.VERTICAL);
      splitter.setSashWidth(3); 
      GridData gridData = new GridData();
      gridData.horizontalAlignment = GridData.FILL;
      gridData.grabExcessHorizontalSpace = true;
      gridData.verticalAlignment = SWT.FILL;
      gridData.grabExcessVerticalSpace = true;
      splitter.setLayoutData(gridData);
      
      /**** Script editor  ****/
      Composite container = toolkit.createComposite(splitter);
      layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.marginBottom = 4;
      container.setLayout(layout);
      Section section = toolkit.createSection(container, Section.TITLE_BAR);
      section.setText(Messages.get().ScriptExecutor_Source);
      gridData = new GridData();
      gridData.horizontalAlignment = GridData.FILL;
      gridData.grabExcessHorizontalSpace = true;
      gridData.verticalAlignment = SWT.FILL;
      gridData.grabExcessVerticalSpace = true;
      section.setLayoutData(gridData);
      
      scriptEditor = new ScriptEditor(section, SWT.BORDER, SWT.H_SCROLL | SWT.V_SCROLL, true);
      section.setClient(scriptEditor);
      scriptEditor.setText(""); //$NON-NLS-1$
      scriptEditor.getTextWidget().addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            onTextModify();
         }
      });
      
      /**** Execution result ****/
      container = toolkit.createComposite(splitter);
      layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.marginTop = 4;
      container.setLayout(layout);
      section = toolkit.createSection(container, Section.TITLE_BAR);
      section.setText(Messages.get().ScriptExecutor_Output);
      gridData = new GridData();
      gridData.horizontalAlignment = GridData.FILL;
      gridData.grabExcessHorizontalSpace = true;
      gridData.verticalAlignment = SWT.FILL;
      gridData.grabExcessVerticalSpace = true;
      section.setLayoutData(gridData);
      
      output = new TextConsole(section, SWT.BORDER);
      section.setClient(output);
      /*executionResult.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            actionCopy.setEnabled(executionResult.getSelectionCount() > 0);
         }
      });*/ // TODO: Think how to split copy action between 2 editors
      
      activateContext();
      createActions();
      contributeToActionBars();
      
      actionSave.setEnabled(false);
   }

   /**
    * Activate context
    */
   private void activateContext()
   {
      IContextService contextService = (IContextService)getSite().getService(IContextService.class);
      if (contextService != null)
      {
         contextService.activateContext("org.netxms.ui.eclipse.nxsl.context.ScriptExecutor"); //$NON-NLS-1$
      }
   }

   /**
    * On text modify
    */
   private void onTextModify()
   {
      if (!modified)
      {
         modified = true;
         firePropertyChange(PROP_DIRTY);
         if((scriptCombo.getSelectionIndex() != -1))
            actionSave.setEnabled(true);
      }
   }

   /* (non-Javadoc)
    * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
    */
   @Override
   public void setFocus()
   {
      scriptEditor.setFocus();
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      final IHandlerService handlerService = (IHandlerService)getSite().getService(IHandlerService.class);
      
      actionSave = new Action(Messages.get().ScriptExecutor_Save, SharedIcons.SAVE) {
         @Override
         public void run()
         {
            intermediateSave(false);
         }
      };
      actionSave.setActionDefinitionId("org.netxms.ui.eclipse.nxsl.commands.save"); //$NON-NLS-1$
      handlerService.activateHandler(actionSave.getActionDefinitionId(), new ActionHandler(actionSave));
      
      actionSaveAs = new Action(Messages.get().ScriptExecutor_SaveAs, SharedIcons.SAVE_AS) {
         @Override
         public void run()
         {
            createNewScript(false); 
         }
      };
      actionSaveAs.setActionDefinitionId("org.netxms.ui.eclipse.nxsl.commands.save_as"); //$NON-NLS-1$
      handlerService.activateHandler(actionSaveAs.getActionDefinitionId(), new ActionHandler(actionSaveAs));
      
      actionClear = new Action(Messages.get().ScriptExecutor_Clear, SharedIcons.CLEAR) {
         @Override
         public void run()
         {
            if(modified)
            {
               if(saveIfRequired(false))
                  return;
            }
            scriptCombo.deselectAll();
            scriptCombo.clearSelection();
            scriptEditor.setText(""); //$NON-NLS-1$
            output.clear();
            form.setText(Messages.get().ScriptExecutor_Noname);
         }
      };

      actionClearOutput = new Action(Messages.get().ScriptExecutor_ClearOutput, SharedIcons.CLEAR_LOG) {
         @Override
         public void run()
         {
            output.clear();
         }
      };
      actionClearOutput.setActionDefinitionId("org.netxms.ui.eclipse.nxsl.commands.clear_output"); //$NON-NLS-1$
      handlerService.activateHandler(actionClearOutput.getActionDefinitionId(), new ActionHandler(actionClearOutput));
      
      actionReload = new RefreshAction(this) {
         @Override
         public void run()
         {
            if (modified)
            {
               if(saveIfRequired(false))
                  return;
            }
            
            updateScriptList(null);
            getScriptContent();
            output.clear();
         }
      };
      actionReload.setText(Messages.get().ScriptExecutor_Reload);
      
      actionExecute = new Action(Messages.get().ScriptExecutor_Execute, SharedIcons.EXECUTE) {
         @Override
         public void run()
         {
            executeScript();
         }
      };
      actionExecute.setActionDefinitionId("org.netxms.ui.eclipse.nxsl.commands.execute_script"); //$NON-NLS-1$
      handlerService.activateHandler(actionExecute.getActionDefinitionId(), new ActionHandler(actionExecute));
   }  
   
   /**
    * Ask if save, save as, cancel or discard action should be done
    */
   private boolean saveIfRequired(boolean onSelectionChange)
   {
      SaveScriptDialog dlg = new SaveScriptDialog(getSite().getShell(), actionSave.isEnabled());
      int rc = dlg.open();
      
      switch(rc)
      {
         case SaveScriptDialog.SAVE_ID:
            intermediateSave(onSelectionChange);
            break;
         case SaveScriptDialog.SAVE_AS_ID:
            createNewScript(onSelectionChange);
            break;
         case SaveScriptDialog.DISCARD_ID:
            getScriptContent(); 
            clearDirtyFlags();
            break;
         default:
            scriptCombo.select(previousSelection);
            break;
      }
      return (rc == IDialogConstants.CANCEL_ID);
   }

   /**
    * Create new script
    */
   private void createNewScript(final boolean saveOnSelectionChange)
   {
      final CreateScriptDialog dlg = new CreateScriptDialog(getSite().getShell(), null);
      if (dlg.open() == Window.OK)
      {
         new ConsoleJob(Messages.get().ScriptExecutor_JobName_Create, this, Activator.PLUGIN_ID, null) {
            @Override
            protected void runInternal(IProgressMonitor monitor) throws Exception
            {
               session.modifyScript(0, dlg.getName(), scriptEditor.getText()); //$NON-NLS-1$
               
               runInUIThread(new Runnable()
               {
                  @Override
                  public void run()
                  {
                     Runnable run = new Runnable() {
                        @Override
                        public void run()
                        {
                           scriptCombo.select(scriptCombo.indexOf(dlg.getName()));
                        }
                     };
                     updateScriptList(saveOnSelectionChange ? null : run);
                     clearDirtyFlags();
                  }
               });
            }

            @Override
            protected String getErrorMessage()
            {
               return Messages.get().ScriptExecutor_JobError_Create;
            }
         }.start();
      }
   }

   /**
    * Updates content of script editor to selected by user script
    */
   protected void getScriptContent()
   {
      final int index = scriptCombo.getSelectionIndex();
      if (index == -1)
         return;
      
      new ConsoleJob(Messages.get().ScriptExecutor_JobName_Update, this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {            
            final Script script = session.getScript(library.get(index).getId());

            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  scriptEditor.setText(script.getSource());
                  clearDirtyFlags();
                  form.setText(script.getName());
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return Messages.get().ScriptExecutor_JobError_Update;
         }
      }.start();
   }
   
   /**
    * Execute script
    */
   protected void executeScript()
   {
      final String script = scriptEditor.getText();
      consoleOutputStream = output.newOutputStream();
      actionExecute.setEnabled(false);
      new ConsoleJob(Messages.get().ScriptExecutor_JobName_Execute, null, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.executeScript(objectId, script, ScriptExecutor.this);
         }
         
         @Override
         protected String getErrorMessage()
         {
            return Messages.get().ScriptExecutor_JobError_Execute;
         }

         /* (non-Javadoc)
          * @see org.netxms.ui.eclipse.jobs.ConsoleJob#jobFinalize()
          */
         @Override
         protected void jobFinalize()
         {
            try
            {
               consoleOutputStream.close();
               consoleOutputStream = null;
            }
            catch(IOException e)
            {
            }
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  actionExecute.setEnabled(true);
               }
            });
         } 
      }.start();      
   }
   
   /**
    * Populates list of scripts with scripts
    */
   private void updateScriptList(final Runnable postProcessor)
   {
      final String selection = (scriptCombo.getSelectionIndex() != -1) ? scriptCombo.getItem(scriptCombo.getSelectionIndex()) : null;
            
      new ConsoleJob(Messages.get().ScriptExecutor_JobName_ReadList, this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  scriptCombo.removeAll();
                  for(Script s : library)
                  {
                     scriptCombo.add(s.getName());
                  }
                  if (postProcessor != null)
                  {
                     postProcessor.run();
                  }
                  else
                  {
                     if(selection != null)
                     {
                        scriptCombo.select(scriptCombo.indexOf(selection));
                     }  
                  }
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return Messages.get().ScriptExecutor_JobError_ReadList;
         }
      }.start();
   }
   
   /**
    * Update script
    */
   public void intermediateSave(boolean saveOnSelectionChange)
   {
      final Script s = library.get( saveOnSelectionChange ? previousSelection : scriptCombo.getSelectionIndex());
      new ConsoleJob(Messages.get().ScriptExecutor_JobName_Update, this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.modifyScript(s.getId(), s.getName(), scriptEditor.getText());

            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  clearDirtyFlags();
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return Messages.get().ScriptExecutor_JobError_Update;
         }
      }.start();
   }
   
   /**
    * Contribute actions to action bar
    */
   private void contributeToActionBars()
   {
      IActionBars bars = getViewSite().getActionBars();
      fillLocalPullDown(bars.getMenuManager());
      fillLocalToolBar(bars.getToolBarManager());
   }

   /**
    * Update editor content
    */
   private void clearDirtyFlags()
   {
      modified = false;
      firePropertyChange(PROP_DIRTY);
      actionSave.setEnabled(false);
   }

   /**
    * Fill local pull-down menu
    * 
    * @param manager Menu manager for pull-down menu
    */
   private void fillLocalPullDown(IMenuManager manager)
   {
      manager.add(actionExecute);
      manager.add(actionClearOutput);
      manager.add(new Separator());
      manager.add(actionSave);
      manager.add(actionSaveAs);
      manager.add(actionClear);
      manager.add(new Separator());
      manager.add(actionReload);
   }

   /**
    * Fill local tool bar
    * 
    * @param manager Menu manager for local toolbar
    */
   private void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionExecute);
      manager.add(actionClearOutput);
      manager.add(new Separator());
      manager.add(actionSave);
      manager.add(actionSaveAs);
      manager.add(actionClear);
      manager.add(new Separator());
      manager.add(actionReload);
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.ui.ISaveablePart#doSave(org.eclipse.core.runtime.IProgressMonitor)
    */
   @Override
   public void doSave(IProgressMonitor monitor)
   {
      try
      {
         Script s = library.get(scriptCombo.getSelectionIndex());
         session.modifyScript(s.getId(), s.getName(), scriptEditor.getText());
      }
      catch(Exception e)
      {
         MessageDialogHelper.openError(getViewSite().getShell(), Messages.get().ScriptExecutor_Error,
               String.format(Messages.get().ScriptExecutor_SaveError, e.getLocalizedMessage()));
      }
      clearDirtyFlags();
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.ui.ISaveablePart#doSaveAs()
    */
   @Override
   public void doSaveAs()
   {
      createNewScript(false);
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.ui.ISaveablePart#isDirty()
    */
   @Override
   public boolean isDirty()
   {
      return modified;
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.ui.ISaveablePart#isSaveAsAllowed()
    */
   @Override
   public boolean isSaveAsAllowed()
   {
      return allowSaveAs;
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.ui.ISaveablePart#isSaveOnCloseNeeded()
    */
   @Override
   public boolean isSaveOnCloseNeeded()
   {
      return modified;
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.ui.ISaveablePart2#promptToSaveOnClose()
    */
   @Override
   public int promptToSaveOnClose()
   {
      SaveScriptDialog dlg = new SaveScriptDialog(getSite().getShell(), actionSave.isEnabled());
      int rc = dlg.open();
      modified = (rc != SaveScriptDialog.SAVE_ID);
      allowSaveAs = (rc == SaveScriptDialog.SAVE_AS_ID);
      return (rc == IDialogConstants.CANCEL_ID) ? CANCEL : ((rc == SaveScriptDialog.DISCARD_ID) ? NO : YES);
   }

   /* (non-Javadoc)
    * @see org.netxms.client.ActionExecutionListener#messageReceived(java.lang.String)
    */
   @Override
   public void messageReceived(final String text)
   {
      if (consoleOutputStream != null)
      {
         try
         {
            consoleOutputStream.write(text);
         }
         catch(IOException e)
         {
         }
      }
   }
}
