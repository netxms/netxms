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
package org.netxms.nxmc.modules.ai.propertypages;

import java.util.Date;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowData;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.netxms.client.NXCSession;
import org.netxms.client.ai.AiOperator;
import org.netxms.client.ai.AiOperatorCheck;
import org.netxms.client.constants.AiCheckAction;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.propertypages.PropertyPage;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.ai.dialogs.AiOperatorCheckEditDialog;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Standing Checks" property page for AI operator instance. Checks are separate server records, so every
 * change on this page is applied immediately, independently of the dialog's OK/Cancel.
 */
public class AiOperatorChecks extends PropertyPage
{
   private static final int COLUMN_ID = 0;
   private static final int COLUMN_NAME = 1;
   private static final int COLUMN_ENABLED = 2;
   private static final int COLUMN_LOCKED = 3;
   private static final int COLUMN_CREATED_BY = 4;
   private static final int COLUMN_ACTION = 5;
   private static final int COLUMN_INTERVAL = 6;
   private static final int COLUMN_OBJECT = 7;
   private static final int COLUMN_LAST_VERDICT = 8;
   private static final int COLUMN_LAST_FIRE = 9;
   private static final int COLUMN_ERRORS = 10;

   private final I18n i18n = LocalizationHelper.getI18n(AiOperatorChecks.class);

   private AiOperator operator;
   private NXCSession session = Registry.getSession();
   private SortableTableViewer viewer;
   private Button buttonEdit;
   private Button buttonDelete;
   private Button buttonLock;

