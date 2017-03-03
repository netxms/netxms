/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2017 Raden Solutions
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
package org.netxms.ui.eclipse.objectbrowser.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.Zone;
import org.netxms.ui.eclipse.objectbrowser.Activator;
import org.netxms.ui.eclipse.objectbrowser.dialogs.helpers.ZoneSelectionDialogComparator;
import org.netxms.ui.eclipse.objectbrowser.dialogs.helpers.ZoneSelectionDialogFilter;
import org.netxms.ui.eclipse.objectbrowser.widgets.internal.ObjectTreeContentProvider;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.FilterText;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Dialog for selecting zone
 */
public class ZoneSelectionDialog extends Dialog
{
   private Zone zone;
   private FilterText filterText;
   private TableViewer zoneList;
   private ZoneSelectionDialogFilter filter;
   
   /**
    * Constructor
    * 
    * @param parent Parent shell
    */
   public ZoneSelectionDialog(Shell parent)
   {
      super(parent);
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText("Select Zone");
      newShell.setSize(300, 500);
   }
   
   /* (non-Javadoc)
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      IDialogSettings settings = Activator.getDefault().getDialogSettings();
      Composite dialogArea = (Composite)super.createDialogArea(parent);
      NXCSession session = (NXCSession)ConsoleSharedData.getSession();
      
      GridLayout layout = new GridLayout();
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.horizontalSpacing = WidgetHelper.OUTER_SPACING;
      dialogArea.setLayout(layout);
      
      filterText = new FilterText(dialogArea, SWT.NONE, null, false);
      GridData gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      filterText.setLayoutData(gd);
      final String filterString = settings.get("SelectZone.Filter"); //$NON-NLS-1$
      if (filterString != null)
         filterText.setText(filterString);
      
      final String[] names = { "Name" };
      final int[] widths = { 200 };
      zoneList = new SortableTableViewer(dialogArea, names, widths, 0, SWT.UP, 
            SWT.BORDER | SWT.FULL_SELECTION | SWT.SINGLE | SWT.V_SCROLL);
      long[] zoneIds = session.findAllZoneIds();
      zoneList.setContentProvider(new ObjectTreeContentProvider(zoneIds));
      zoneList.setComparator(new ZoneSelectionDialogComparator());
      zoneList.setLabelProvider(WorkbenchLabelProvider.getDecoratingWorkbenchLabelProvider());
      filter = new ZoneSelectionDialogFilter();
      if (filterString != null)
         filter.setFilterString(filterString);
      zoneList.addFilter(filter);
      zoneList.setInput(session);
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      zoneList.getTable().setLayoutData(gd);
      
      filterText.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            filter.setFilterString(filterText.getText());
            zoneList.refresh();
         }
      });
      
      zoneList.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            ZoneSelectionDialog.this.okPressed();
         }
      });
      
      filterText.setFocus();
      return dialogArea;
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      final IStructuredSelection selection = (IStructuredSelection)zoneList.getSelection();
      if (selection.size() > 0)
      {
         zone = (Zone)selection.getFirstElement();
      }
      else
      {
         MessageDialogHelper.openWarning(getShell(), "Warning", "Please select zone object");
         return;
      }
      super.okPressed();
   }

   /**
    * @return the zoneObjectId
    */
   public long getZoneId()
   {
      return zone.getObjectId();
   }
   
   /**
    * @return the zone name
    */
   public String getZoneName()
   {
      return zone.getObjectName();
   }
}
