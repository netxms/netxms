/**
 * 
 */
package org.netxms.ui.eclipse.alarmviewer;

import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.TableColumn;
import org.netxms.client.NXCSession;
import org.netxms.client.events.Alarm;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.alarmviewer.widgets.AlarmList;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * @author victor
 *
 */
public class AlarmComparator extends ViewerComparator
{
	private static final long serialVersionUID = 1L;

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
		switch((Integer)sortColumn.getData("ID")) //$NON-NLS-1$
		{
			case AlarmList.COLUMN_SEVERITY:
				rc = compareNumbers(((Alarm)e1).getCurrentSeverity(), ((Alarm)e2).getCurrentSeverity());
				break;
			case AlarmList.COLUMN_STATE:
				rc = compareNumbers(((Alarm)e1).getState(), ((Alarm)e2).getState());
				break;
			case AlarmList.COLUMN_SOURCE:
				GenericObject obj1 = ((NXCSession)ConsoleSharedData.getSession()).findObjectById(((Alarm)e1).getSourceObjectId());
				GenericObject obj2 = ((NXCSession)ConsoleSharedData.getSession()).findObjectById(((Alarm)e2).getSourceObjectId());
				String name1 = (obj1 != null) ? obj1.getObjectName() : Messages.AlarmComparator_Unknown;
				String name2 = (obj2 != null) ? obj2.getObjectName() : Messages.AlarmComparator_Unknown;
				rc = name1.compareToIgnoreCase(name2);
				break;
			case AlarmList.COLUMN_MESSAGE:
				rc = ((Alarm)e1).getMessage().compareToIgnoreCase(((Alarm)e2).getMessage());
				break;
			case AlarmList.COLUMN_COUNT:
				rc = compareNumbers(((Alarm)e1).getRepeatCount(), ((Alarm)e2).getRepeatCount());
				break;
			case AlarmList.COLUMN_CREATED:
				rc = ((Alarm)e1).getCreationTime().compareTo(((Alarm)e2).getCreationTime());
				break;
			case AlarmList.COLUMN_LASTCHANGE:
				rc = ((Alarm)e1).getLastChangeTime().compareTo(((Alarm)e2).getLastChangeTime());
				break;
			default:
				rc = 0;
				break;
		}
		int dir = ((TableViewer)viewer).getTable().getSortDirection();
		return (dir == SWT.UP) ? rc : -rc;
	}
}
