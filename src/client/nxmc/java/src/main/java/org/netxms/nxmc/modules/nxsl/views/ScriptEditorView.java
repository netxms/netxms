/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
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

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.dialogs.IInputValidator;
import org.eclipse.jface.dialogs.InputDialog;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.dnd.Clipboard;
import org.eclipse.swt.dnd.TextTransfer;
import org.eclipse.swt.dnd.TransferData;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.NXCSession;
import org.netxms.client.Script;
import org.netxms.client.ScriptCompilationResult;
import org.netxms.client.ScriptCompilationWarning;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.ConfigurationView;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.widgets.MessageArea;
import org.netxms.nxmc.keyboard.KeyStroke;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.nxsl.widgets.ScriptEditor;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Script editor view
 */
public class ScriptEditorView extends ConfigurationView
{
   private final I18n i18n = LocalizationHelper.getI18n(ScriptEditorView.class);

   private NXCSession session;
   private ScriptEditor editor;
   private long scriptId;
   private String scriptName;
   private boolean modified = false;
   private boolean showLineNumbers = true;
   private Action actionSave;
   private Action actionCompile;
   private Action actionShowLineNumbers;
   private Action actionGoToLine;
   private Action actionSelectAll;
   private Action actionCut;
   private Action actionCopy;
   private Action actionPaste;

   /**
    * Create new script editor view.
    *
    * @param scriptId script ID
    * @param scriptName script name
    */
   public ScriptEditorView(long scriptId, String scriptName)
   {
      super(scriptName, ResourceManager.getImageDescriptor("icons/config-views/script-editor.png"), "configuration.script-editor." + Long.toString(scriptId), false);
      this.scriptId = scriptId;
      this.scriptName = scriptName;
      this.session = Registry.getSession();
   }

   /**
    * Clone constructor
    *
    * @param scriptId script ID
    * @param scriptName script name
    */
   public ScriptEditorView()
   {
      super(null, null, null, false);
      this.scriptId = 0;
      this.scriptName = "";
      this.session = Registry.getSession();
   }
   
   

   /**
    * @see org.netxms.nxmc.base.views.View#cloneView()
    */
   @Override
   public View cloneView()
   {
      ScriptEditorView view = (ScriptEditorView)super.cloneView();
      view.scriptId = scriptId;
      view.scriptName = scriptName;
      return view;
   }
   
