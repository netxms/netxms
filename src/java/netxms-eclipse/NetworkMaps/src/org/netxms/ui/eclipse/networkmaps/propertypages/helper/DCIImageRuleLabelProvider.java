/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.networkmaps.propertypages.helper;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.maps.configs.DCIImageRule;
import org.netxms.ui.eclipse.networkmaps.propertypages.DCIImageRuleList;

/**
 * Label provider for threshold objects
 */
public class DCIImageRuleLabelProvider extends LabelProvider implements ITableLabelProvider
{
	private static final String[] OPERATIONS = { "<", "<=", "==", ">=", ">", "!=", "like", "!like" }; //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$ //$NON-NLS-4$ //$NON-NLS-5$ //$NON-NLS-6$ //$NON-NLS-7$ //$NON-NLS-8$
		
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
		switch(columnIndex)
		{
			case DCIImageRuleList.COLUMN_OPERATION:
			   StringBuffer text = new StringBuffer(""); //$NON-NLS-1$
				text.append(OPERATIONS[((DCIImageRule)element).getComparisonType()]);
				text.append(' ');
				text.append(((DCIImageRule)element).getCompareValue());
				return text.toString();
			case DCIImageRuleList.COLUMN_COMMENT:
				return ((DCIImageRule)element).getComment();
		}
		return null;
	}
}
