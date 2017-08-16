/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.epp.propertypages.helpers;

import org.eclipse.jface.viewers.ITableColorProvider;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.widgets.Display;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.netxms.client.events.EventGroup;
import org.netxms.client.events.EventObject;
import org.netxms.ui.eclipse.epp.Activator;

/**
 * Label provider for event template objects
 */
public class EventObjectLabelProvider extends WorkbenchLabelProvider implements ITableLabelProvider, ITableColorProvider
{
   private static final Color COLOR_GROUP = new Color(Display.getDefault(), new RGB(255, 221, 173));
   public static Image IMG_EVENT_GROUP = Activator.getImageDescriptor("icons/event_group.png").createImage();
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
	 */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
	   if ((columnIndex != 0))
	      return null;
	   if (element instanceof EventGroup)
	      return IMG_EVENT_GROUP;
		return getImage(element);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
	 */
	@Override
	public String getColumnText(Object element, int columnIndex)
	{
	   return ((EventObject)element).getName();	
	}

   @Override
   public Color getForeground(Object element, int columnIndex)
   {
      return null;
   }

   @Override
   public Color getBackground(Object element, int columnIndex)
   {
      if (element instanceof EventGroup)
         return COLOR_GROUP;
      return null;
   }
}
