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
package org.netxms.ui.eclipse.datacollection.widgets.internal;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.datacollection.DciValue;
import org.netxms.ui.eclipse.datacollection.Activator;
import org.netxms.ui.eclipse.datacollection.widgets.DciList;

/**
 * Label provider for DCI list
 */
public class DciListLabelProvider extends LabelProvider implements ITableLabelProvider
{
	private static final long serialVersionUID = 1L;

	// State images
	private Image[] stateImages = new Image[3];
	
	/**
	 * Default constructor 
	 */
	public DciListLabelProvider()
	{
		super();

		stateImages[0] = Activator.getImageDescriptor("icons/active.gif").createImage(); //$NON-NLS-1$
		stateImages[1] = Activator.getImageDescriptor("icons/disabled.gif").createImage(); //$NON-NLS-1$
		stateImages[2] = Activator.getImageDescriptor("icons/unsupported.gif").createImage(); //$NON-NLS-1$
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
	 */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		return (columnIndex == DciList.COLUMN_ID) ? stateImages[((DciValue)element).getStatus()] : null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
	 */
	@Override
	public String getColumnText(Object element, int columnIndex)
	{
		switch(columnIndex)
		{
			case DciList.COLUMN_ID:
				return Long.toString(((DciValue)element).getId());
			case DciList.COLUMN_PARAMETER:
				return ((DciValue)element).getName();
			case DciList.COLUMN_DESCRIPTION:
				return ((DciValue)element).getDescription();
		}
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.BaseLabelProvider#dispose()
	 */
	@Override
	public void dispose()
	{
		for(int i = 0; i < stateImages.length; i++)
			stateImages[i].dispose();
		super.dispose();
	}
}
