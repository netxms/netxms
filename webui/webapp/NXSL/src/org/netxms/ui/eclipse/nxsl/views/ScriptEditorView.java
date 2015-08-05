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
package org.netxms.ui.eclipse.nxsl.views;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.ISaveablePart;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.commands.ActionHandler;
import org.eclipse.ui.contexts.IContextService;
import org.eclipse.ui.handlers.IHandlerService;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.Script;
import org.netxms.client.ScriptCompilationResult;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.nxsl.Activator;
import org.netxms.ui.eclipse.nxsl.Messages;
import org.netxms.ui.eclipse.nxsl.widgets.ScriptEditor;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.widgets.CompositeWithMessageBar;

/**
 * Script editor view
 */
@SuppressWarnings("deprecation")
public class ScriptEditorView extends ViewPart implements ISaveablePart
{
	public static final String ID = "org.netxms.ui.eclipse.nxsl.views.ScriptEditorView"; //$NON-NLS-1$
	
	private NXCSession session;
	private CompositeWithMessageBar editorMessageBar;
	private ScriptEditor editor;
	private long scriptId;
	private String scriptName;
	private Action actionRefresh;
	private Action actionSave;
	private Action actionCompile;
	private boolean modified = false;
	private boolean showLineNumbers = true;
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
	 */
	@Override
	public void init(IViewSite site) throws PartInitException
	{
		super.init(site);
		
		session = ConsoleSharedData.getSession();
		scriptId = Long.parseLong(site.getSecondaryId());
		
		IDialogSettings settings = Activator.getDefault().getDialogSettings();
		if (settings.get("ScriptEditor.showLineNumbers") != null)
		   showLineNumbers = settings.getBoolean("ScriptEditor.showLineNumbers");
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		parent.setLayout(new FillLayout());
		
		editorMessageBar = new CompositeWithMessageBar(parent, SWT.NONE);
		editor = new ScriptEditor(editorMessageBar, SWT.NONE, SWT.H_SCROLL | SWT.V_SCROLL, showLineNumbers);
		editorMessageBar.setContent(editor);
      editor.getTextWidget().addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            if (!modified)
            {
               modified = true;
               firePropertyChange(PROP_DIRTY);
               actionSave.setEnabled(true);
            }
         }
      });
		
		activateContext();
		createActions();
		contributeToActionBars();
		//createPopupMenu();

		reloadScript();
	}

   /**
    * Activate context
    */
   private void activateContext()
   {
      IContextService contextService = (IContextService)getSite().getService(IContextService.class);
      if (contextService != null)
      {
         contextService.activateContext("org.netxms.ui.eclipse.nxsl.context.ScriptEditor"); //$NON-NLS-1$
      }
   }

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		editor.setFocus();
	}
	
	/**
	 * Create actions
	 */
   private void createActions()
	{
      final IHandlerService handlerService = (IHandlerService)getSite().getService(IHandlerService.class);
      
		actionRefresh = new RefreshAction(this) {
			@Override
			public void run()
			{
				reloadScript();
			}
		};
		
		actionSave = new Action(Messages.get().ScriptEditorView_Save, SharedIcons.SAVE) {
			@Override
			public void run()
			{
				saveScript();
			}
		};
      actionSave.setActionDefinitionId("org.netxms.ui.eclipse.nxsl.commands.save"); //$NON-NLS-1$
      handlerService.activateHandler(actionSave.getActionDefinitionId(), new ActionHandler(actionSave));
		
		actionCompile = new Action("&Compile", Activator.getImageDescriptor("icons/compile.gif")) {
         @Override
         public void run()
         {
            compileScript();
         }
      };
      actionCompile.setActionDefinitionId("org.netxms.ui.eclipse.nxsl.commands.compile"); //$NON-NLS-1$
      handlerService.activateHandler(actionCompile.getActionDefinitionId(), new ActionHandler(actionCompile));
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
	 * Fill local pull-down menu
	 * 
	 * @param manager
	 *           Menu manager for pull-down menu
	 */
	private void fillLocalPullDown(IMenuManager manager)
	{
		manager.add(actionCompile);
		manager.add(actionSave);
		manager.add(new Separator());
		manager.add(actionRefresh);
	}

	/**
	 * Fill local tool bar
	 * 
	 * @param manager
	 *           Menu manager for local toolbar
	 */
	private void fillLocalToolBar(IToolBarManager manager)
	{
      manager.add(actionCompile);
		manager.add(actionSave);
		manager.add(new Separator());
		manager.add(actionRefresh);
	}
	
	/**
	 * Reload script from server
	 */
	private void reloadScript()
	{
		new ConsoleJob(String.format(Messages.get().ScriptEditorView_LoadJobTitle, scriptId), this, Activator.PLUGIN_ID, null) {
			@Override
			protected String getErrorMessage()
			{
				return String.format(Messages.get().ScriptEditorView_LoadJobError, scriptId);
			}

			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final Script script = session.getScript(scriptId);
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						scriptName = script.getName();
						setPartName(String.format(Messages.get().ScriptEditorView_PartName, scriptName));
						editor.setText(script.getSource());
						actionSave.setEnabled(false);
						modified = false;
		            firePropertyChange(PROP_DIRTY);
					}
				});
			}
		}.start();
	}
	
	/**
	 * Compile script
	 */
	private void compileScript()
	{
      final String source = editor.getText();
      editor.getTextWidget().setEditable(false);
      new ConsoleJob("Compile script", this, Activator.PLUGIN_ID, null) {
         @Override
         protected String getErrorMessage()
         {
            return "Cannot compile script";
         }

         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            final ScriptCompilationResult result = session.compileScript(source, false);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  if (result.success)
                  {
                     editorMessageBar.showMessage(CompositeWithMessageBar.INFORMATION, "Script compiled successfully");
                  }
                  else
                  {
                     editorMessageBar.showMessage(CompositeWithMessageBar.WARNING, result.errorMessage);
                  }
                  editor.getTextWidget().setEditable(true);
               }
            });
         }
      }.start();
	}

	/**
	 * Save script
	 */
	private void saveScript()
	{
		final String source = editor.getText();
		editor.getTextWidget().setEditable(false);
		new ConsoleJob(Messages.get().ScriptEditorView_SaveJobTitle, this, Activator.PLUGIN_ID, null) {
			@Override
			protected String getErrorMessage()
			{
				return Messages.get().ScriptEditorView_SaveJobError;
			}

			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
            final ScriptCompilationResult result = session.compileScript(source, false);
            if (result.success)
            {
               getDisplay().syncExec(new Runnable() {
                  @Override
                  public void run()
                  {
                     editorMessageBar.hideMessage();
                  }
               });
            }
            else
            {
               getDisplay().syncExec(new Runnable() {
                  @Override
                  public void run()
                  {
                     if (MessageDialogHelper.openQuestion(getSite().getShell(), "Compilation Errors", 
                           String.format("Script compilation failed (%s)\r\nSave changes anyway?", result.errorMessage)))
                        result.success = true;
                     editorMessageBar.showMessage(CompositeWithMessageBar.WARNING, result.errorMessage);
                  }
               });
            }
            if (result.success)
            {
				doScriptSave(source, monitor);
			}
            else
            {
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     editor.getTextWidget().setEditable(true);
                  }
               });
            }
			}
		}.start();
	}
	
	/**
	 * Do actual script save
	 * 
	 * @param source
	 * @param monitor
	 * @throws Exception
	 */
	private void doScriptSave(String source, IProgressMonitor monitor) throws Exception
	{
		session.modifyScript(scriptId, scriptName, source);
		editor.getDisplay().asyncExec(new Runnable() {
         @Override
         public void run()
         {
            if (editor.isDisposed())
               return;
            
            editor.getTextWidget().setEditable(true);
            actionSave.setEnabled(false);
            modified = false;
            firePropertyChange(PROP_DIRTY);
         }
      });
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#getAdapter(java.lang.Class)
	 */
	@SuppressWarnings("rawtypes")
	@Override
	public Object getAdapter(Class adapter)
	{
		Object object = super.getAdapter(adapter);
		if (object != null)
		{
			return object;
		}
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.ISaveablePart#doSave(org.eclipse.core.runtime.IProgressMonitor)
	 */
	@Override
	public void doSave(IProgressMonitor monitor)
	{
		final String source = editor.getText();
		editor.getTextWidget().setEditable(false);
		try
		{
			doScriptSave(source, monitor);
		}
		catch(Exception e)
		{
			MessageDialogHelper.openError(getViewSite().getShell(), Messages.get().ScriptEditorView_Error, String.format(Messages.get().ScriptEditorView_SaveErrorMessage, e.getMessage()));
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.ISaveablePart#doSaveAs()
	 */
	@Override
	public void doSaveAs()
	{
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.ISaveablePart#isDirty()
	 */
	@Override
	public boolean isDirty()
	{
		return modified;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.ISaveablePart#isSaveAsAllowed()
	 */
	@Override
	public boolean isSaveAsAllowed()
	{
		return false;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.ISaveablePart#isSaveOnCloseNeeded()
	 */
	@Override
	public boolean isSaveOnCloseNeeded()
	{
		return modified;
	}

   /* (non-Javadoc)
    * @see org.eclipse.ui.part.WorkbenchPart#dispose()
    */
   @Override
   public void dispose()
   {
      IDialogSettings settings = Activator.getDefault().getDialogSettings();
      settings.put("ScriptEditor.showLineNumbers", showLineNumbers);
      super.dispose();
   }
}
