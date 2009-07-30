package org.netxms.ui.eclipse.usermanager;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.swt.graphics.Image;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.netxms.client.NXCUser;
import org.netxms.client.NXCUserDBObject;
import org.netxms.ui.eclipse.usermanager.views.UserManager;

/**
 * Label provider for user manager
 * 
 * @author Victor
 * 
 */
public class UserLabelProvider extends WorkbenchLabelProvider implements ITableLabelProvider
{
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		return (columnIndex == 0) ? getImage(element) : null;
	}

	@Override
	public String getColumnText(Object element, int columnIndex)
	{
		switch(columnIndex)
		{
			case UserManager.COLUMN_NAME:
				return getText(element);
			case UserManager.COLUMN_TYPE:
				return (element instanceof NXCUser) ? "User" : "Group";
			case UserManager.COLUMN_FULLNAME:
				return (element instanceof NXCUser) ? ((NXCUser) element).getFullName() : null;
			case UserManager.COLUMN_DESCRIPTION:
				return ((NXCUserDBObject) element).getDescription();
			case UserManager.COLUMN_GUID:
				return ((NXCUserDBObject) element).getGuid().toString();
		}
		return null;
	}
}