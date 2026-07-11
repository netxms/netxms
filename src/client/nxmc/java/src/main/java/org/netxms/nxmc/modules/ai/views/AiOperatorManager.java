/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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
package org.netxms.nxmc.modules.ai.views;

import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.NXCSession;
import org.netxms.client.ai.AiOperator;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.ConfigurationView;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.ai.dialogs.AiOperatorEditDialog;
import org.netxms.nxmc.modules.ai.views.helpers.AiOperatorComparator;
import org.netxms.nxmc.modules.ai.views.helpers.AiOperatorFilter;
import org.netxms.nxmc.modules.ai.views.helpers.AiOperatorLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.xnap.commons.i18n.I18n;

/**
 * AI operator instance management view
 */
public class AiOperatorManager extends ConfigurationView
{
   private final I18n i18n = LocalizationHelper.getI18n(AiOperatorManager.class);

   public static final int COLUMN_ID = 0;
   public static final int COLUMN_NAME = 1;
   public static final int COLUMN_DESCRIPTION = 2;
   public static final int COLUMN_STATE = 3;
   public static final int COLUMN_OWNER = 4;
   public static final int COLUMN_MODEL_SLOT = 5;
   public static final int COLUMN_MIN_INTERVAL = 6;
   public static final int COLUMN_MAX_INTERVAL = 7;
   public static final int COLUMN_TOKEN_BUDGET = 8;
   public static final int COLUMN_TOKENS_USED = 9;
   public static final int COLUMN_ITERATION = 10;
   public static final int COLUMN_LAST_EXECUTION = 11;
   public static final int COLUMN_NEXT_EXECUTION = 12;
   public static final int COLUMN_FOCUS = 13;
   public static final int COLUMN_EXPLANATION = 14;

   private SortableTableViewer viewer;
   private NXCSession session;
   private Action actionNew;
   private Action actionEdit;
   private Action actionDelete;
   private Action actionEnable;
   private Action actionDisable;
   private Action actionResetState;

   /**
    * Create AI operator manager view
    */
   public AiOperatorManager()
   {
      super(LocalizationHelper.getI18n(AiOperatorManager.class).tr("AI Operators"), ResourceManager.getImageDescriptor("icons/config-views/ai-operators.png"), "configuration.ai-operators", true);
      session = Registry.getSession();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      final String[] columnNames = { i18n.tr("ID"), i18n.tr("Name"), i18n.tr("Description"), i18n.tr("State"), i18n.tr("Owner"),
            i18n.tr("Model slot"), i18n.tr("Min interval"), i18n.tr("Max interval"), i18n.tr("Token budget"), i18n.tr("Tokens used today"),
            i18n.tr("Iteration"), i18n.tr("Last execution"), i18n.tr("Next execution"), i18n.tr("Current focus"), i18n.tr("Explanation") };
      final int[] columnWidths = { 60, 200, 300, 80, 120, 120, 90, 90, 100, 120, 80, 150, 150, 300, 400 };
      viewer = new SortableTableViewer(parent, columnNames, columnWidths, COLUMN_ID, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI, "AiOperatorManager");
      viewer.setContentProvider(new ArrayContentProvider());
      AiOperatorLabelProvider labelProvider = new AiOperatorLabelProvider(viewer);
      viewer.setLabelProvider(labelProvider);
      viewer.setComparator(new AiOperatorComparator());
      AiOperatorFilter filter = new AiOperatorFilter(labelProvider);
      setFilterClient(viewer, filter);
      viewer.addFilter(filter);
      viewer.addSelectionChangedListener((event) -> {
         IStructuredSelection selection = event.getStructuredSelection();
         actionEdit.setEnabled(selection.size() == 1);
         actionDelete.setEnabled(!selection.isEmpty());
         actionEnable.setEnabled(!selection.isEmpty());
         actionDisable.setEnabled(!selection.isEmpty());
         actionResetState.setEnabled(!selection.isEmpty());
      });
      viewer.addDoubleClickListener((event) -> editOperator());
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
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      new Job(i18n.tr("Loading AI operator instances"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final List<AiOperator> operators = session.getAiOperators();
            runInUIThread(() -> viewer.setInput(operators.toArray()));
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot get list of AI operator instances");
         }
      }.start();
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionNew = new Action(i18n.tr("&New..."), SharedIcons.ADD_OBJECT) {
         @Override
         public void run()
         {
            createOperator();
         }
      };
      addKeyBinding("M1+N", actionNew);

      actionEdit = new Action(i18n.tr("&Edit..."), SharedIcons.EDIT) {
         @Override
         public void run()
         {
            editOperator();
         }
      };
      actionEdit.setEnabled(false);

      actionDelete = new Action(i18n.tr("&Delete"), SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            deleteOperators();
         }
      };
      actionDelete.setEnabled(false);
      addKeyBinding("M1+D", actionDelete);

