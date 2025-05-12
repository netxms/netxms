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
package org.netxms.nxmc.modules.nxsl.views;

import java.io.IOException;
import java.util.Collections;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.SashForm;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.Script;
import org.netxms.client.TextOutputAdapter;
import org.netxms.nxmc.Memento;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.views.ViewNotRestoredException;
import org.netxms.nxmc.base.widgets.TextConsole;
import org.netxms.nxmc.base.widgets.TextConsole.IOConsoleOutputStream;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.nxsl.dialogs.CreateScriptDialog;
import org.netxms.nxmc.modules.nxsl.dialogs.SaveScriptDialog;
import org.netxms.nxmc.modules.nxsl.widgets.ScriptEditor;
import org.netxms.nxmc.modules.objects.views.AdHocObjectView;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Sored on server agent's configuration editor
 */
public class ScriptExecutorView extends AdHocObjectView
{
   private final I18n i18n = LocalizationHelper.getI18n(ScriptExecutorView.class);

   private Label scriptName;
   private Combo scriptCombo;
   private ScriptEditor scriptEditor;
   private Text parametersField;
   private TextConsole output;
   private IOConsoleOutputStream consoleOutputStream;
   private long vmId;
   private Action actionSave;
   private Action actionSaveAs;
   private Action actionClearSource;
   private Action actionClearOutput;
   private Action actionExecute;
   private Action actionClearOutputAndExecute;
   private Action actionStop;
   private List<Script> library;
   private int previousSelection = -1;
   private boolean modified = false;
   private Runnable postRefreshAction = null;

   /**
    * Create agent configuration editor for given node.
    *
    * @param node node object
    */
   public ScriptExecutorView(long objectId, long context)
   {
      super(LocalizationHelper.getI18n(ScriptExecutorView.class).tr("Execute Script"), ResourceManager.getImageDescriptor("icons/object-views/script-executor.png"), "objects.script-executor", objectId, context, false);
   }  

   /**
    * Create agent configuration editor for given node.
    *
    * @param node node object
    */
   protected ScriptExecutorView()
   {
      super(LocalizationHelper.getI18n(ScriptExecutorView.class).tr("Execute Script"), ResourceManager.getImageDescriptor("icons/object-views/script-executor.png"), "objects.script-executor", 0, 0, false);  
   }

