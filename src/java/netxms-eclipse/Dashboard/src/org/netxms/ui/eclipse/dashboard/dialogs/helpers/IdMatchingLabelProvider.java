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
package org.netxms.ui.eclipse.dashboard.dialogs.helpers;

import org.eclipse.jface.viewers.IColorProvider;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.netxms.ui.eclipse.dashboard.Activator;
import org.netxms.ui.eclipse.dashboard.Messages;
import org.netxms.ui.eclipse.dashboard.dialogs.IdMatchingDialog;
import org.netxms.ui.eclipse.objectbrowser.shared.ObjectIcons;
import org.netxms.ui.eclipse.shared.SharedColors;
import org.netxms.ui.eclipse.tools.ImageCache;

/**
 * Label provider for ID matching
 */
public class IdMatchingLabelProvider extends LabelProvider implements ITableLabelProvider, IColorProvider
{
	private ImageCache imageCache = new ImageCache();
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
	 */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		if (columnIndex != 0)
			return null;
		
		if (element instanceof ObjectIdMatchingData)
			return imageCache.add(ObjectIcons.getObjectImageDescriptor(((ObjectIdMatchingData)element).objectClass));
		
		if (element instanceof DciIdMatchingData)
			return imageCache.add(Activator.getImageDescriptor("icons/dci.png")); //$NON-NLS-1$
		
		return null;
	}

	/* (non-Javadoc)
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
				return (id > 0) ? Long.toString(id) : Messages.IdMatchingLabelProvider_NoMatch;
			case IdMatchingDialog.COLUMN_DESTINATION_NAME:
				return ((IdMatchingData)element).getDestinationName();
		}
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IColorProvider#getForeground(java.lang.Object)
	 */
	@Override
	public Color getForeground(Object element)
	{
		return (((IdMatchingData)element).getDestinationId() != 0) ? SharedColors.BLACK : SharedColors.RED;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IColorProvider#getBackground(java.lang.Object)
	 */
	@Override
	public Color getBackground(Object element)
	{
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.BaseLabelProvider#dispose()
	 */
	@Override
	public void dispose()
	{
		imageCache.dispose();
		super.dispose();
	}
}
