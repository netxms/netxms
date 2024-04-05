/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objects.widgets.helpers;

import java.util.HashMap;
import java.util.Map;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.PhysicalComponent;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Interface;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.widgets.EntityMibTreeViewer;

/**
 * Label provider for component tree
 */
public class ComponentTreeLabelProvider extends LabelProvider implements ITableLabelProvider
{
	private static final String[] className = { 
	   null, 
	   LocalizationHelper.getI18n(ComponentTreeLabelProvider.class).tr("Other"), 
	   LocalizationHelper.getI18n(ComponentTreeLabelProvider.class).tr("Unknown"), 
	   LocalizationHelper.getI18n(ComponentTreeLabelProvider.class).tr("Chassis"), 
	   LocalizationHelper.getI18n(ComponentTreeLabelProvider.class).tr("Backplane"), 
	   LocalizationHelper.getI18n(ComponentTreeLabelProvider.class).tr("Container"),
	   LocalizationHelper.getI18n(ComponentTreeLabelProvider.class).tr("Power Supply"), 
	   LocalizationHelper.getI18n(ComponentTreeLabelProvider.class).tr("Fan"), 
	   LocalizationHelper.getI18n(ComponentTreeLabelProvider.class).tr("Sensor"), 
	   LocalizationHelper.getI18n(ComponentTreeLabelProvider.class).tr("Module"), 
	   LocalizationHelper.getI18n(ComponentTreeLabelProvider.class).tr("Port"), 
	   LocalizationHelper.getI18n(ComponentTreeLabelProvider.class).tr("Stack") 
	};

	private Map<Integer, Interface> interfaces = new HashMap<Integer, Interface>();

	/**
	 * Set current node
	 * 
    * @param node new node object
    */
   public void setNode(AbstractNode node)
   {
      interfaces.clear();
      if (node != null)
      {
         for(AbstractObject i : node.getAllChildren(AbstractObject.OBJECT_INTERFACE))
         {
            if (((Interface)i).getIfIndex() > 0)
            {
               interfaces.put(((Interface)i).getIfIndex(), (Interface)i);
            }
         }
      }
   }

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
		PhysicalComponent c = (PhysicalComponent)element;
		switch(columnIndex)
		{
         case EntityMibTreeViewer.COLUMN_CLASS:
				try
				{
					return className[c.getPhyClass()];
				}
				catch(ArrayIndexOutOfBoundsException e)
				{
					return className[PhysicalComponent.UNKNOWN];
				}
         case EntityMibTreeViewer.COLUMN_DESCRIPTION:
            return c.getDescription();
         case EntityMibTreeViewer.COLUMN_INTERFACE:
            return getInterfaceName(c);
         case EntityMibTreeViewer.COLUMN_FIRMWARE:
				return c.getFirmware();
         case EntityMibTreeViewer.COLUMN_MODEL:
				return c.getModel();
         case EntityMibTreeViewer.COLUMN_NAME:
				return c.getDisplayName();
         case EntityMibTreeViewer.COLUMN_SERIAL:
				return c.getSerialNumber();
         case EntityMibTreeViewer.COLUMN_VENDOR:
				return c.getVendor();
		}
		return null;
	}
	
	/**
	 * Get corresponding interface name
	 * 
	 * @param c component
	 * @return interface name or empty string
	 */
	private String getInterfaceName(PhysicalComponent c)
	{
	   int ifIndex = c.getIfIndex();
	   if (ifIndex <= 0)
	      return "";

	   Interface iface = interfaces.get(ifIndex);
	   return (iface != null) ? iface.getObjectName() : null;
	}
}
