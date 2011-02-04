/**
 * 
 */
package org.netxms.ui.eclipse.objecttools.views.helpers;

import java.util.List;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;

/**
 * Label provider for NetXMS table
 *
 */
public class TableLabelProvider extends LabelProvider implements ITableLabelProvider
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
	@SuppressWarnings("unchecked")
	@Override
	public String getColumnText(Object element, int columnIndex)
	{
		List<String> row = (List<String>)element;
		
		if (columnIndex >= row.size())
			return null;
		
		return row.get(columnIndex);
	}
}