   /**
    * @see org.netxms.nxmc.base.views.View#postClone(org.netxms.nxmc.base.views.View)
    */
   @Override
   protected void postClone(View origin)
   {
      //Do not call super post clone (default action for ConfigurationView is to call refresh)
      ScriptEditorView view = (ScriptEditorView)origin;
      editor.setText(view.editor.getText());
      modified = view.modified;
      actionSave.setEnabled(modified);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      editor = new ScriptEditor(parent, SWT.NONE, SWT.H_SCROLL | SWT.V_SCROLL, showLineNumbers, false);
      editor.getTextWidget().addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            if (!modified)
            {
               modified = true;
               actionSave.setEnabled(true);

               boolean selected = editor.getTextWidget().getSelectionCount() > 0;
               actionCut.setEnabled(selected);
               actionCopy.setEnabled(selected);
            }
         }
      });
      editor.getTextWidget().addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            boolean selected = editor.getTextWidget().getSelectionCount() > 0;
            actionCut.setEnabled(selected);
            actionCopy.setEnabled(selected);
         }
      });

      createActions();
      createContextMenu();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#postContentCreate()
    */
   @Override
   protected void postContentCreate()
   {
      super.postContentCreate();
      refresh();
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
            saveScript();
         }
      };
      addKeyBinding("M1+S", actionSave);

      actionCompile = new Action(i18n.tr("&Compile"), ResourceManager.getImageDescriptor("icons/compile.png")) {
         @Override
         public void run()
         {
            editor.compileScript();
         }
      };
      addKeyBinding("F2", actionCompile);

      actionShowLineNumbers = new Action(i18n.tr("Show line &numbers"), Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            showLineNumbers = actionShowLineNumbers.isChecked();
            editor.showLineNumbers(showLineNumbers);
         }
      };
      actionShowLineNumbers.setChecked(showLineNumbers);
      addKeyBinding("M1+M3+L", actionShowLineNumbers);

      actionGoToLine = new Action(i18n.tr("&Go to line...")) {
         @Override
         public void run()
         {
            goToLine();
         }
      };
      addKeyBinding("M1+G", actionGoToLine);

      actionSelectAll = new Action(i18n.tr("Select &all")) {
         @Override
         public void run()
         {
            editor.getTextWidget().selectAll();
         }
      };
      addKeyBinding("M1+A", actionSelectAll);
      
      //Do not require key bindings (already part of Styles text)
      actionCut = new Action(i18n.tr("C&ut") + "\t" + KeyStroke.parse("M1+X").toString(), SharedIcons.CUT) {
         @Override
         public void run()
         {
            editor.getTextWidget().cut();
         }
      };
      actionCut.setEnabled(false);
      addKeyBinding("M1+X", actionCut);

      actionCopy = new Action(i18n.tr("&Copy") + "\t" + KeyStroke.parse("M1+C").toString(), SharedIcons.COPY) {
         @Override
         public void run()
         {
            editor.getTextWidget().copy();
         }
      };
      actionCopy.setEnabled(false);

      actionPaste = new Action(i18n.tr("&Paste") + "\t" + KeyStroke.parse("M1+P").toString(), SharedIcons.PASTE) {
         @Override
         public void run()
         {
            editor.getTextWidget().paste();
         }
      };
      actionPaste.setEnabled(canPaste());
   }

   /**
    * Create context menu
    */
   private void createContextMenu()
   {
      // Create menu manager.
      MenuManager menuMgr = new MenuManager();
      menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener(new IMenuListener() {
         public void menuAboutToShow(IMenuManager mgr)
         {
            fillContextMenu(mgr);
         }
      });

      // Create menu.
      Menu menu = menuMgr.createContextMenu(editor.getTextWidget());
      editor.getTextWidget().setMenu(menu);
   }

   /**
    * Fill context menu
    * 
    * @param mgr Menu manager
    */
   protected void fillContextMenu(final IMenuManager mgr)
   {
      mgr.add(actionCopy);
      mgr.add(actionCut);
      mgr.add(actionPaste);
      mgr.add(new Separator());      
      mgr.add(actionSelectAll);
      mgr.add(new Separator());
      mgr.add(actionCompile);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      super.fillLocalToolBar(manager);
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
      TransferData[] available = WidgetHelper.getAvailableTypes(cb);
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
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      new Job(i18n.tr("Loading script"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final Script script = session.getScript(scriptId);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  scriptName = script.getName();
                  setName(scriptName);
                  editor.setText(script.getSource());
                  actionSave.setEnabled(false);
                  modified = false;
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot load script");
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
      new Job(i18n.tr("Save script"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final ScriptCompilationResult result = session.compileScript(source, false);
            if (result.success)
            {
               getDisplay().syncExec(new Runnable() {
                  @Override
                  public void run()
                  {
                     editor.clearMessages();
                     editor.clearHighlighting();
                  }
               });
            }
            else
            {
               getDisplay().syncExec(new Runnable() {
                  @Override
                  public void run()
                  {
                     if (MessageDialogHelper.openQuestion(getWindow().getShell(), i18n.tr("Compilation Errors"),
                           String.format(i18n.tr("Script compilation failed (%s)\nSave changes anyway?"), result.errorMessage)))
                        result.success = true;

                     clearMessages();
                     addMessage(MessageArea.ERROR, result.errorMessage, true);
                     editor.highlightErrorLine(result.errorLine);
                  }
               });
            }
            if (result.success)
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

                     for(ScriptCompilationWarning w : result.warnings)
                     {
                        addMessage(MessageArea.WARNING, i18n.tr("Warning in line {0}: {1}", Integer.toString(w.lineNumber), w.message), true);
                        if (w.lineNumber != result.errorLine)
                           editor.highlightWarningLine(w.lineNumber);
                     }

                     addMessage(MessageArea.SUCCESS, "Save successful", false);
                  }
               });
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

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot save script");
         }
      }.start();
   }

   /**
    * @see org.netxms.nxmc.base.views.ConfigurationView#isModified()
    */
   @Override
   public boolean isModified()
   {
      return modified;
   }

   /**
    * @see org.netxms.nxmc.base.views.ConfigurationView#save()
    */
   @Override
   public void save()
   {
      // This method normally called only whan save is requested on view closure,
      // so we should skip compilation phase and only save script as is
      final String source = editor.getText();
      new Job(i18n.tr("Save script"), null) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.modifyScript(scriptId, scriptName, source);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot save script");
         }
      }.start();
   }

   /**
    * Go to specific line
    */
   private void goToLine()
   {
      final int maxLine = editor.getLineCount();
      InputDialog dlg = new InputDialog(getWindow().getShell(), i18n.tr("Go to Line"), String.format(i18n.tr("Enter line number (1..%d)"), maxLine),
            Integer.toString(editor.getCurrentLine()), new IInputValidator() {
               @Override
               public String isValid(String newText)
               {
                  try
                  {
                     int n = Integer.parseInt(newText);
                     if ((n < 1) || (n > maxLine))
                        return i18n.tr("Number out of range");
                     return null;
                  }
                  catch(NumberFormatException e)
                  {
                     return i18n.tr("Invalid number");
                  }
               }
            });
      if (dlg.open() != Window.OK)
         return;

      editor.setCaretToLine(Integer.parseInt(dlg.getValue()));
   }

   /**
    * @see org.netxms.nxmc.base.views.View#setFocus()
    */
   @Override
   public void setFocus()
   {
      if (!editor.isDisposed())
         editor.setFocus();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#isCloseable()
    */
   @Override
   public boolean isCloseable()
   {
      return true;
   }
}
