/**
 * NetXMS - open source network management system
 * Copyright (C) 2016 RadenSolutions
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
package org.netxms.ui.eclipse.alarmviewer.dialogs;

import java.util.HashMap;
import java.util.Locale;
import java.util.Map;
import java.util.Map.Entry;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.viewers.CellLabelProvider;
import org.eclipse.jface.viewers.IStructuredContentProvider;
import org.eclipse.jface.viewers.TableViewerColumn;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerCell;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.TableColumn;
import org.netxms.client.constants.RCC;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Show alarm state change failures
 */
public class AlarmStateChangeFailureDialog extends Dialog
{
   private SortableTableViewer alarmList;
   private Map<Long, Integer> terminationFails = new HashMap<Long, Integer>();

   /**
    * @param parentShell
    */
   public AlarmStateChangeFailureDialog(Shell parentShell, Map<Long, Integer> terminationFails)
   {
      super(parentShell);
      this.terminationFails = terminationFails;
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText("ALarm State Change Errors");
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      GridLayout layout = new GridLayout();
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.horizontalSpacing = WidgetHelper.OUTER_SPACING;
      layout.numColumns = 2;
      dialogArea.setLayout(layout);
      
      Label text = new Label(dialogArea, SWT.NONE);
      text.setText("State of some alarms was not changed due to the following reasons:");
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.horizontalSpan = 2;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.minimumHeight = 50;
      text.setLayoutData(gd);

      alarmList = new SortableTableViewer(dialogArea, SWT.BORDER | SWT.FULL_SELECTION | SWT.MULTI | SWT.V_SCROLL);
      alarmList.setContentProvider(new MapContentProvider());
      createColumns(parent, alarmList);
      alarmList.setInput(terminationFails);

      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.horizontalSpan = 2;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.heightHint = 350;
      alarmList.getTable().setLayoutData(gd);

      return dialogArea;
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      super.okPressed();
   }

   /**
    * Creates the columns for the table
    * @param parent
    * @param viewer
    */
   private void createColumns(final Composite parent, final SortableTableViewer viewer)
   {
      final String[] titles = { "ID", "Reason" };
      final int[] widths = { 80, 250 };

      // first column for alarm id
      TableViewerColumn col = createTableViewerColumn(titles[0], widths[0], 0);
      col.setLabelProvider(new CellLabelProvider()
      {
         @Override
         public void update(ViewerCell cell)
         {
            @SuppressWarnings("rawtypes")
            Object value = ((Entry)cell.getElement()).getKey();

            if (value != null)
               cell.setText(value.toString());
            else
               cell.setText("");
         }
      });

      // second column for termination failure reason
      col = createTableViewerColumn(titles[1], widths[1], 1);
      col.setLabelProvider(new CellLabelProvider()
      {
         @Override
         public void update(ViewerCell cell)
         {
            @SuppressWarnings("rawtypes")
            Object value = ((Entry)cell.getElement()).getValue();

            if (value != null)
               cell.setText(RCC.getText((int)value, Locale.getDefault().getLanguage(), null));
            else
               cell.setText("");
         }
      });
   }

   /**
    * Creates a single column
    * 
    * @param title
    * @param bound
    * @param colNumber
    * @return TableViewerColumn
    */
   private TableViewerColumn createTableViewerColumn(String title, int bound, final int colNumber)
   {
      final TableViewerColumn viewerColumn = new TableViewerColumn(alarmList, SWT.NONE);
      final TableColumn column = viewerColumn.getColumn();
      column.setText(title);
      column.setWidth(bound);
      column.setResizable(true);
      column.setMoveable(true);
      return viewerColumn;
   }

   /**
    * Content provider for Map objects
    */
   private class MapContentProvider implements IStructuredContentProvider
   {
      @Override
      public void inputChanged(Viewer viewer, Object oldInput, Object newInput)
      {
      }

      @SuppressWarnings("rawtypes")
      @Override
      public Object[] getElements(Object inputElement)
      {
         if (inputElement == null)
            return null;

         if (inputElement instanceof Map)
         {
            return ((Map)inputElement).entrySet().toArray();
         }

         return new Object[0];
      }

      @Override
      public void dispose()
      {
      }
   }
}
