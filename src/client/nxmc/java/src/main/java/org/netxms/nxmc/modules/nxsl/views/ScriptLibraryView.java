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

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.NXCSession;
import org.netxms.client.Script;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.ConfigurationView;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.nxsl.dialogs.CreateScriptDialog;
import org.netxms.nxmc.modules.nxsl.views.helpers.ScriptComparator;
import org.netxms.nxmc.modules.nxsl.views.helpers.ScriptFilter;
import org.netxms.nxmc.modules.nxsl.views.helpers.ScriptLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Script library view
 */
public class ScriptLibraryView extends ConfigurationView
{
   private final I18n i18n = LocalizationHelper.getI18n(ScriptLibraryView.class);

   public static final int COLUMN_ID = 0;
   public static final int COLUMN_NAME = 1;

   private static final String ID = "configuration.script-library";

   private NXCSession session;
   private SortableTableViewer viewer;
   private Action actionNew;
   private Action actionEdit;
   private Action actionRename;
   private Action actionDelete;
   private Action actionCopyName;

   /**
    * Create script library view
    */
   public ScriptLibraryView()
   {
      super(LocalizationHelper.getI18n(ScriptLibraryView.class).tr("Script Library"), ResourceManager.getImageDescriptor("icons/config-views/script_library.png"), ID, true);
      session = Registry.getSession();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      final String[] names = { i18n.tr("ID"), i18n.tr("Name") };
      final int[] widths = { 90, 500 };
      viewer = new SortableTableViewer(parent, names, widths, 0, SWT.UP, SortableTableViewer.DEFAULT_STYLE);
      WidgetHelper.restoreTableViewerSettings(viewer, ID);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new ScriptLabelProvider());
      viewer.setComparator(new ScriptComparator());
      ScriptFilter filter = new ScriptFilter();
      setFilterClient(viewer, filter);
      viewer.addFilter(filter);
      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            IStructuredSelection selection = (IStructuredSelection)event.getSelection();
            if (selection != null)
            {
               actionEdit.setEnabled(selection.size() == 1);
               actionRename.setEnabled(selection.size() == 1);
               actionDelete.setEnabled(selection.size() > 0);
               actionCopyName.setEnabled(selection.size() == 1);
            }
         }
      });
      viewer.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            editScript();
         }
      });
      viewer.getTable().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveTableViewerSettings(viewer, ID);
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
    * @see org.netxms.nxmc.base.views.ConfigurationView#isModified()
    */
   @Override
   public boolean isModified()
   {
      return false;
   }

   /**
    * @see org.netxms.nxmc.base.views.ConfigurationView#save()
    */
   @Override
   public void save()
   {
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionNew = new Action(i18n.tr("Create &new script..."), SharedIcons.ADD_OBJECT) {
         @Override
         public void run()
         {
            createNewScript();
         }
      };
      addKeyBinding("M1+N", actionNew);

      actionEdit = new Action(i18n.tr("&Edit"), SharedIcons.EDIT) {
         @Override
         public void run()
         {
            editScript();
         }
      };
      actionEdit.setEnabled(false);

      actionRename = new Action(i18n.tr("&Rename...")) {
         @Override
         public void run()
         {
            renameScript();
         }
      };
      actionRename.setEnabled(false);
      addKeyBinding("F2", actionRename);

      actionDelete = new Action(i18n.tr("&Delete"), SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            deleteScript();
         }
      };
      actionDelete.setEnabled(false);
      addKeyBinding("M1+D", actionDelete);

      actionCopyName = new Action(i18n.tr("&Copy name to clipboard"), SharedIcons.COPY) {
         @Override
         public void run()
         {
            copyNameToClipboard();
         }
      };
      addKeyBinding("M1+C", actionCopyName);
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
      Menu menu = menuMgr.createContextMenu(viewer.getTable());
      viewer.getTable().setMenu(menu);
   }

   /**
    * Fill context menu
    * 
    * @param mgr Menu manager
    */
   protected void fillContextMenu(final IMenuManager mgr)
   {
      mgr.add(actionNew);
      mgr.add(new Separator());
      mgr.add(actionEdit);
      mgr.add(actionRename);
      mgr.add(actionDelete);
      mgr.add(new Separator());
      mgr.add(actionCopyName);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      super.fillLocalToolBar(manager);
      manager.add(actionNew);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      new Job(i18n.tr("Loading library script list"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final List<Script> library = session.getScriptLibrary();
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  viewer.setInput(library.toArray());
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot load list of library scripts");
         }
      }.start();
   }

   /**
    * Create new script
    */
   private void createNewScript()
   {
      final CreateScriptDialog dlg = new CreateScriptDialog(getWindow().getShell(), null);
      if (dlg.open() != Window.OK)
         return;

      new Job(i18n.tr("Creating new script"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final long id = session.modifyScript(0, dlg.getName(), ""); //$NON-NLS-1$
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  Object[] input = (Object[])viewer.getInput();
                  List<Script> list = new ArrayList<Script>(input.length);
                  for(Object o : input)
                     list.add((Script)o);
                  final Script script = new Script(id, dlg.getName(), ""); //$NON-NLS-1$
                  list.add(script);
                  viewer.setInput(list.toArray());
                  viewer.setSelection(new StructuredSelection(script));
                  editScript();
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot create new script in library");
         }
      }.start();
   }

   /**
    * Edit script
    */
   private void editScript()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      Script script = (Script)selection.getFirstElement();
      openView(new ScriptEditorView(script.getId(), script.getName()));
   }

   /**
    * Edit script
    */
   private void renameScript()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      final Script script = (Script)selection.getFirstElement();
      final CreateScriptDialog dlg = new CreateScriptDialog(getWindow().getShell(), script.getName());
      if (dlg.open() == Window.OK)
      {
         new Job(i18n.tr("Rename library script"), this) {
            @Override
            protected void run(IProgressMonitor monitor) throws Exception
            {
               session.renameScript(script.getId(), dlg.getName());
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     Object[] input = (Object[])viewer.getInput();
                     List<Script> list = new ArrayList<Script>(input.length);
                     for(Object o : input)
                     {
                        if (((Script)o).getId() != script.getId())
                           list.add((Script)o);
                     }
                     final Script newScript = new Script(script.getId(), dlg.getName(), script.getSource());
                     list.add(newScript);
                     viewer.setInput(list.toArray());
                     viewer.setSelection(new StructuredSelection(newScript));
                  }
               });
            }

            @Override
            protected String getErrorMessage()
            {
               return i18n.tr("Cannot rename library script");
            }
         }.start();
      }
   }

   /**
    * Delete selected script(s)
    */
   @SuppressWarnings("rawtypes")
   private void deleteScript()
   {
      final IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;

      if (!MessageDialogHelper.openQuestion(getWindow().getShell(), i18n.tr("Confirm Delete"),
            i18n.tr("Do you really want to delete selected scripts?")))
         return;

      new Job(i18n.tr("Delete scripts from library"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            Iterator it = selection.iterator();
            while(it.hasNext())
            {
               Script script = (Script)it.next();
               session.deleteScript(script.getId());
            }
         }

         @Override
         protected void jobFinalize()
         {
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  refresh();
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot delete script from library");
         }
      }.start();
   }

   /**
    * Copy script name to clipboard
    */
   private void copyNameToClipboard()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      WidgetHelper.copyToClipboard(((Script)selection.getFirstElement()).getName());
   }
}
