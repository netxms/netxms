/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2009 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.logviewer.dialogs.helpers;

import org.eclipse.jface.viewers.ILabelProvider;
import org.eclipse.jface.viewers.ILabelProviderListener;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.log.ColumnFilter;
import org.netxms.client.log.LogColumn;
import org.netxms.ui.eclipse.logviewer.widgets.helpers.FilterTreeElement;

/**
 * @author Victor
 *
 */
public class FilterTreeLabelProvider implements ILabelProvider
{
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ILabelProvider#getImage(java.lang.Object)
	 */
	@Override
	public Image getImage(Object element)
	{
		// TODO Auto-generated method stub
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ILabelProvider#getText(java.lang.Object)
	 */
	@Override
	public String getText(Object element)
	{
		FilterTreeElement fte = (FilterTreeElement)element;
		switch(fte.getType())
		{
			case FilterTreeElement.ROOT:
				return "[FILTER]";
			case FilterTreeElement.COLUMN:
				return ((LogColumn)fte.getObject()).getDescription();
			case FilterTreeElement.FILTER:
				final ColumnFilter cf = (ColumnFilter)fte.getObject();
				switch(cf.getType())
				{
					case ColumnFilter.EQUALS:
						return "= " + cf.getEqualsTo();
					case ColumnFilter.LIKE:
						return "LIKE " + cf.getLike();
					case ColumnFilter.RANGE:
						return "IN RANGE " + cf.getRangeFrom() + " .. " + cf.getRangeTo();
					case ColumnFilter.SET:
						return (cf.getOperation() == ColumnFilter.AND) ? "AND" : "OR";
					default:
						return "";
				}
			default:
				return "";
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IBaseLabelProvider#addListener(org.eclipse.jface.viewers.ILabelProviderListener)
	 */
	@Override
	public void addListener(ILabelProviderListener listener)
	{
		// TODO Auto-generated method stub

	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IBaseLabelProvider#dispose()
	 */
	@Override
	public void dispose()
	{
		// TODO Auto-generated method stub

	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IBaseLabelProvider#isLabelProperty(java.lang.Object, java.lang.String)
	 */
	@Override
	public boolean isLabelProperty(Object element, String property)
	{
		// TODO Auto-generated method stub
		return false;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IBaseLabelProvider#removeListener(org.eclipse.jface.viewers.ILabelProviderListener)
	 */
	@Override
	public void removeListener(ILabelProviderListener listener)
	{
		// TODO Auto-generated method stub

	}
}
