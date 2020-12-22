/**
 * NetXMS - open source network management system
 * Copyright (C) 2020 Raden Solutions
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
package org.netxms.ui.eclipse.datacollection.dialogs;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.TableViewerColumn;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.base.NXCPCodes;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.ui.eclipse.datacollection.Messages;
import org.netxms.ui.eclipse.datacollection.dialogs.helpers.BulkDciUpdateElementUI;
import org.netxms.ui.eclipse.datacollection.dialogs.helpers.BulkUpdateLabelProvider;
import org.netxms.ui.eclipse.datacollection.dialogs.helpers.BulkValueEditSupport;

/**
 * DCI bulk update dialog
 */
public class BulkUpdateDialog extends Dialog
{
   private TableViewer tableViewer;
   private List<BulkDciUpdateElementUI> elements;

   /**
    * Bulk update dialog constructor
    * 
    * @param parentShell shell
    * @param isCustomRetention true if all DCIs have custom retention time
    * @param isCustomInterval true if all DCIs have custom interval
    */
   public BulkUpdateDialog(Shell parentShell, boolean isCustomRetention, boolean isCustomInterval)
   {      
      super(parentShell);

      elements = new ArrayList<BulkDciUpdateElementUI>();

      String[] pollingModes = {"No change", Messages.get().General_FixedIntervalsDefault, Messages.get().General_FixedIntervalsCustom, Messages.get().General_CustomSchedule};
      BulkDciUpdateElementUI pollingMode = new BulkDciUpdateElementUI("Polling mode", NXCPCodes.VID_POLLING_SCHEDULE_TYPE, pollingModes);
      elements.add(pollingMode);
      elements.add(new BulkDciUpdateElementUI("Polling interval (seconds)", NXCPCodes.VID_POLLING_INTERVAL, null, new BulkDciUpdateElementUI.EditModeSelector() {
         @Override
         public boolean isEditable()
         {
            return (isCustomInterval && pollingMode.getSelectionValue() == -1) || (pollingMode.getSelectionValue() == DataCollectionObject.POLLING_SCHEDULE_CUSTOM);
         }
      }));

      String[] retentionTypes = {"No change", Messages.get().General_UseDefaultRetention, Messages.get().General_UseCustomRetention, Messages.get().General_NoStorage};
      BulkDciUpdateElementUI retentionType = new BulkDciUpdateElementUI("Retention mode", NXCPCodes.VID_RETENTION_TYPE, retentionTypes);
      elements.add(retentionType);
      elements.add(new BulkDciUpdateElementUI("Retention time (days)", NXCPCodes.VID_RETENTION_TIME, null, new BulkDciUpdateElementUI.EditModeSelector() {
         @Override
         public boolean isEditable()
         {
            return (isCustomRetention && retentionType.getSelectionValue() == -1) || (retentionType.getSelectionValue() == DataCollectionObject.RETENTION_CUSTOM);
         }
      }));
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      int style = SWT.BORDER | SWT.H_SCROLL | SWT.V_SCROLL | SWT.HIDE_SELECTION;      

      String[] columnNames = new String[] { "Attribute", "Value" };
      tableViewer = new TableViewer(parent, style);
      
      tableViewer.setColumnProperties(columnNames);  

      TableViewerColumn column = new TableViewerColumn(tableViewer, SWT.LEFT);
      column.getColumn().setText("Attribute");
      column.getColumn().setWidth(300);

      column = new TableViewerColumn(tableViewer, SWT.LEFT);
      column.getColumn().setText("Value");
      column.getColumn().setWidth(400);
      column.setEditingSupport(new BulkValueEditSupport(tableViewer));  

      tableViewer.getTable().setLinesVisible(true);
      tableViewer.getTable().setHeaderVisible(true);
      tableViewer.setContentProvider(new ArrayContentProvider());
      tableViewer.setLabelProvider(new BulkUpdateLabelProvider());
      tableViewer.setInput(elements.toArray());
      
      return dialogArea;
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText("Bulk DCI update");
   }
   
   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      super.okPressed();
   }

   /**
    * Get bulk update elements
    * 
    * @return list of bulk update elements
    */
   public List<BulkDciUpdateElementUI> getBulkUpdateElements()
   {
      return elements;
   }
}
