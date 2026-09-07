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

import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
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
import org.netxms.client.ai.AiOperatorInstructionsHistoryRecord;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.propertypages.PropertyPage;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Standing Instructions" property page for AI operator instance: instructions text written by the operator
 * itself, lock flag, and change history.
 */
public class AiOperatorInstructions extends PropertyPage
{
   private static final int COLUMN_TIMESTAMP = 0;
   private static final int COLUMN_ITERATION = 1;
   private static final int COLUMN_TEXT = 2;

   private final I18n i18n = LocalizationHelper.getI18n(AiOperatorInstructions.class);

   private AiOperator operator;
   private NXCSession session = Registry.getSession();
   private LabeledText textInstructions;
   private Button checkLocked;
   private SortableTableViewer historyViewer;
   private Button buttonRestore;

   /**
    * Create page.
    *
    * @param operator operator instance to edit
    */
   public AiOperatorInstructions(AiOperator operator)
   {
      super(LocalizationHelper.getI18n(AiOperatorInstructions.class).tr("Standing Instructions"));
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
      layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
      dialogArea.setLayout(layout);

      textInstructions = new LabeledText(dialogArea, SWT.NONE, SWT.BORDER | SWT.MULTI | SWT.V_SCROLL | SWT.WRAP);
      textInstructions.setLabel(i18n.tr("Standing instructions (written by the operator itself; injected after the persona prompt)"));
      textInstructions.setText(operator.getInstructions());
      GridData gd = new GridData(SWT.FILL, SWT.FILL, true, true);
      gd.heightHint = 150;
      gd.widthHint = 500;
      textInstructions.setLayoutData(gd);

      checkLocked = new Button(dialogArea, SWT.CHECK);
      checkLocked.setText(i18n.tr("&Locked (updates returned by the operator are ignored)"));
      checkLocked.setSelection(operator.isInstructionsLocked());

      if (operator.getId() != 0)
      {
         Label label = new Label(dialogArea, SWT.NONE);
         label.setText(i18n.tr("History (previous versions, most recent first)"));
         gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
         gd.verticalIndent = WidgetHelper.OUTER_SPACING;
         label.setLayoutData(gd);

         final String[] columnNames = { i18n.tr("Changed"), i18n.tr("Iteration"), i18n.tr("Previous text") };
         final int[] columnWidths = { 150, 70, 300 };
         historyViewer = new SortableTableViewer(dialogArea, columnNames, columnWidths, COLUMN_TIMESTAMP, SWT.DOWN, SWT.BORDER | SWT.FULL_SELECTION);
         historyViewer.setContentProvider(new ArrayContentProvider());
         historyViewer.setLabelProvider(new HistoryLabelProvider());
         historyViewer.setComparator(new ViewerComparator() {
            @Override
            public int compare(Viewer viewer, Object e1, Object e2)
            {
               int rc = Long.compare(((AiOperatorInstructionsHistoryRecord)e1).getId(), ((AiOperatorInstructionsHistoryRecord)e2).getId());
               return (historyViewer.getTable().getSortDirection() == SWT.UP) ? rc : -rc;
            }
         });
         gd = new GridData(SWT.FILL, SWT.FILL, true, true);
         gd.heightHint = 120;
         historyViewer.getControl().setLayoutData(gd);

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

         Button buttonView = new Button(buttons, SWT.PUSH);
         buttonView.setText(i18n.tr("&View..."));
         RowData rd = new RowData();
         rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
         buttonView.setLayoutData(rd);
         buttonView.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            viewSelectedRecord();
         }
      });

         buttonRestore = new Button(buttons, SWT.PUSH);
         buttonRestore.setText(i18n.tr("&Restore"));
         rd = new RowData();
         rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
         buttonRestore.setLayoutData(rd);
         buttonRestore.setEnabled(false);
         buttonRestore.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            restoreSelectedRecord();
         }
      });

         historyViewer.addSelectionChangedListener((e) -> buttonRestore.setEnabled(historyViewer.getStructuredSelection().size() == 1));
         historyViewer.addDoubleClickListener((e) -> viewSelectedRecord());

         loadHistory();
      }

      return dialogArea;
   }

   /**
    * Load instructions history from server
    */
   private void loadHistory()
   {
      new Job(i18n.tr("Loading standing instructions history"), null) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final List<AiOperatorInstructionsHistoryRecord> records = session.getAiOperatorInstructionsHistory(operator.getId());
            runInUIThread(() -> {
               if (!historyViewer.getControl().isDisposed())
                  historyViewer.setInput(records.toArray());
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot load standing instructions history");
         }
      }.start();
   }

   /**
    * Show full text of selected history record
    */
   private void viewSelectedRecord()
   {
      IStructuredSelection selection = historyViewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      AiOperatorInstructionsHistoryRecord record = (AiOperatorInstructionsHistoryRecord)selection.getFirstElement();
      String text = record.getPreviousText();
      MessageDialogHelper.openInformation(getShell(), i18n.tr("Previous Standing Instructions"), text.isEmpty() ? i18n.tr("(empty)") : text);
   }

   /**
    * Put text of selected history record into the editor (saved when the dialog is confirmed)
    */
   private void restoreSelectedRecord()
   {
      IStructuredSelection selection = historyViewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      textInstructions.setText(((AiOperatorInstructionsHistoryRecord)selection.getFirstElement()).getPreviousText());
   }

   /**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(boolean isApply)
   {
      operator.setInstructions(textInstructions.getText().trim());
      operator.setInstructionsLocked(checkLocked.getSelection());
      return true;
   }

   /**
    * Label provider for history records
    */
   private class HistoryLabelProvider extends LabelProvider implements ITableLabelProvider
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
         AiOperatorInstructionsHistoryRecord record = (AiOperatorInstructionsHistoryRecord)element;
         switch(columnIndex)
         {
            case COLUMN_TIMESTAMP:
               return (record.getTimestamp() != null) ? DateFormatFactory.getDateTimeFormat().format(record.getTimestamp()) : "";
            case COLUMN_ITERATION:
               return Integer.toString(record.getIteration());
            case COLUMN_TEXT:
               String text = record.getPreviousText();
               int newline = text.indexOf('\n');
               return (newline != -1) ? text.substring(0, newline) + "..." : text;
         }
         return null;
      }
   }
}
