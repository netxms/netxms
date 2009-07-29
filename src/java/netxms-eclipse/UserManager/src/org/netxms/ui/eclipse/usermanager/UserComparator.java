/**
 * 
 */
package org.netxms.ui.eclipse.usermanager;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.NXCUser;
import org.netxms.client.NXCUserDBObject;
import org.netxms.client.NXCUserGroup;
import org.netxms.ui.eclipse.tools.SortableTableViewer;
import org.netxms.ui.eclipse.usermanager.views.UserManager;

/**
 * @author Victor
 *
 */
public class UserComparator extends ViewerComparator
{
	/**
	 * Compare object types
	 */
	private int compareTypes(Object o1, Object o2)
	{
		int type1 = (o1 instanceof NXCUserGroup) ? 1 : 0;
		int type2 = (o2 instanceof NXCUserGroup) ? 1 : 0;
		return type1 - type2;
	}

	/**
	 * Get full name for user db object
	 */
	private String getFullName(Object object)
	{
		if (object instanceof NXCUser)
			return ((NXCUser) object).getFullName();
		return "";
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		int result;

		switch((Integer) ((SortableTableViewer) viewer).getTable().getSortColumn().getData("ID"))
		{
			case UserManager.COLUMN_NAME:
				result = ((NXCUserDBObject) e1).getName().compareToIgnoreCase(((NXCUserDBObject) e2).getName());
				break;
			case UserManager.COLUMN_TYPE:
				result = compareTypes(e1, e2);
				break;
			case UserManager.COLUMN_FULLNAME:
				result = getFullName(e1).compareToIgnoreCase(getFullName(e2));
				break;
			case UserManager.COLUMN_DESCRIPTION:
				result = ((NXCUserDBObject) e1).getDescription().compareToIgnoreCase(
						((NXCUserDBObject) e2).getDescription());
				break;
			case UserManager.COLUMN_GUID:
				result = ((NXCUserDBObject) e1).getGuid().toString().compareTo(
						((NXCUserDBObject) e2).getGuid().toString());
				break;
			default:
				result = 0;
				break;
		}
		return (((SortableTableViewer) viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
	}
}
