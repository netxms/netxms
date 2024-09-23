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
package org.netxms.nxmc.modules.dashboards.dialogs.helpers;

import org.eclipse.jface.viewers.IColorProvider;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.dashboards.dialogs.IdMatchingDialog;
import org.netxms.nxmc.modules.objects.ObjectIcons;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.ThemeEngine;
import org.xnap.commons.i18n.I18n;

/**
 * Label provider for ID matching
 */
public class IdMatchingLabelProvider extends LabelProvider implements ITableLabelProvider, IColorProvider
{
   private I18n i18n = LocalizationHelper.getI18n(IdMatchingLabelProvider.class);
   private Image dciImage = ResourceManager.getImage("icons/dci/dc-object.png");
   private ObjectIcons objectIcons = Registry.getSingleton(ObjectIcons.class);

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		if (columnIndex != 0)
			return null;

		if (element instanceof ObjectIdMatchingData)
         return objectIcons.getImage(((ObjectIdMatchingData)element).objectClass);

		if (element instanceof DciIdMatchingData)
         return dciImage;

		return null;
	}

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
	@Override
	public String getColumnText(Object element, int columnIndex)
	{
		switch(columnIndex)
		{
			case IdMatchingDialog.COLUMN_SOURCE_ID:
				return Long.toString(((IdMatchingData)element).getSourceId());
			case IdMatchingDialog.COLUMN_SOURCE_NAME:
				return ((IdMatchingData)element).getSourceName();
			case IdMatchingDialog.COLUMN_DESTINATION_ID:
				long id = ((IdMatchingData)element).getDestinationId();
            return ((id > 0) && (id != AbstractObject.UNKNOWN)) ? Long.toString(id) : i18n.tr("no match");
			case IdMatchingDialog.COLUMN_DESTINATION_NAME:
				return ((IdMatchingData)element).getDestinationName();
		}
		return null;
	}

   /**
    * @see org.eclipse.jface.viewers.IColorProvider#getForeground(java.lang.Object)
    */
	@Override
	public Color getForeground(Object element)
	{
      long id = ((IdMatchingData)element).getDestinationId();
      return ((id > 0) && (id != AbstractObject.UNKNOWN)) ? null : ThemeEngine.getForegroundColor("List.Error");
	}

   /**
    * @see org.eclipse.jface.viewers.IColorProvider#getBackground(java.lang.Object)
    */
	@Override
	public Color getBackground(Object element)
	{
		return null;
	}

   /**
    * @see org.eclipse.jface.viewers.BaseLabelProvider#dispose()
    */
	@Override
	public void dispose()
	{
      dciImage.dispose();
		super.dispose();
	}
}
