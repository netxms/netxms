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
package org.netxms.ui.eclipse.perfview.propertypages.helpers;

import java.util.List;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.datacollection.ChartDciConfig;
import org.netxms.ui.eclipse.perfview.Messages;
import org.netxms.ui.eclipse.perfview.propertypages.TemplateDataSources;

/**
 * Label provider for DCI list
 */
public class DciTemplateListLabelProvider extends LabelProvider implements ITableLabelProvider
{
	private List<ChartDciConfig> elementList;
	
	/**
	 * The constructor
	 */
	public DciTemplateListLabelProvider(List<ChartDciConfig> elementList)
	{
		this.elementList = elementList;
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
		ChartDciConfig dci = (ChartDciConfig)element;
		switch(columnIndex)
		{
			case TemplateDataSources.COLUMN_POSITION:
				return Integer.toString(elementList.indexOf(dci) + 1);
			case TemplateDataSources.COLUMN_NAME:
				return dci.getDciName(); //$NON-NLS-1$ //$NON-NLS-2$
			case TemplateDataSources.COLUMN_DESCRIPTION:
				return dci.getDciDescription();
			case TemplateDataSources.COLUMN_LABEL:
				return dci.name;
			case TemplateDataSources.COLUMN_COLOR:
				return dci.color.equalsIgnoreCase(ChartDciConfig.UNSET_COLOR) ? Messages.get().DciListLabelProvider_Auto : dci.color;
		}
		return null;
	}
}
