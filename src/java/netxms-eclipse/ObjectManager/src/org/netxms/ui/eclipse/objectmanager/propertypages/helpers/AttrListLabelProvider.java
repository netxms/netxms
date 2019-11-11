/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.objectmanager.propertypages.helpers;

import java.util.Map.Entry;
import org.eclipse.jface.viewers.IColorProvider;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.widgets.Display;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.configs.CustomAttribute;
import org.netxms.ui.eclipse.objectmanager.propertypages.CustomAttributes;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Label provider for custom attributes list elements
 */
public class AttrListLabelProvider extends LabelProvider implements ITableLabelProvider, IColorProvider
{
   private static final Color COLOR_INHERITED = new Color(Display.getDefault(), new RGB(220,220,220));
   private WorkbenchLabelProvider wbLabelProvider = new WorkbenchLabelProvider();
   
   private NXCSession session;
   
   /**
    * Constructore
    */
   public AttrListLabelProvider()
   {
      session = ConsoleSharedData.getSession();
   }
   
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
	 */
   @SuppressWarnings("rawtypes")
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
	   if (columnIndex == CustomAttributes.COLUMN_INHERETED_FROM)
	   {
         CustomAttribute attr = (CustomAttribute)((Entry)element).getValue();
         AbstractObject object = session.findObjectById(attr.getSourceObject());
	      return (object != null) ? wbLabelProvider.getImage(object) : null;
	   }
	   
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
	 */
	@SuppressWarnings("rawtypes")
	@Override
	public String getColumnText(Object element, int columnIndex)
	{
		if (!(element instanceof Entry))
			return null;

		Object obj;
		CustomAttribute attr;
		switch(columnIndex)
		{
			case CustomAttributes.COLUMN_NAME:
				obj = ((Entry)element).getKey();
				return (obj instanceof String) ? (String)obj : null;
			case CustomAttributes.COLUMN_VALUE:
			   attr = (CustomAttribute)((Entry)element).getValue();
				return attr.getValue();
         case CustomAttributes.COLUMN_INHERITABLE:
            attr = (CustomAttribute)((Entry)element).getValue();
            return (attr.getFlags() & CustomAttribute.INHERITABLE) > 0 ? "Yes" : "No";
         case CustomAttributes.COLUMN_INHERETED_FROM:
            attr = (CustomAttribute)((Entry)element).getValue();
            if (attr.getSourceObject() == 0)
               return null;
            AbstractObject object = session.findObjectById(attr.getSourceObject());
            return (object != null) ? object.getObjectName() : ("[" + Long.toString(attr.getSourceObject()) + "]"); //$NON-NLS-1$ //$NON-NLS-2$
		}
		return null;
	}

   @Override
   public Color getForeground(Object element)
   {
      return null;
   }

   @SuppressWarnings("rawtypes")
   @Override
   public Color getBackground(Object element)
   {
      CustomAttribute attr = (CustomAttribute)((Entry)element).getValue();
      if (attr.getSourceObject() != 0 && (attr.getFlags() & CustomAttribute.REDEFINED) == 0)
         return COLOR_INHERITED;
      return null;
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.BaseLabelProvider#dispose()
    */
   @Override
   public void dispose()
   {
      wbLabelProvider.dispose();
      super.dispose();
   }
}
