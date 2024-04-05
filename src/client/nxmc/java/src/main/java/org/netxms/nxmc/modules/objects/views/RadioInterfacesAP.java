/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objects.views;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.swt.SWT;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.AccessPoint;
import org.netxms.client.topology.RadioInterface;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.views.helpers.RadioInterfaceAPComparator;
import org.netxms.nxmc.modules.objects.views.helpers.RadioInterfaceAPLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.xnap.commons.i18n.I18n;

/**
 * List of radio interfaces (access point)
 */
public class RadioInterfacesAP extends NodeSubObjectTableView
{
   private final I18n i18n = LocalizationHelper.getI18n(RadioInterfacesAP.class);

   public static final int COLUMN_INDEX = 0;
   public static final int COLUMN_NAME = 1;
   public static final int COLUMN_SSID = 2;
   public static final int COLUMN_BSSID = 3;
   public static final int COLUMN_NIC_VENDOR = 4;
   public static final int COLUMN_BAND = 5;
   public static final int COLUMN_CHANNEL = 6;
   public static final int COLUMN_FREQUENCY = 7;
   public static final int COLUMN_TX_POWER_DBM = 8;
   public static final int COLUMN_TX_POWER_MW = 9;

   /**
    * Radio interface view constructor
    */
   public RadioInterfacesAP()
   {
      super(LocalizationHelper.getI18n(RadioInterfacesAP.class).tr("Radios"), ResourceManager.getImageDescriptor("icons/object-views/radio_interfaces.png"), "objects.ap-radios", false);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 53;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return (context != null) && (((context instanceof AbstractNode) && ((AbstractNode)context).isWirelessAccessPoint()) || (context instanceof AccessPoint));
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.NodeSubObjectTableView#createViewer()
    */
   @Override
   protected void createViewer()
   {
      final String[] names = { i18n.tr("Index"), i18n.tr("Name"), i18n.tr("SSID"), i18n.tr("BSSID"), i18n.tr("NIC vendor"), i18n.tr("Band"), i18n.tr("Channel"), i18n.tr("Frequency"),
            i18n.tr("Tx power dBm"), i18n.tr("Tx power mW") };
      final int[] widths = { 90, 120, 120, 100, 200, 90, 90, 90, 90, 90 };
		viewer = new SortableTableViewer(mainArea, names, widths, 1, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
		viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new RadioInterfaceAPLabelProvider(viewer));
      viewer.setComparator(new RadioInterfaceAPComparator());

		createPopupMenu();
	}

   /**
    * @see org.netxms.nxmc.modules.objects.views.NodeSubObjectView#refresh()
    */
   @Override
   public void refresh()
   {
      if (getObject() == null)
      {
         viewer.setInput(new RadioInterface[0]);
         return;
      }

      List<RadioInterface> list = new ArrayList<RadioInterface>();
      if (getObject() instanceof AccessPoint)
      {
         for(RadioInterface rif : ((AccessPoint)getObject()).getRadios())
            list.add(rif);
      }
      else
      {
         for(RadioInterface rif : ((AbstractNode)getObject()).getRadios())
            list.add(rif);
      }

      viewer.setInput(list);
      viewer.packColumns();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectUpdate(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectUpdate(AbstractObject object)
   {
      refresh();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.NodeSubObjectView#needRefreshOnObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   public boolean needRefreshOnObjectChange(AbstractObject object)
   {
      return false;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.NodeSubObjectTableView#fillContextMenu(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   protected void fillContextMenu(IMenuManager manager)
   {
      manager.add(actionCopyToClipboard);
      manager.add(actionExportToCsv);
   }
}
