/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.objectview.objecttabs.helpers;

import java.util.HashMap;
import java.util.Map;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.PhysicalComponent;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Interface;
import org.netxms.ui.eclipse.objectview.Messages;
import org.netxms.ui.eclipse.objectview.objecttabs.ComponentsTab;

/**
 * Label provider for component tree
 */
public class ComponentTreeLabelProvider extends LabelProvider implements ITableLabelProvider
{
	private final String[] className = { 
	   null, 
	   Messages.get().ComponentTreeLabelProvider_ClassOther, 
	   Messages.get().ComponentTreeLabelProvider_ClassUnknown, 
	   Messages.get().ComponentTreeLabelProvider_ClassChassis, 
	   Messages.get().ComponentTreeLabelProvider_ClassBackplane, 
	   Messages.get().ComponentTreeLabelProvider_ClassContainer,
	   Messages.get().ComponentTreeLabelProvider_ClassPS, 
	   Messages.get().ComponentTreeLabelProvider_ClassFan, 
	   Messages.get().ComponentTreeLabelProvider_ClassSensor, 
	   Messages.get().ComponentTreeLabelProvider_ClassModule, 
	   Messages.get().ComponentTreeLabelProvider_ClassPort, 
	   Messages.get().ComponentTreeLabelProvider_ClassStack 
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

   /* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
	 */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
	 */
	@Override
	public String getColumnText(Object element, int columnIndex)
	{
		PhysicalComponent c = (PhysicalComponent)element;
		switch(columnIndex)
		{
			case ComponentsTab.COLUMN_CLASS:
				try
				{
					return className[c.getPhyClass()];
				}
				catch(ArrayIndexOutOfBoundsException e)
				{
					return className[PhysicalComponent.UNKNOWN];
				}
         case ComponentsTab.COLUMN_DESCRIPTION:
            return c.getDescription();
         case ComponentsTab.COLUMN_INTERFACE:
            return getInterfaceName(c);
			case ComponentsTab.COLUMN_FIRMWARE:
				return c.getFirmware();
			case ComponentsTab.COLUMN_MODEL:
				return c.getModel();
			case ComponentsTab.COLUMN_NAME:
				return c.getDisplayName();
			case ComponentsTab.COLUMN_SERIAL:
				return c.getSerialNumber();
			case ComponentsTab.COLUMN_VENDOR:
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