   /**
    * Create page.
    *
    * @param operator operator instance to edit
    */
   public AiOperatorChecks(AiOperator operator)
   {
      super(LocalizationHelper.getI18n(AiOperatorChecks.class).tr("Standing Checks"));
      noDefaultAndApplyButton();
      this.operator = operator;
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      Composite dialogArea = new Composite(parent, SWT.NONE);

      GridLayout layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      dialogArea.setLayout(layout);

      Label label = new Label(dialogArea, SWT.WRAP);
      label.setText(i18n.tr("Standing checks are NXSL scripts run by the server on schedule without LLM involvement. Changes on this page are applied immediately."));
      GridData gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
      gd.widthHint = 600;
      label.setLayoutData(gd);

      final String[] columnNames = { i18n.tr("ID"), i18n.tr("Name"), i18n.tr("Enabled"), i18n.tr("Locked"), i18n.tr("Created by"), i18n.tr("Action"),
            i18n.tr("Interval"), i18n.tr("Object"), i18n.tr("Last verdict"), i18n.tr("Last fire"), i18n.tr("Errors") };
      final int[] columnWidths = { 50, 160, 60, 60, 80, 70, 70, 140, 90, 140, 60 };
      viewer = new SortableTableViewer(dialogArea, columnNames, columnWidths, COLUMN_ID, SWT.UP, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new CheckLabelProvider());
      viewer.setComparator(new ViewerComparator() {
         @Override
         public int compare(Viewer v, Object e1, Object e2)
         {
            AiOperatorCheck c1 = (AiOperatorCheck)e1;
            AiOperatorCheck c2 = (AiOperatorCheck)e2;
            int rc;
            switch((Integer)viewer.getTable().getSortColumn().getData("ID"))
            {
               case COLUMN_NAME:
                  rc = c1.getName().compareToIgnoreCase(c2.getName());
                  break;
               case COLUMN_LAST_FIRE:
                  rc = Long.compare(time(c1.getLastFire()), time(c2.getLastFire()));
                  break;
               case COLUMN_ERRORS:
                  rc = Integer.compare(c1.getConsecutiveErrors(), c2.getConsecutiveErrors());
                  break;
               default:
                  rc = Integer.compare(c1.getId(), c2.getId());
                  break;
            }
            return (viewer.getTable().getSortDirection() == SWT.UP) ? rc : -rc;
         }

         private long time(Date date)
         {
            return (date != null) ? date.getTime() : 0;
         }
      });
      gd = new GridData(SWT.FILL, SWT.FILL, true, true);
      gd.heightHint = 200;
      gd.widthHint = 600;
      viewer.getControl().setLayoutData(gd);

      Composite buttons = new Composite(dialogArea, SWT.NONE);
      RowLayout buttonLayout = new RowLayout();
      buttonLayout.type = SWT.HORIZONTAL;
      buttonLayout.pack = false;
      buttonLayout.marginWidth = 0;
      buttonLayout.marginRight = 0;
      buttons.setLayout(buttonLayout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.RIGHT;
      buttons.setLayoutData(gd);

      Button buttonAdd = new Button(buttons, SWT.PUSH);
      buttonAdd.setText(i18n.tr("&Add..."));
      RowData rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      buttonAdd.setLayoutData(rd);
      buttonAdd.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            addCheck();
         }
      });

      buttonEdit = new Button(buttons, SWT.PUSH);
      buttonEdit.setText(i18n.tr("&Edit..."));
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      buttonEdit.setLayoutData(rd);
      buttonEdit.setEnabled(false);
      buttonEdit.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            editCheck();
         }
      });

      buttonLock = new Button(buttons, SWT.PUSH);
      buttonLock.setText(i18n.tr("&Lock"));
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      buttonLock.setLayoutData(rd);
      buttonLock.setEnabled(false);
      buttonLock.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            toggleLock();
         }
      });

      buttonDelete = new Button(buttons, SWT.PUSH);
      buttonDelete.setText(i18n.tr("&Delete"));
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      buttonDelete.setLayoutData(rd);
      buttonDelete.setEnabled(false);
      buttonDelete.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            deleteChecks();
         }
      });

      viewer.addDoubleClickListener((e) -> editCheck());
      viewer.addSelectionChangedListener((e) -> {
         IStructuredSelection selection = viewer.getStructuredSelection();
         buttonEdit.setEnabled(selection.size() == 1);
         buttonDelete.setEnabled(!selection.isEmpty());
         buttonLock.setEnabled(selection.size() == 1);
         if (selection.size() == 1)
            buttonLock.setText(((AiOperatorCheck)selection.getFirstElement()).isLocked() ? i18n.tr("Un&lock") : i18n.tr("&Lock"));
      });

      refresh();
      return dialogArea;
   }

   /**
    * Reload check list from server
    */
   private void refresh()
   {
      new Job(i18n.tr("Loading standing checks"), null) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final List<AiOperatorCheck> checks = session.getAiOperatorChecks(operator.getId());
            runInUIThread(() -> {
               if (!viewer.getControl().isDisposed())
                  viewer.setInput(checks.toArray());
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot load standing checks");
         }
      }.start();
   }

   /**
    * Create new check
    */
   private void addCheck()
   {
      AiOperatorCheckEditDialog dlg = new AiOperatorCheckEditDialog(getShell(), new AiOperatorCheck(operator.getId(), ""));
      if (dlg.open() == Window.OK)
         saveCheck(dlg.getCheck());
   }

   /**
    * Edit selected check
    */
   private void editCheck()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      AiOperatorCheckEditDialog dlg = new AiOperatorCheckEditDialog(getShell(), (AiOperatorCheck)selection.getFirstElement());
      if (dlg.open() == Window.OK)
         saveCheck(dlg.getCheck());
   }

   /**
    * Lock or unlock selected check
    */
   private void toggleLock()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      AiOperatorCheck check = (AiOperatorCheck)selection.getFirstElement();
      check.setLocked(!check.isLocked());
      saveCheck(check);
   }

   /**
    * Save check on server and reload the list
    *
    * @param check check to save
    */
   private void saveCheck(final AiOperatorCheck check)
   {
      new Job(i18n.tr("Saving standing check"), null) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.modifyAiOperatorCheck(check);
            runInUIThread(() -> refresh());
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot save standing check");
         }
      }.start();
   }

   /**
    * Delete selected checks
    */
   private void deleteChecks()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;

      if (!MessageDialogHelper.openConfirm(getShell(), i18n.tr("Confirm Delete"), i18n.tr("Selected standing checks will be deleted. Are you sure?")))
         return;

      final Object[] objects = selection.toArray();
      new Job(i18n.tr("Deleting standing checks"), null) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(Object o : objects)
               session.deleteAiOperatorCheck(operator.getId(), ((AiOperatorCheck)o).getId());
            runInUIThread(() -> refresh());
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot delete standing check");
         }
      }.start();
   }

   /**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(boolean isApply)
   {
      return true;
   }

   /**
    * Get display text for check verdict
    *
    * @param check check
    * @return verdict text
    */
   private String getVerdictText(AiOperatorCheck check)
   {
      switch(check.getLastVerdict())
      {
         case QUIET:
            return i18n.tr("Quiet");
         case FIRED:
            return i18n.tr("Fired");
         case FAILED:
            return i18n.tr("Error");
         default:
            return i18n.tr("Never run");
      }
   }

   /**
    * Label provider for check list
    */
   private class CheckLabelProvider extends LabelProvider implements ITableLabelProvider
   {
      /**
       * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
       */
      @Override
      public Image getColumnImage(Object element, int columnIndex)
      {
         return null;
      }

      /**
       * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
       */
      @Override
      public String getColumnText(Object element, int columnIndex)
      {
         AiOperatorCheck check = (AiOperatorCheck)element;
         switch(columnIndex)
         {
            case COLUMN_ID:
               return Integer.toString(check.getId());
            case COLUMN_NAME:
               return check.getName();
            case COLUMN_ENABLED:
               return check.isEnabled() ? i18n.tr("Yes") : i18n.tr("No");
            case COLUMN_LOCKED:
               return check.isLocked() ? i18n.tr("Yes") : i18n.tr("No");
            case COLUMN_CREATED_BY:
               return check.isCreatedByModel() ? i18n.tr("Operator") : i18n.tr("User");
            case COLUMN_ACTION:
               return (check.getAction() == AiCheckAction.OBSERVE) ? i18n.tr("Observe") : i18n.tr("Wake");
            case COLUMN_INTERVAL:
               return Integer.toString(check.getInterval());
            case COLUMN_OBJECT:
               return (check.getObjectId() != 0) ? session.getObjectName(check.getObjectId()) : "";
            case COLUMN_LAST_VERDICT:
               return getVerdictText(check);
            case COLUMN_LAST_FIRE:
               return (check.getLastFire() != null) ? DateFormatFactory.getDateTimeFormat().format(check.getLastFire()) : "";
            case COLUMN_ERRORS:
               return Integer.toString(check.getConsecutiveErrors());
         }
         return null;
      }
   }
}
