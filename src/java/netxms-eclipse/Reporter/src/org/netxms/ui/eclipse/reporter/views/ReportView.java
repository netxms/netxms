/**
 * 
 */
package org.netxms.ui.eclipse.reporter.views;

import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CTabFolder;
import org.eclipse.swt.custom.CTabItem;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.part.ViewPart;

/**
 * Report view
 */
public class ReportView extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.reporter.views.ReportView";
	
	private CTabFolder tabFolder;

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		tabFolder = new CTabFolder(parent, SWT.BOTTOM | SWT.FLAT | SWT.MULTI);
		tabFolder.setUnselectedImageVisible(true);
		tabFolder.setSimple(true);
		
		CTabItem item = new CTabItem(tabFolder, SWT.NONE);
		item.setText("Execute");

		item = new CTabItem(tabFolder, SWT.NONE);
		item.setText("Results");
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		tabFolder.setFocus();
	}

}