      actionEnable = new Action(i18n.tr("En&able")) {
         @Override
         public void run()
         {
            setOperatorsEnabled(true);
         }
      };
      actionEnable.setEnabled(false);

      actionDisable = new Action(i18n.tr("D&isable")) {
         @Override
         public void run()
         {
            setOperatorsEnabled(false);
         }
      };
      actionDisable.setEnabled(false);

      actionResetState = new Action(i18n.tr("&Reset accumulated state")) {
         @Override
         public void run()
         {
            resetOperators();
         }
      };
      actionResetState.setEnabled(false);
   }

   /**
    * Create context menu
    */
   private void createContextMenu()
   {
      MenuManager menuMgr = new MenuManager();
      menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener((m) -> fillContextMenu(m));

      Menu menu = menuMgr.createContextMenu(viewer.getControl());
      viewer.getControl().setMenu(menu);
   }

   /**
    * Fill context menu
    *
    * @param manager Menu manager
    */
   protected void fillContextMenu(final IMenuManager manager)
   {
      manager.add(actionNew);
      manager.add(actionEdit);
      manager.add(actionDelete);
      manager.add(new Separator());
      manager.add(actionEnable);
      manager.add(actionDisable);
      manager.add(actionResetState);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionNew);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      Action resetAction = viewer.getResetColumnOrderAction();
      if (resetAction != null)
         manager.add(resetAction);
      Action showAllAction = viewer.getShowAllColumnsAction();
      if (showAllAction != null)
         manager.add(showAllAction);
      Action autoSizeAction = viewer.getAutoSizeColumnsAction();
      if (autoSizeAction != null)
      {
         manager.add(new Separator());
         manager.add(autoSizeAction);
      }
      manager.add(actionNew);
   }

   /**
    * Create new operator instance
    */
   private void createOperator()
   {
      final AiOperatorEditDialog dlg = new AiOperatorEditDialog(getWindow().getShell(), null);
      if (dlg.open() != Window.OK)
         return;

      saveOperator(dlg.getOperator());
   }

   /**
    * Edit selected operator instance
    */
   private void editOperator()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      final AiOperatorEditDialog dlg = new AiOperatorEditDialog(getWindow().getShell(), (AiOperator)selection.getFirstElement());
      if (dlg.open() != Window.OK)
         return;

      saveOperator(dlg.getOperator());
   }

   /**
    * Save operator instance on server.
    *
    * @param operator operator instance to save
    */
   private void saveOperator(final AiOperator operator)
   {
      new Job(i18n.tr("Saving AI operator instance"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.modifyAiOperator(operator);
            runInUIThread(() -> refresh());
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot save AI operator instance");
         }
      }.start();
   }

   /**
    * Delete selected operator instances
    */
   private void deleteOperators()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;

      if (!MessageDialogHelper.openConfirm(getWindow().getShell(), i18n.tr("Confirm Delete"),
            i18n.tr("Selected AI operator instances and all their observations will be deleted. Are you sure?")))
         return;

      final Object[] objects = selection.toArray();
      new Job(i18n.tr("Deleting AI operator instances"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(Object o : objects)
               session.deleteAiOperator(((AiOperator)o).getId());
            runInUIThread(() -> refresh());
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot delete AI operator instance");
         }
      }.start();
   }

   /**
    * Enable or disable selected operator instances.
    *
    * @param enabled true to enable
    */
   private void setOperatorsEnabled(final boolean enabled)
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;

      final Object[] objects = selection.toArray();
      new Job(enabled ? i18n.tr("Enabling AI operator instances") : i18n.tr("Disabling AI operator instances"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(Object o : objects)
            {
               AiOperator operator = (AiOperator)o;
               operator.setEnabled(enabled);
               session.modifyAiOperator(operator);
            }
            runInUIThread(() -> refresh());
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot change AI operator instance state");
         }
      }.start();
   }

   /**
    * Reset accumulated state of selected operator instances
    */
   private void resetOperators()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;

      if (!MessageDialogHelper.openConfirm(getWindow().getShell(), i18n.tr("Confirm Reset"),
            i18n.tr("Accumulated state (memento, current focus, watch list, and iteration counter) of selected AI operator instances will be cleared. Are you sure?")))
         return;

      final Object[] objects = selection.toArray();
      new Job(i18n.tr("Resetting AI operator instances"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(Object o : objects)
               session.resetAiOperator(((AiOperator)o).getId());
            runInUIThread(() -> refresh());
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot reset AI operator instance");
         }
      }.start();
   }
}
