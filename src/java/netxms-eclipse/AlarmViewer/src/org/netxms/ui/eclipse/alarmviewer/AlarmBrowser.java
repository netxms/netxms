/**
 * 
 */
package org.netxms.ui.eclipse.alarmviewer;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.part.ViewPart;

/**
 * @author Victor
 *
 */
public class AlarmBrowser extends ViewPart
{
	public static final String ID = "org.netxms.nxmc.alarmviewer.view.alarm_browser";
	
	private AlarmView alarmView;

	/**
	 * 
	 */
	public AlarmBrowser()
	{
		// TODO Auto-generated constructor stub
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
      FormLayout formLayout = new FormLayout();
		parent.setLayout(formLayout);
		
		alarmView = new AlarmView(this, parent, SWT.NONE);
		FormData fd = new FormData();
		fd.left = new FormAttachment(0, 0);
		fd.top = new FormAttachment(0, 0);
		fd.right = new FormAttachment(100, 0);
		fd.bottom = new FormAttachment(100, 0);
		alarmView.setLayoutData(fd);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		alarmView.setFocus();
	}
}
