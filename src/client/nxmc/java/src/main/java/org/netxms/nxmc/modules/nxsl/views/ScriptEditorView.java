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
import org.eclipse.jface.action.ToolBarManager;
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
import org.netxms.client.NXCSession;
import org.netxms.client.Script;
import org.netxms.client.ScriptCompilationResult;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.ConfigurationView;
import org.netxms.nxmc.base.widgets.MessageArea;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.nxsl.widgets.ScriptEditor;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Script editor view
 */
public class ScriptEditorView extends ConfigurationView
{
   private I18n i18n = LocalizationHelper.getI18n(ScriptEditorView.class);
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
    * @param name
    * @param image
    * @param id
    * @param hasFilter
    */
   public ScriptEditorView(long scriptId, String scriptName)
   {
      super(scriptName, ResourceManager.getImageDescriptor("icons/config-views/script-editor.png"), "ScriptEditor" + Long.toString(scriptId), false);
      this.scriptId = scriptId;
      this.scriptName = scriptName;
      this.session = Registry.getSession();
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
   }

   /**
    * @see org.netxms.nxmc.base.views.View#postContentCreate()
    */
   @Override
   protected void postContentCreate()
   {
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

      actionCompile = new Action(i18n.tr("&Compile"), ResourceManager.getImageDescriptor("icons/compile.gif")) {
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

      actionCut = new Action(i18n.tr("C&ut"), SharedIcons.CUT) {
         @Override
         public void run()
         {
            editor.getTextWidget().cut();
         }
      };
      actionCut.setEnabled(false);
      addKeyBinding("M1+X", actionCut);

      actionCopy = new Action(i18n.tr("&Copy"), SharedIcons.COPY) {
         @Override
         public void run()
         {
            editor.getTextWidget().copy();
         }
      };
      actionCopy.setEnabled(false);
      addKeyBinding("M1+C", actionCopy);

      actionPaste = new Action(i18n.tr("&Paste"), SharedIcons.PASTE) {
         @Override
         public void run()
         {
            editor.getTextWidget().paste();
         }
      };
      actionPaste.setEnabled(canPaste());
      addKeyBinding("M1+V", actionPaste);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolbar(org.eclipse.jface.action.ToolBarManager)
    */
   @Override
   protected void fillLocalToolbar(ToolBarManager manager)
   {
      super.fillLocalToolbar(manager);
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
                           String.format(i18n.tr("Script compilation failed (%s)\r\nSave changes anyway?"), result.errorMessage)))
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
      saveScript();
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
}
