/**
 * 
 */
package org.netxms.ui.eclipse.objectview.views;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCNode;
import org.netxms.client.NXCObject;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.objectview.LastValuesView;
import org.netxms.ui.eclipse.shared.NXMCSharedData;

/**
 * @author Victor
 *
 */
public class LastValues extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.objectview.view.last_values";
	
	private NXCSession session;
	private NXCNode node;
	private LastValuesView dataView;
	
	/**
	 * 
	 */
	public LastValues()
	{
		// TODO Auto-generated constructor stub
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
	 */
	@Override
	public void init(IViewSite site) throws PartInitException
	{
		super.init(site);
		
		session = NXMCSharedData.getInstance().getSession();
		NXCObject obj = session.findObjectById(Long.parseLong(site.getSecondaryId()));
		node = ((obj != null) && (obj instanceof NXCNode)) ? (NXCNode)obj : null;
		setPartName("Last Values - " + ((node != null) ? node.getObjectName() : "<error>"));
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
      FormLayout formLayout = new FormLayout();
		parent.setLayout(formLayout);
		
		dataView = new LastValuesView(this, parent, SWT.NONE, node);
		FormData fd = new FormData();
		fd.left = new FormAttachment(0, 0);
		fd.top = new FormAttachment(0, 0);
		fd.right = new FormAttachment(100, 0);
		fd.bottom = new FormAttachment(100, 0);
		dataView.setLayoutData(fd);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		dataView.setFocus();
	}
}
