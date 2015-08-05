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

import java.io.IOException;
import java.io.InputStream;
import java.util.PropertyResourceBundle;
import java.util.ResourceBundle;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.dialogs.IInputValidator;
import org.eclipse.jface.dialogs.InputDialog;
import org.eclipse.jface.text.IFindReplaceTarget;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.StyledText;
import org.eclipse.swt.dnd.Clipboard;
import org.eclipse.swt.dnd.TextTransfer;
import org.eclipse.swt.dnd.TransferData;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.ISaveablePart;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.commands.ActionHandler;
import org.eclipse.ui.contexts.IContextService;
import org.eclipse.ui.handlers.IHandlerService;
import org.eclipse.ui.part.ViewPart;
import org.eclipse.ui.texteditor.FindReplaceAction;
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
	
	private static final Color ERROR_COLOR = new Color(Display.getDefault(), 255, 0, 0);
	
	private NXCSession session;
	private CompositeWithMessageBar editorMessageBar;
	private ScriptEditor editor;
	private long scriptId;
	private String scriptName;
	private Action actionRefresh;
	private Action actionSave;
	private Action actionCompile;
	private Action actionShowLineNumbers;
	private Action actionGoToLine;
	private Action actionSelectAll;
   private Action actionCut;
   private Action actionCopy;
   private Action actionPaste;
	private FindReplaceAction actionFindReplace;
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
		editor.getTextWidget().addModifyListener(new ModifyListener() {
			@Override
			public void modifyText(ModifyEvent e)
			{
				if (!modified)
				{
					modified = true;
					firePropertyChange(PROP_DIRTY);
					actionSave.setEnabled(true);
					actionFindReplace.update();
					
					boolean selected = editor.getTextWidget().getSelectionCount() > 0;
					actionCut.setEnabled(selected);
               actionCopy.setEnabled(selected);
				}
			}
		});
		editor.getTextWidget().addSelectionListener(new SelectionListener() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            boolean selected = editor.getTextWidget().getSelectionCount() > 0;
            actionCut.setEnabled(selected);
            actionCopy.setEnabled(selected);
         }
         
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }
      });
		editorMessageBar.setContent(editor);

		activateContext();
		createActions();
		contributeToActionBars();
		createPopupMenu();

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
	 * Get resource bundle
	 * @return
	 * @throws IOException
	 */
	private ResourceBundle getResourceBundle() throws IOException
	{
		InputStream in = null;
		String resource = "resource.properties"; //$NON-NLS-1$
		ClassLoader loader = this.getClass().getClassLoader();
		if (loader != null)
		{
			in = loader.getResourceAsStream(resource);
		}
		else
		{
			in = ClassLoader.getSystemResourceAsStream(resource);
		}
		
		return new PropertyResourceBundle(in);
	}

	/**
	 * Create actions
	 */
	private void createActions()
	{
      final IHandlerService handlerService = (IHandlerService)getSite().getService(IHandlerService.class);
      
		try
		{
			actionFindReplace = new FindReplaceAction(getResourceBundle(), "actions.find_and_replace.", this); //$NON-NLS-1$
			handlerService.activateHandler("org.eclipse.ui.edit.findReplace", new ActionHandler(actionFindReplace)); 		 //$NON-NLS-1$
		}
		catch(IOException e)
		{
		   Activator.logError("Cannot create find/replace action", e);
		}
		
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
      
      actionShowLineNumbers = new Action("Show line &numbers", Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            showLineNumbers = actionShowLineNumbers.isChecked();
            editor.showLineNumbers(showLineNumbers);
         }
      };
      actionShowLineNumbers.setChecked(showLineNumbers);
      actionShowLineNumbers.setActionDefinitionId("org.netxms.ui.eclipse.nxsl.commands.showLineNumbers"); //$NON-NLS-1$
      handlerService.activateHandler(actionShowLineNumbers.getActionDefinitionId(), new ActionHandler(actionShowLineNumbers));
      
      actionGoToLine = new Action("&Go to line...") {
         @Override
         public void run()
         {
            goToLine();
         }
      };
      actionGoToLine.setActionDefinitionId("org.netxms.ui.eclipse.nxsl.commands.goToLine"); //$NON-NLS-1$
      handlerService.activateHandler(actionGoToLine.getActionDefinitionId(), new ActionHandler(actionGoToLine));
      
      actionSelectAll = new Action("Select &all") {
         @Override
         public void run()
         {
            editor.getTextWidget().selectAll();
         }
      };
      actionSelectAll.setActionDefinitionId("org.netxms.ui.eclipse.nxsl.commands.selectAll"); //$NON-NLS-1$
      handlerService.activateHandler(actionSelectAll.getActionDefinitionId(), new ActionHandler(actionSelectAll));

      actionCut = new Action("C&ut", SharedIcons.CUT) {
         @Override
         public void run()
         {
            editor.getTextWidget().cut();
         }
      };
      actionCut.setActionDefinitionId("org.netxms.ui.eclipse.nxsl.commands.cut"); //$NON-NLS-1$
      handlerService.activateHandler(actionCut.getActionDefinitionId(), new ActionHandler(actionCut));
      actionCut.setEnabled(false);

      actionCopy = new Action("&Copy", SharedIcons.COPY) {
         @Override
         public void run()
         {
            editor.getTextWidget().copy();
         }
      };
      actionCopy.setActionDefinitionId("org.netxms.ui.eclipse.nxsl.commands.copy"); //$NON-NLS-1$
      handlerService.activateHandler(actionCopy.getActionDefinitionId(), new ActionHandler(actionCopy));
      actionCopy.setEnabled(false);

      actionPaste = new Action("&Paste", SharedIcons.PASTE) {
         @Override
         public void run()
         {
            editor.getTextWidget().paste();
         }
      };
      actionPaste.setActionDefinitionId("org.netxms.ui.eclipse.nxsl.commands.paste"); //$NON-NLS-1$
      handlerService.activateHandler(actionPaste.getActionDefinitionId(), new ActionHandler(actionPaste));
      actionPaste.setEnabled(canPaste());
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
		manager.add(actionFindReplace);
      manager.add(actionGoToLine);
      manager.add(new Separator());
      manager.add(actionCut);
      manager.add(actionCopy);
      manager.add(actionPaste);
      manager.add(actionSelectAll);
      manager.add(new Separator());
      manager.add(actionShowLineNumbers);
      manager.add(new Separator());
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
    * Create pop-up menu for user list
    */
   private void createPopupMenu()
   {
      // Create menu manager
      MenuManager menuMgr = new MenuManager();
      menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener(new IMenuListener() {
         public void menuAboutToShow(IMenuManager manager)
         {
            fillContextMenu(manager);
         }
      });

      // Create menu
      Menu menu = menuMgr.createContextMenu(editor.getTextWidget());
      editor.getTextWidget().setMenu(menu);
   }

   /**
    * Fill context menu
    * 
    * @param mgr Menu manager
    */
   protected void fillContextMenu(final IMenuManager manager)
   {
      actionPaste.setEnabled(canPaste());
      
      manager.add(actionFindReplace);
      manager.add(actionGoToLine);
      manager.add(new Separator());
      manager.add(actionCut);
      manager.add(actionCopy);
      manager.add(actionPaste);
      manager.add(actionSelectAll);
      manager.add(new Separator());
      manager.add(actionShowLineNumbers);
      manager.add(new Separator());
      manager.add(actionCompile);
      manager.add(actionSave);
   }
   
   /**
    * Check if paste action can work
    * 
    * @return
    */
   private boolean canPaste()
   {
      Clipboard cb = new Clipboard(Display.getCurrent());
      TransferData[] available = cb.getAvailableTypes();
      boolean enabled = false;
      for(int i = 0; i < available.length; i++) 
      {
         if (TextTransfer.getInstance().isSupportedType(available[i])) 
         {
            enabled = true;
            break;
         }
      }
      cb.dispose();
      return enabled;
   }
	
	/**
	 * Go to specific line
	 */
	private void goToLine()
	{
	   StyledText textControl = editor.getTextWidget();
	   final int maxLine = textControl.getLineCount();
	   
	   InputDialog dlg = new InputDialog(getSite().getShell(), "Go to Line", 
	         String.format("Enter line number (1..%d)", maxLine), 
	         Integer.toString(textControl.getLineAtOffset(textControl.getCaretOffset()) + 1), 
	         new IInputValidator() {
               @Override
               public String isValid(String newText)
               {
                  try
                  {
                     int n = Integer.parseInt(newText);
                     if ((n < 1) || (n > maxLine))
                        return "Number out of range";
                     return null;
                  }
                  catch(NumberFormatException e)
                  {
                     return "Invalid number";
                  }
               }
            });
	   if (dlg.open() != Window.OK)
	      return;
	   
	   int line = Integer.parseInt(dlg.getValue());
	   textControl.setCaretOffset(textControl.getOffsetAtLine(line - 1));
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
						actionFindReplace.update();
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
                  StyledText s = editor.getTextWidget();
                  s.setLineBackground(0, s.getLineCount(), null);
                  if (result.success)
                  {
                     editorMessageBar.showMessage(CompositeWithMessageBar.INFORMATION, "Script compiled successfully");
                  }
                  else
                  {
                     editorMessageBar.showMessage(CompositeWithMessageBar.WARNING, result.errorMessage);
                     s.setLineBackground(result.errorLine - 1, 1, ERROR_COLOR);
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
                     StyledText s = editor.getTextWidget();
                     s.setLineBackground(0, s.getLineCount(), null);
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
                     StyledText s = editor.getTextWidget();
                     s.setLineBackground(0, s.getLineCount(), null);
                     s.setLineBackground(result.errorLine - 1, 1, ERROR_COLOR);
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
		if (adapter.equals(IFindReplaceTarget.class))
		{
			return editor.getFindReplaceTarget();
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
