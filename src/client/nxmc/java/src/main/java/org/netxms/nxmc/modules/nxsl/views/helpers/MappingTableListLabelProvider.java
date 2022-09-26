/**
 * 
 */
package org.netxms.nxmc.modules.nxsl.views.helpers;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.mt.MappingTable;
import org.netxms.client.mt.MappingTableDescriptor;
import org.netxms.nxmc.modules.nxsl.views.MappingTableManagerView;

/**
 * Label provider for mapping tables list
 */
public class MappingTableListLabelProvider extends LabelProvider implements ITableLabelProvider
{
   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		return null;
	}

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
	@Override
	public String getColumnText(Object element, int columnIndex)
	{
		MappingTableDescriptor d = (MappingTableDescriptor)element;
		switch(columnIndex)
		{
         case MappingTableManagerView.COLUMN_ID:
				return Integer.toString(d.getId());
         case MappingTableManagerView.COLUMN_NAME:
				return d.getName();
         case MappingTableManagerView.COLUMN_DESCRIPTION:
				return d.getDescription();
         case MappingTableManagerView.COLUMN_FLAGS:
            return ((d.getFlags() & MappingTable.NUMERIC_KEYS) != 0) ? "NUMERIC" : "";
		}
		return null;
	}
}
