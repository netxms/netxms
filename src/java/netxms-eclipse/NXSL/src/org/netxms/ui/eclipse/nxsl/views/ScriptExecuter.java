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
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.SashForm;
import org.eclipse.swt.custom.StyledText;
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
import org.eclipse.ui.forms.widgets.FormToolkit;
import org.eclipse.ui.forms.widgets.ScrolledForm;
import org.eclipse.ui.forms.widgets.Section;
import org.eclipse.ui.part.ViewPart;
import org.netxms.api.client.scripts.Script;
import org.netxms.api.client.scripts.ScriptLibraryManager;
import org.netxms.client.ActionExecutionListener;
import org.netxms.client.NXCSession;
import org.netxms.client.agent.config.ConfigContent;
import org.netxms.client.datacollection.TransformationTestResult;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.nxsl.Activator;
import org.netxms.ui.eclipse.nxsl.dialogs.CreateScriptDialog;
import org.netxms.ui.eclipse.nxsl.dialogs.SaveScriptDialog;
import org.netxms.ui.eclipse.nxsl.widgets.ScriptEditor;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Sored on server agent's configuration editor
 */
public class ScriptExecuter extends ViewPart implements ISaveablePart2, ActionExecutionListener
{
   public static final String ID = "org.netxms.ui.eclipse.nxsl.views.ScriptExecuter"; //$NON-NLS-1$

   private NXCSession session;
   private ScriptLibraryManager scriptLibraryManager;
   private boolean modified = false;
   private long nodeId;

   private ScrolledForm form;
   private Combo scriptCombo;
   private ScriptEditor scriptEditor;
   private StyledText executionResult;
   private Action actionSave;
   private Action actionSaveAs;
   private Action actionClear;
   private Action actionReload;
   private Action actionExecute;
   private List<Script> library;
   private int previousSelection = -1;

   private boolean allowSaveAs = false;
   
   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
    */
   @Override
   public void init(IViewSite site) throws PartInitException
   {
      super.init(site);

      session = (NXCSession)ConsoleSharedData.getSession();
      nodeId = Long.parseLong(site.getSecondaryId());
      scriptLibraryManager = (ScriptLibraryManager)ConsoleSharedData.getSession();

      AbstractObject object = session.findObjectById(nodeId);
      setPartName("Script executer for node " + ((object != null) ? object.getObjectName() : Long.toString(nodeId)));
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createPartControl(Composite parent)
   {
      parent.setLayout(new FillLayout());

      /**** Script executor form ****/
      FormToolkit toolkit = new FormToolkit(getSite().getShell().getDisplay());

      Composite formContainer = new Composite(parent, SWT.NONE);
      GridLayout layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.numColumns = 2;
      layout.horizontalSpacing = 0;
      formContainer.setLayout(layout);      
      
      form = toolkit.createScrolledForm(formContainer);
      form.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
      form.setText("noname");
      
      layout = new GridLayout();
      form.getBody().setLayout(layout);
      GridData gridData = new GridData();
      gridData.horizontalAlignment = SWT.FILL;
      gridData.grabExcessHorizontalSpace = true;
      form.getBody().setLayoutData(gridData);       

      /**** Script list dropdown ****/
      scriptCombo = WidgetHelper.createLabeledCombo(form.getBody(), SWT.READ_ONLY, "Script library list", WidgetHelper.DEFAULT_LAYOUT_DATA);
      updateScriptList(null); 
      scriptCombo.addSelectionListener( new SelectionListener() {
         
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            System.out.println("selection changed");  
            if(modified)
            {
               if(saveIfRequired(true))
                  return;
            }           
            getScriptConetnt();        
            previousSelection = scriptCombo.getSelectionIndex();
         }
         
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            System.out.println("selection changed");   
            if(modified)
            {
               if(saveIfRequired(true))
                  return;
            }           
            getScriptConetnt();       
            previousSelection = scriptCombo.getSelectionIndex();  
         }
      });
      
      SashForm splitter = new SashForm(form.getBody(), SWT.VERTICAL);
      splitter.setSashWidth(3); 
      gridData = new GridData();
      gridData.horizontalAlignment = GridData.FILL;
      gridData.grabExcessHorizontalSpace = true;
      gridData.verticalAlignment = SWT.FILL;
      gridData.grabExcessVerticalSpace = true;
      splitter.setLayoutData(gridData);
      
      /**** Script editor  ****/
      Section section = toolkit.createSection(splitter, Section.TITLE_BAR);
      section.setText("Filter");
      section.setLayout(layout);
      gridData = new GridData();
      gridData.horizontalAlignment = GridData.FILL;
      gridData.grabExcessHorizontalSpace = true;
      gridData.verticalAlignment = SWT.FILL;
      gridData.grabExcessVerticalSpace = true;
      section.setLayoutData(gridData);
      
      scriptEditor = new ScriptEditor(section, SWT.BORDER, SWT.H_SCROLL | SWT.V_SCROLL, 0);
      section.setClient(scriptEditor);
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
      section = toolkit.createSection(splitter, Section.TITLE_BAR);
      section.setText("Execution result");
      gridData = new GridData();
      gridData.horizontalAlignment = GridData.FILL;
      gridData.grabExcessHorizontalSpace = true;
      gridData.verticalAlignment = SWT.FILL;
      gridData.grabExcessVerticalSpace = true;
      section.setLayoutData(gridData);
      
