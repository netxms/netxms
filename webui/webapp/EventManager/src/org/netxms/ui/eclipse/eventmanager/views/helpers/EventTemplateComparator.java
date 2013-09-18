/**
 * 
 */
package org.netxms.ui.eclipse.eventmanager.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.events.EventTemplate;
import org.netxms.ui.eclipse.eventmanager.views.EventConfigurator;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * @author Victor
 *
 */
public class EventTemplateComparator extends ViewerComparator
{
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		int result;

		switch((Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID")) //$NON-NLS-1$
		{
			case EventConfigurator.COLUMN_CODE:
				result = (int)(((EventTemplate)e1).getCode() - ((EventTemplate)e2).getCode());
				break;
			case EventConfigurator.COLUMN_NAME:
				result = ((EventTemplate)e1).getName().compareToIgnoreCase(((EventTemplate)e2).getName());
				break;
			case EventConfigurator.COLUMN_SEVERITY:
				result = ((EventTemplate)e1).getSeverity() - ((EventTemplate)e2).getSeverity();
				break;
			case EventConfigurator.COLUMN_FLAGS:
				result = ((EventTemplate)e1).getFlags() - ((EventTemplate)e2).getFlags();
				break;
			case EventConfigurator.COLUMN_MESSAGE:
				result = ((EventTemplate)e1).getMessage().compareToIgnoreCase(((EventTemplate)e2).getMessage());
				break;
			case EventConfigurator.COLUMN_DESCRIPTION:
				result = ((EventTemplate)e1).getDescription().compareToIgnoreCase(((EventTemplate)e2).getDescription());
				break;
			default:
				result = 0;
				break;
		}
		return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
	}
}
