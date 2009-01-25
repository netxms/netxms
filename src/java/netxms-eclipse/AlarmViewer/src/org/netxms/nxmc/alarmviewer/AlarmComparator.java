/**
 * 
 */
package org.netxms.nxmc.alarmviewer;

import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.TableColumn;
import org.netxms.client.NXCAlarm;

/**
 * @author victor
 *
 */
public class AlarmComparator extends ViewerComparator
{
	/**
	 * 
	 */
	public AlarmComparator()
	{
		super();
	}
	
	
	/**
	 * Compare two numbers and return -1, 0, or 1
	 */
	private int compareNumbers(long n1, long n2)
	{
		return (n1 < n2) ? -1 : ((n1 > n2) ? 1 : 0);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		TableColumn sortColumn = ((TableViewer)viewer).getTable().getSortColumn();
		if (sortColumn == null)
			return 0;
		
		int rc;
		switch((Integer)sortColumn.getData("ID"))
		{
			case AlarmView.COLUMN_SEVERITY:
				rc = compareNumbers(((NXCAlarm)e1).getCurrentSeverity(), ((NXCAlarm)e2).getCurrentSeverity());
				break;
			case AlarmView.COLUMN_STATE:
				rc = compareNumbers(((NXCAlarm)e1).getState(), ((NXCAlarm)e2).getState());
				break;
			case AlarmView.COLUMN_MESSAGE:
				rc = ((NXCAlarm)e1).getMessage().compareToIgnoreCase(((NXCAlarm)e2).getMessage());
				break;
			default:
				rc = 0;
				break;
		}
		int dir = ((TableViewer)viewer).getTable().getSortDirection();
		return (dir == SWT.UP) ? rc : -rc;
	}
}