      executionResult = new StyledText(section, SWT.H_SCROLL | SWT.V_SCROLL);
      executionResult.setEditable(false);
      executionResult.setFont(JFaceResources.getTextFont());
      /*executionResult.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            actionCopy.setEnabled(executionResult.getSelectionCount() > 0);
         }
      });*/ // TODO: Think how to split copy action between 2 editors
      
      gridData = new GridData();
      gridData.horizontalAlignment = GridData.FILL;
      gridData.grabExcessHorizontalSpace = true;
      gridData.verticalAlignment = SWT.FILL;
      gridData.grabExcessVerticalSpace = true;
      executionResult.setLayoutData(gridData);

      createActions();
      contributeToActionBars();
      
      actionSave.setEnabled(false);
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

   /*
    * (non-Javadoc)
    * 
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
      actionSave = new Action("Save", SharedIcons.SAVE) {
         @Override
         public void run()
         {
            intermidiateSave(false);
         }
      };
      
      actionSaveAs = new Action("Save as", SharedIcons.SAVE) { //TODO: Download new icon
         @Override
         public void run()
         {
            createNewScript(false); 
         }
      };
      
      actionClear = new Action("Clear", SharedIcons.CLEAR) {
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
            executionResult.setText("");
            form.setText("noname");
         }
      };
      
      actionReload = new Action("Reload script", SharedIcons.REFRESH) {
         @Override
         public void run()
         {
            if(modified)
            {
               if(saveIfRequired(false))
                  return;
            }
            
            updateScriptList(null);
            getScriptConetnt();
            executionResult.setText("");
         }
      };
      
      actionExecute = new Action("Execute", SharedIcons.EXECUTE) {
         @Override
         public void run()
         {
            executeScript();
         }
      };
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
            intermidiateSave(onSelectionChange);
            break;
         case SaveScriptDialog.SAVE_AS_ID:
            createNewScript(onSelectionChange);
            break;
         case SaveScriptDialog.DISCARD_ID:
            getScriptConetnt(); 
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
         new ConsoleJob("Create new script", this, Activator.PLUGIN_ID, null) {
            @Override
            protected void runInternal(IProgressMonitor monitor) throws Exception
            {
               scriptLibraryManager.modifyScript(0, dlg.getName(), scriptEditor.getText()); //$NON-NLS-1$
               
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
                           System.out.println("Trying to set newly created item");
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
               return "Error modifying script";
            }
         }.start();
      }
   }

   /**
    * Updates content of script editor to selected by user script
    */
   protected void getScriptConetnt()
   {
      final int index = scriptCombo.getSelectionIndex();
      if(index == -1)
         return;
      
      new ConsoleJob("Update script content", this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {            
            final Script script = session.getScript(library.get(index).getId());

            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  scriptEditor.setText(script.getSource());
                  form.setText(script.getName());
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return "Not possible to get script content";
         }
      }.start();
   }
   
   /**
    * Execute script
    */
   protected void executeScript()
   {
      final String scrpt = scriptEditor.getText();
      new ConsoleJob("Execute script", null, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.executeScript(nodeId, scrpt, ScriptExecuter.this);
         }
         
         @Override
         protected String getErrorMessage()
         {
            return "Error executing script";
         } 
      }.start();      
   }
   
   /**
    * Populates list of scripts with scripts
    */
   private void updateScriptList(final Runnable postProcessor)
   {
      final String selection = (scriptCombo.getSelectionIndex() != -1) ? scriptCombo.getItem(scriptCombo.getSelectionIndex()) : null;
            
      new ConsoleJob("Update script list", this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            library = scriptLibraryManager.getScriptLibrary();

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
            return "Not possible to get script list";
         }
      }.start();
   }
   
   /**
    * Update script
    */
   public void intermidiateSave(boolean saveOnSelectionChange)
   {
      final Script s = library.get( saveOnSelectionChange ? previousSelection : scriptCombo.getSelectionIndex());
      new ConsoleJob("Update script", this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            scriptLibraryManager.modifyScript(s.getId(), s.getName(), scriptEditor.getText());

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
            return "Not possible to save script";
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
      manager.add(actionSave);
      manager.add(actionSaveAs);
      manager.add(actionClear);
      manager.add(actionReload);
      manager.add(new Separator());
      manager.add(actionExecute);
   }

   /**
    * Fill local tool bar
    * 
    * @param manager Menu manager for local toolbar
    */
   private void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionSave);
      manager.add(actionSaveAs);
      manager.add(actionClear);
      manager.add(actionReload);
      manager.add(new Separator());
      manager.add(actionExecute);
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
         scriptLibraryManager.modifyScript(s.getId(), s.getName(), scriptEditor.getText());
      }
      catch(Exception e)
      {
         MessageDialogHelper.openError(getViewSite().getShell(), "Error",
               "Error while saving script: " + e.getMessage());
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
      new ConsoleJob("Update script", this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {

            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  executionResult.setText(executionResult.getText() + text); //Shold be writen with ID(If user runs 2 scripts simultaniously output could be mixed.
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return "Not possible to save script";
         }
      }.start();
   }
}
