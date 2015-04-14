/**
 * 
 */
package org.netxms.ui.eclipse.serverconfig.views.helpers;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.mt.MappingTable;
import org.netxms.client.mt.MappingTableDescriptor;
import org.netxms.ui.eclipse.serverconfig.Messages;
import org.netxms.ui.eclipse.serverconfig.views.MappingTables;

/**
 * Label provider for mapping tables list
 */
public class MappingTableListLabelProvider extends LabelProvider implements ITableLabelProvider
{
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
		MappingTableDescriptor d = (MappingTableDescriptor)element;
		switch(columnIndex)
		{
			case MappingTables.COLUMN_ID:
				return Integer.toString(d.getId());
			case MappingTables.COLUMN_NAME:
				return d.getName();
			case MappingTables.COLUMN_DESCRIPTION:
				return d.getDescription();
			case MappingTables.COLUMN_FLAGS:
				return ((d.getFlags() & MappingTable.NUMERIC_KEYS) != 0) ? Messages.get().MappingTableListLabelProvider_FlagNumeric : Messages.get().MappingTableListLabelProvider_FlagNone;
		}
		return null;
	}
}
