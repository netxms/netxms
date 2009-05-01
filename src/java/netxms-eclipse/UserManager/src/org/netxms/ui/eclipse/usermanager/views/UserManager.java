/**
 * 
 */
package org.netxms.ui.eclipse.usermanager.views;

import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.tools.SortableTableViewer;

/**
 * @author Victor
 *
 */
public class UserManager extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.usermanager.view.user_manager";
	public static final String JOB_FAMILY = "UserManagerJob";
		
	private TableViewer viewer;
	private NXCSession session;

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		final String[] names = { "Name", "Type", "Full Name", "Description", "GUID" };
		final int[] widths = { 100, 80, 180, 180, 100 };
		viewer = new SortableTableViewer(parent, names, widths, 0, SWT.DOWN, SortableTableViewer.DEFAULT_STYLE);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		viewer.getControl().setFocus();
	}
}