   /**
    * @see org.netxms.nxmc.base.views.View#postClone(org.netxms.nxmc.base.views.View)
    */
   @Override
   protected void postClone(View origin)
   {
      super.postClone(origin);

      ScriptExecutorView view = (ScriptExecutorView)origin;
      scriptName.setText(view.scriptName.getText());      
      parametersField.setText(view.parametersField.getText());
      output.setText(view.output.getText());

      Runnable run = new Runnable() {
         @Override
         public void run()
         {
            scriptCombo.select(view.scriptCombo.getSelectionIndex());
            scriptEditor.setText(view.scriptEditor.getText());
            actionSave.setEnabled(view.actionSave.isEnabled());        
         }
      };
      updateScriptList(run);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createContent(Composite parent)
   {
      Composite formContainer = new Composite(parent, SWT.NONE);
      GridLayout layout = new GridLayout();
      layout.verticalSpacing = 8;
      formContainer.setLayout(layout);

      scriptName = new Label(formContainer, SWT.LEFT);
      scriptName.setFont(JFaceResources.getBannerFont());
      scriptName.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      scriptName.setText(i18n.tr("Noname"));

      /**** Script list dropdown ****/
      scriptCombo = WidgetHelper.createLabeledCombo(formContainer, SWT.READ_ONLY, i18n.tr("Script from library"), WidgetHelper.DEFAULT_LAYOUT_DATA);
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

      SashForm splitter = new SashForm(formContainer, SWT.VERTICAL);
      splitter.setSashWidth(3); 
      GridData gridData = new GridData();
      gridData.horizontalAlignment = GridData.FILL;
      gridData.grabExcessHorizontalSpace = true;
      gridData.verticalAlignment = SWT.FILL;
      gridData.grabExcessVerticalSpace = true;
      splitter.setLayoutData(gridData);

      /**** Script parameters  ****/      
      Composite container = new Composite(splitter, SWT.NONE);
      layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.marginBottom = 4;
      container.setLayout(layout);

      Label label = new Label(container, SWT.LEFT);
      label.setText(i18n.tr("Parameters (comma-separated list)"));

      parametersField = new Text(container, SWT.SINGLE | SWT.BORDER);
      gridData = new GridData();
      gridData.horizontalAlignment = GridData.FILL;
      gridData.grabExcessHorizontalSpace = true;
      parametersField.setLayoutData(gridData);

      /**** Script editor  ****/
      label = new Label(container, SWT.LEFT);
      label.setText(i18n.tr("Source"));

      scriptEditor = new ScriptEditor(container, SWT.BORDER, SWT.H_SCROLL | SWT.V_SCROLL);
      scriptEditor.setText("");
      scriptEditor.getTextWidget().addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            onTextModify();
         }
      });
      gridData = new GridData();
      gridData.horizontalAlignment = GridData.FILL;
      gridData.grabExcessHorizontalSpace = true;
      gridData.verticalAlignment = SWT.FILL;
      gridData.grabExcessVerticalSpace = true;
      scriptEditor.setLayoutData(gridData);

      /**** Execution result ****/
      container = new Composite(splitter, SWT.NONE);
      layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.marginTop = 4;
      container.setLayout(layout);

      label = new Label(container, SWT.LEFT);
      label.setText(i18n.tr("Output"));

      output = new TextConsole(container, SWT.BORDER);
      gridData = new GridData();
      gridData.horizontalAlignment = GridData.FILL;
      gridData.grabExcessHorizontalSpace = true;
      gridData.verticalAlignment = SWT.FILL;
      gridData.grabExcessVerticalSpace = true;
      output.setLayoutData(gridData);

      createActions();

      actionSave.setEnabled(false);
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#postContentCreate()
    */
   @Override
   protected void postContentCreate()
   {
      super.postContentCreate();   
      updateScriptList(postRefreshAction);
   }

   /**
    * On text modify
    */
   private void onTextModify()
   {
      if (!modified)
      {
         modified = true;
         if (scriptCombo.getSelectionIndex() != -1)
            actionSave.setEnabled(true);
      }
   }

   /**
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
      actionSave = new Action(i18n.tr("&Save"), SharedIcons.SAVE) {
         @Override
         public void run()
         {
            saveScript(false, false);
         }
      };
      addKeyBinding("M1+S", actionSave);

      actionSaveAs = new Action(i18n.tr("Save &as..."), SharedIcons.SAVE_AS) {
         @Override
         public void run()
         {
            createNewScript(false, false);
         }
      };
      addKeyBinding("M1+M2+S", actionSaveAs);

      actionClearSource = new Action(i18n.tr("&Clear source"), SharedIcons.CLEAR) {
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
            scriptEditor.setText("");
            output.clear();
            scriptName.setText(i18n.tr("Noname"));
         }
      };

      actionClearOutput = new Action(i18n.tr("Clear &output"), SharedIcons.CLEAR_LOG) {
         @Override
         public void run()
         {
            output.clear();
         }
      };
      addKeyBinding("M1+L", actionClearOutput);

      actionExecute = new Action(i18n.tr("E&xecute"), SharedIcons.EXECUTE) {
         @Override
         public void run()
         {
            executeScript();
         }
      };
      addKeyBinding("F9", actionExecute);

      actionClearOutputAndExecute = new Action(i18n.tr("Clear o&utput and execute"), ResourceManager.getImageDescriptor("icons/nxsl/clear-and-execute.png")) {
         @Override
         public void run()
         {
            output.clear();
            executeScript();
         }
      };
      addKeyBinding("M2+F9", actionClearOutputAndExecute);

      actionStop = new Action(i18n.tr("&Stop"), SharedIcons.TERMINATE) {
         @Override
         public void run()
         {
            stopScript();
         }
      };
      actionStop.setEnabled(false);
      addKeyBinding("M1+T", actionStop);
   }  

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      if (modified)
      {
         if (saveIfRequired(false))
            return;
      }
      updateScriptList(null);
      getScriptContent();
      output.clear();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#beforeClose()
    */
   @Override
   public boolean beforeClose()
   {
      if (!modified)
         return true;

      SaveScriptDialog dlg = new SaveScriptDialog(getWindow().getShell(), actionSave.isEnabled());
      int rc = dlg.open();
      switch(rc)
      {
         case SaveScriptDialog.SAVE_ID:
            saveScript(false, true);
            break;
         case SaveScriptDialog.SAVE_AS_ID:
            createNewScript(false, true);
            break;
         case SaveScriptDialog.DISCARD_ID:
            getScriptContent();
            clearModificationFlag();
            break;
         default:
            scriptCombo.select(previousSelection);
            break;
      }
      return (rc != IDialogConstants.CANCEL_ID);
   }

   /**
    * Ask if save, save as, cancel or discard action should be done
    * 
    * @param onSelectionChange set to true if save happens because of library script selection change
    */
   private boolean saveIfRequired(boolean onSelectionChange)
   {
      SaveScriptDialog dlg = new SaveScriptDialog(getWindow().getShell(), actionSave.isEnabled());
      int rc = dlg.open();
      switch(rc)
      {
         case SaveScriptDialog.SAVE_ID:
            saveScript(onSelectionChange, false);
            break;
         case SaveScriptDialog.SAVE_AS_ID:
            createNewScript(onSelectionChange, false);
            break;
         case SaveScriptDialog.DISCARD_ID:
            getScriptContent(); 
            clearModificationFlag();
            break;
         default:
            scriptCombo.select(previousSelection);
            break;
      }
      return (rc == IDialogConstants.CANCEL_ID);
   }

   /**
    * Create new script
    * 
    * @param saveOnSelectionChange true if called because of library script selection change
    * @param saveOnClose true if called on closing view
    */
   private boolean createNewScript(final boolean saveOnSelectionChange, final boolean saveOnClose)
   {
      final CreateScriptDialog dlg = new CreateScriptDialog(getWindow().getShell(), null);
      if (dlg.open() != Window.OK)
         return false;

      final String scriptSource = scriptEditor.getText();
      new Job(i18n.tr("Creating library script"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.modifyScript(0, dlg.getName(), scriptSource);
            if (!saveOnClose)
            {
               runInUIThread(new Runnable() {
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
                     clearModificationFlag();
                  }
               });
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot create library script");
         }
      }.start();
      return true;
   }

   /**
    * Updates content of script editor to selected by user script
    */
   protected void getScriptContent()
   {
      final int index = scriptCombo.getSelectionIndex();
      if (index == -1)
         return;

      new Job(i18n.tr("Loading library script"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {            
            final Script script = session.getScript(library.get(index).getId());
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  scriptEditor.setText(script.getSource());
                  clearModificationFlag();
                  scriptName.setText(script.getName());
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot load library script");
         }
      }.start();
   }

   /**
    * Execute script
    */
   protected void executeScript()
   {
      final String script = scriptEditor.getText();
      final String parameters = parametersField.getText();
      consoleOutputStream = output.newOutputStream();
      actionExecute.setEnabled(false);
      actionClearOutputAndExecute.setEnabled(false);
      actionStop.setEnabled(true);
      clearMessages();
      Job job = new Job(i18n.tr("Executing script"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.executeScript(getObjectId(), script, parameters, new TextOutputAdapter() {
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

               @Override
               public void setStreamId(long streamId)
               {
                  vmId = streamId;
               }
            }, true);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot execute script");
         } 

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
            runInUIThread(() -> {
               actionExecute.setEnabled(true);
               actionClearOutputAndExecute.setEnabled(true);
               actionStop.setEnabled(false);
            });
         } 
      };
      job.setUser(false);
      job.start();
   }

   /**
    * Stop running script
    */
   private void stopScript()
   {
      new Job(i18n.tr("Stopping script"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.stopScript(vmId);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot stop script");
         }
      }.start();
   }

   /**
    * Populates list of scripts with scripts
    */
   private void updateScriptList(final Runnable postProcessor)
   {
      final String selection = (scriptCombo.getSelectionIndex() != -1) ? scriptCombo.getItem(scriptCombo.getSelectionIndex()) : null;

      new Job(i18n.tr("Reading list of library scripts"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            library = session.getScriptLibrary();
            Collections.sort(library, (lhs, rhs) -> lhs.getName().compareTo(rhs.getName()));
            runInUIThread(() -> {
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
                  if (selection != null)
                  {
                     scriptCombo.select(scriptCombo.indexOf(selection));
                  }
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot read list of library scripts");
         }
      }.start();
   }

   /**
    * Save script
    *
    * @param saveOnSelectionChange true if called because of library script selection change
    * @param saveOnClose true if called on closing view
    */
   public void saveScript(boolean saveOnSelectionChange, final boolean saveOnClose)
   {
      final Script s = library.get(saveOnSelectionChange ? previousSelection : scriptCombo.getSelectionIndex());
      final String scriptSource = scriptEditor.getText();
      new Job(i18n.tr("Updating library script"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.modifyScript(s.getId(), s.getName(), scriptSource);
            if (!saveOnClose)
            {
               runInUIThread(() -> clearModificationFlag());
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot update library script");
         }
      }.start();
   }

   /**
    * Update editor content
    */
   private void clearModificationFlag()
   {
      modified = false;
      actionSave.setEnabled(false);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionExecute);
      manager.add(actionClearOutputAndExecute);
      manager.add(actionStop);
      manager.add(actionClearOutput);
      manager.add(new Separator());
      manager.add(actionSave);
      manager.add(actionSaveAs);
      manager.add(actionClearSource);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionExecute);
      manager.add(actionClearOutputAndExecute);
      manager.add(actionStop);
      manager.add(actionClearOutput);
      manager.add(new Separator());
      manager.add(actionSave);
      manager.add(actionSaveAs);
      manager.add(actionClearSource);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#saveState(org.netxms.nxmc.Memento)
    */
   @Override
   public void saveState(Memento memento)
   {
      super.saveState(memento);        
      memento.set("scriptName", scriptName.getText());  
      memento.set("parameters", parametersField.getText());
      memento.set("scriptSelection", scriptCombo.getSelectionIndex());  
      if (modified)
         memento.set("scripText", scriptEditor.getText());
   }

   /**
    * @throws ViewNotRestoredException 
    * @see org.netxms.nxmc.base.views.ViewWithContext#restoreState(org.netxms.nxmc.Memento)
    */
   @Override
   public void restoreState(Memento memento) throws ViewNotRestoredException
   {      
      super.restoreState(memento);

      postRefreshAction = new Runnable() {
         @Override
         public void run()
         {            
            int selection = memento.getAsInteger("scriptSelection", -1);
            scriptCombo.select(selection);
            String script = memento.getAsString("scripText", null);
            if (script != null)
            {
               scriptEditor.setText(script);
               actionSave.setEnabled(true);     
               modified = true;
            }    
            else
            {
               getScriptContent();
            }
            parametersField.setText(memento.getAsString("parameters", ""));
         }
      };
   }
}
