/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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
package org.netxms.nxmc.modules.datacollection.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.modules.datacollection.views.DataCollectionView;

/**
 * DCI comparator
 */
public class DciComparator extends ViewerComparator
{
	private DciLabelProvider labelProvider;

	/**
	 * Default constructor
	 */
	public DciComparator(DciLabelProvider labelProvider)
	{
		this.labelProvider = labelProvider;
	}

   /**
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		int result;

		DataCollectionObject dci1 = (DataCollectionObject)e1;
		DataCollectionObject dci2 = (DataCollectionObject)e2;

      int column = (Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID");
		switch(column)
		{
			case DataCollectionView.DC_COLUMN_ID:
				result = (int)(dci1.getId() - dci2.getId());
				break;
			case DataCollectionView.DC_COLUMN_DESCRIPTION:
				result = dci1.getDescription().compareToIgnoreCase(dci2.getDescription());
				break;
			case DataCollectionView.DC_COLUMN_PARAMETER:
				result = dci1.getName().compareToIgnoreCase(dci2.getName());
				break;
			case DataCollectionView.DC_COLUMN_INTERVAL:
				result = (int)(dci1.getComparablePollingInterval() - dci2.getComparablePollingInterval());
				break;
			case DataCollectionView.DC_COLUMN_RETENTION:
				result = (int)(dci1.getComparableRetentionTime() - dci2.getComparableRetentionTime());
				break;
			case DataCollectionView.DC_COLUMN_ORIGIN:
         case DataCollectionView.DC_COLUMN_DATAUNIT:
			case DataCollectionView.DC_COLUMN_DATATYPE:
			case DataCollectionView.DC_COLUMN_STATUS:
         case DataCollectionView.DC_COLUMN_TAG:
			case DataCollectionView.DC_COLUMN_TEMPLATE:
         case DataCollectionView.DC_COLUMN_RELATEDOBJ:
         case DataCollectionView.DC_COLUMN_STATUSCALC:
         case DataCollectionView.DC_COLUMN_THRESHOLD:
				final String text1 = labelProvider.getColumnText(e1, column);
				final String text2 = labelProvider.getColumnText(e2, column);
				if (text1 != null && text2 != null)
				{
					result = text1.compareToIgnoreCase(text2);
				}
				else
				{
					if (text1 == null)
					{
						if (text2 == null)
						{
							result = 0;
						}
						else
						{
							result = -1;
						}
					}
					else
					{
						result = 1;
					}
				}
				break;
			default:
				result = 0;
				break;
		}
		return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
	}
}
