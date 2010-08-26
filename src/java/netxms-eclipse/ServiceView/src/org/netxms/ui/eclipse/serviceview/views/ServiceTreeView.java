/**
 * 
 */
package org.netxms.ui.eclipse.serviceview.views;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.serviceview.widgets.ServiceTree;
import org.netxms.ui.eclipse.shared.NXMCSharedData;

/**
 * Service tree view
 *
 */
public class ServiceTreeView extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.serviceview.views.ServiceTreeView";
	
	private NXCSession session;
	private ServiceTree serviceTree;

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		session = NXMCSharedData.getInstance().getSession();
		
		parent.setLayout(new FillLayout());
		serviceTree = new ServiceTree(parent, SWT.NONE, session.findObjectById(2));
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		serviceTree.setFocus();
	}

}
