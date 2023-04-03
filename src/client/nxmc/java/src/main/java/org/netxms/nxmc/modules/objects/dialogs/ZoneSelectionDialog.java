/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Raden Solutions
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
package org.netxms.nxmc.modules.objects.dialogs;

import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.Zone;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.dialogs.DialogWithFilter;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.dialogs.helpers.ZoneSelectionDialogComparator;
import org.netxms.nxmc.modules.objects.dialogs.helpers.ZoneSelectionDialogFilter;
import org.netxms.nxmc.modules.objects.widgets.helpers.DecoratingObjectLabelProvider;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog for selecting zone
 */
public class ZoneSelectionDialog extends DialogWithFilter
{
   private I18n i18n = LocalizationHelper.getI18n(ZoneSelectionDialog.class);
   private Zone zone;
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

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(i18n.tr("Select Zone"));
      newShell.setSize(300, 500);
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);
      NXCSession session = Registry.getSession();

      GridLayout layout = new GridLayout();
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.horizontalSpacing = WidgetHelper.OUTER_SPACING;
      dialogArea.setLayout(layout);

      final String[] names = { i18n.tr("Name") };
      final int[] widths = { 200 };
      zoneList = new SortableTableViewer(dialogArea, names, widths, 0, SWT.UP, SWT.BORDER | SWT.FULL_SELECTION | SWT.SINGLE | SWT.V_SCROLL);
      zoneList.setContentProvider(new ArrayContentProvider());
      zoneList.setComparator(new ZoneSelectionDialogComparator());
      zoneList.setLabelProvider(new DecoratingObjectLabelProvider());
      filter = new ZoneSelectionDialogFilter();
      zoneList.addFilter(filter);
      setFilterClient(zoneList, filter);

      GridData gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      zoneList.getTable().setLayoutData(gd);
      
      zoneList.setInput(session.getAllZones());
      
      zoneList.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            ZoneSelectionDialog.this.okPressed();
         }
      });
      
      return dialogArea;
   }

   /**
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
         MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Please select zone object"));
         return;
      }
      super.okPressed();
   }

   /**
    * @return the zoneObjectId
    */
   public int getZoneUIN()
   {
      return zone.getUIN();
   }

   /**
    * Get zone object ID
    * 
    * @return zone object ID
    */
   public long getZoneObjectId()
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
