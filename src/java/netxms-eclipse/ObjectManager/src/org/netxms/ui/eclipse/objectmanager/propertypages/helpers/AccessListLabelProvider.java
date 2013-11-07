/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.swt.graphics.Image;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.netxms.client.AccessListElement;

/**
 * Label provider for NetXMS objects access lists
 *
 */
public class AccessListLabelProvider extends WorkbenchLabelProvider implements ITableLabelProvider
{
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
	 */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		if (columnIndex == 0)
			return getImage(element);
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
			case 0:
				return getText(element);
			case 1:
				AccessListElement e = (AccessListElement)element;
				StringBuilder sb = new StringBuilder(16);
				sb.append(e.hasRead() ? 'R' : '-');
				sb.append(e.hasModify() ? 'M' : '-');
				sb.append(e.hasCreate() ? 'C' : '-');
				sb.append(e.hasDelete() ? 'D' : '-');
				sb.append(e.hasControl() ? 'O' : '-');
				sb.append(e.hasSendEvents() ? 'E' : '-');
				sb.append(e.hasReadAlarms() ? 'V' : '-');
				sb.append(e.hasAckAlarms() ? 'K' : '-');
				sb.append(e.hasTerminateAlarms() ? 'T' : '-');
				sb.append(e.hasPushData() ? 'P' : '-');
				sb.append(e.hasAccessControl() ? 'A' : '-');
				return sb.toString();
		}
		return null;
	}
}
