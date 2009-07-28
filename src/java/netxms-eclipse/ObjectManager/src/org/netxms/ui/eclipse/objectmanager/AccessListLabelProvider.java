/**
 * 
 */
package org.netxms.ui.eclipse.objectmanager;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.swt.graphics.Image;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.netxms.client.NXCAccessListElement;

/**
 * @author Victor
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
				NXCAccessListElement e = (NXCAccessListElement)element;
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
