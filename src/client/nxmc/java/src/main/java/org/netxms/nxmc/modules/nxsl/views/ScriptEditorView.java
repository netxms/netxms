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

import java.util.HashMap;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.NXCSession;
import org.netxms.client.Script;
import org.netxms.client.ScriptCompilationResult;
import org.netxms.client.ScriptCompilationWarning;
import org.netxms.nxmc.Memento;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.ConfigurationView;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.views.ViewNotRestoredException;
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
   private final I18n i18n = LocalizationHelper.getI18n(ScriptEditorView.class);

   private static final Map<String, String> hintMap = new HashMap<String, String>() {
      private static final long serialVersionUID = 1L;
      {
         put("Hook::StatusPoll",
               "Available global variables:\n" +
               "   $object - current object, one of 'NetObj' subclasses.\n" +
               "   $node - current object if it is a 'Node' class. Not set for other object types.\n" +
               " \n" +
               "Expected return value:\n" +
               "   none - return value is ignored.\n");
         put("Hook::ConfigurationPoll",
               "Available global variables:\n" +
               "   $object - current object, one of the 'NetObj' subclasses\n" +
               "   $node - current object if it is a 'Node' class\n" +
               " \n" +
               "Expected return value:\n" +
               "   none - return value is ignored\n");
         put("Hook::InstancePoll",
               "Available global variables:\n" +
               "   $object - current object, one of the 'NetObj' subclasses\n" +
               "   $node - current object if it is a 'Node' class\n" +
               " \n" +
               "Expected return value:\n" +
               "   none - return value is ignored\n");
         put("Hook::TopologyPoll",
               "Available global variables:\n" +
               "   $node - current node, an object of the 'Node' type\n" +
               " \n" +
               "Expected return value:\n" +
               "   none - return value is ignored\n");
         put("Hook::CreateInterface",
               "Available global variables:\n" +
               "   $node - current node, an object of the 'Node' type\n" +
               "   $1 - current interface, an object of the 'Interface' type\n" +
               " \n" +
               "Expected return value:\n" +
               "   true/false - boolean - whether the interface should be created\n");
         put("Hook::AcceptNewNode",
               "Available global variables:\n" +
               "   $ipAddress - IP address of the node being processed, an object of the 'InetAddress' class\n" +
               "   $ipAddr - IP address of the node being processed as text\n" +
               "   $ipNetMask - netmask of the node being processed as an integer\n" +
               "   $macAddress - MAC address, an object of the 'MacAddress' class\n" +
               "   $macAddr - MAC address of the node being processed\n" +
               "   $zoneId - zone ID of the node being processed\n" +
               " \n" +
               "Expected return value:\n" +
               "   true/false - boolean - whether node processing should continue\n");
         put("Hook::DiscoveryPoll",
               "Available global variables:\n" +
               "   $node - current node, an object of the 'Node' type\n" +
               " \n" +
               "Expected return value:\n" +
               "   none - return value is ignored\n");
         put("Hook::PostObjectCreate",
               "Available global variables:\n" +
               "   $object - current object, one of the 'NetObj' subclasses\n" +
               "   $node - current object if it is a 'Node' class\n" +
               " \n" +
               "Expected return value:\n" +
               "   none - return value is ignored\n");
         put("Hook::CreateSubnet",
               "Available global variables:\n" +
               "   $node - current node, an object of the 'Node' class\n" +
               "   $1 - current subnet, an object of the 'Subnet' class\n" +
               " \n" +
               "Expected return value:\n" +
               "   true/false - boolean - whether the subnet should be created\n");
         put("Hook::UpdateInterface",
               "Available global variables:\n" +
               "   $node - current node, an object of the 'Node' class\n" +
               "   $interface - current interface, an object of the 'Interface' class\n" +
               " \n" +
               "Expected return value:\n" +
               "   none - return value is ignored\n");
         put("Hook::EventProcessor",
               "Available global variables:\n" +
               "   $object - event source object, one of the 'NetObj' subclasses\n" +
               "   $node - event source object if it is a 'Node' class\n" +
               "   $event - event being processed (an object of the 'Event' class)\n" +
               " \n" +
               "Expected return value:\n" +
               "   none - return value is ignored\n");
         put("Hook::AlarmStateChange",
               "Available global variables:\n" +
               "   $alarm - alarm being processed (an object of the 'Alarm' class)\n" +
               "\n" +
               "Expected return value:\n" +
               "   none - return value is ignored\n");
         put("Hook::OpenUnboundTunnel",
               "Available global variables:\n" +
               "   $tunnel - incoming tunnel information (an object of the 'Tunnel' class)\n" +
               " \n" +
               "Expected return value:\n" +
               "   none - return value is ignored\n");
         put("Hook::OpenBoundTunnel",
               "Available global variables:\n" +
               "   $node - node to which this tunnel was bound (an object of the 'Node' class)\n" +
               "   $tunnel - incoming tunnel information (an object of the 'Tunnel' class)\n" +
               " \n" +
               "Expected return value:\n" +
               "   none - return value is ignored\n");
         put("Hook::LDAPSynchronization",
               "Available global variables:\n" +
               "   $ldapObject - LDAP object being synchronized (an object of the 'LDAPObject' class)\n" +
               " \n" +
               "Expected return value:\n" +
               "   true/false - boolean - whether the processing of this LDAP object should continue\n");
         put("Hook::Login",
               "Available global variables:\n" +
               "   $user - user object (an object of the 'User' class)\n" +
               "   $session - session object (an object of the 'ClientSession' class)\n" +
               " \n" +
               "Expected return value:\n" +
               "   true/false - boolean - whether the login for this session should continue\n");
         put("Hook::RegisterForConfigurationBackup",
               "Available global variables:\n" +
               "   $node - node to be tested (an object of the 'Node' class)\n" +
               " \n" +
               "Expected return value:\n" +
               "   true/false - boolean - whether this node should be registered for configuration backup\n");
      }
   };

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
   private String savedScript = null;

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
      super(null, ResourceManager.getImageDescriptor("icons/config-views/script-editor.png"), null, false);
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
      String hint = hintMap.get(scriptName);

      editor = new ScriptEditor(parent, SWT.NONE, SWT.H_SCROLL | SWT.V_SCROLL, hint, false) {
         @Override
         protected void fillContextMenu(IMenuManager manager)
         {
            super.fillContextMenu(manager);
            manager.add(new Separator());
            manager.add(actionCompile);
         }
      };
      editor.showLineNumbers(showLineNumbers);
      editor.getTextWidget().addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            if (!modified)
            {
               modified = true;
               actionSave.setEnabled(true);
            }
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
      super.postContentCreate();
      if (savedScript != null)
      {
         modified = true;
         actionSave.setEnabled(true);
         editor.setText(savedScript);
         savedScript = null;
      }
      else
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

      actionGoToLine = new Action(i18n.tr("&Go to line..."), ResourceManager.getImageDescriptor("icons/nxsl/go-to-line.png")) {
         @Override
         public void run()
         {
            editor.goToLine();
         }
      };
      addKeyBinding("M1+G", actionGoToLine);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionGoToLine);
      manager.add(new Separator());
      manager.add(actionShowLineNumbers);
      manager.add(new Separator());
      manager.add(actionCompile);
      manager.add(actionSave);
      super.fillLocalMenu(manager);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionGoToLine);
      manager.add(actionCompile);
      manager.add(actionSave);
      super.fillLocalToolBar(manager);
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

   /**
    * @see org.netxms.nxmc.base.views.View#saveState(org.netxms.nxmc.Memento)
    */
   @Override
   public void saveState(Memento memento)
   {
      super.saveState(memento);
      memento.set("scriptId", scriptId);
      memento.set("scriptName", scriptName);
      if (modified)
      {
         memento.set("savedScript", editor.getText());
      }
   }

   /**
    * @throws ViewNotRestoredException
    * @see org.netxms.nxmc.base.views.ViewWithContext#restoreState(org.netxms.nxmc.Memento)
    */
   @Override
   public void restoreState(Memento memento) throws ViewNotRestoredException
   {
      super.restoreState(memento);
      scriptId = memento.getAsLong("scriptId", 0);
      scriptName = memento.getAsString("scriptName");
      savedScript  = memento.getAsString("savedScript", null);
      setName(scriptName);
   }
}
