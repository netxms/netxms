/**
 * Overview tab for object browser
 */
package org.netxms.nxmc.core;

import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.swt.custom.CTabFolder;
import org.netxms.nxmc.core.extensionproviders.ObjectTab;

/**
 * @author victor
 *
 */
public class ObjectTabOverview extends ObjectTab
{

	/**
	 * @param tabFolder
	 */
	public ObjectTabOverview(CTabFolder tabFolder)
	{
		super(tabFolder);
	}

	/* (non-Javadoc)
	 * @see org.netxms.nxmc.core.ObjectTab#getImageDescriptor()
	 */
	@Override
	protected ImageDescriptor getImageDescriptor()
	{
		return Activator.getImageDescriptor("icons/information.png");
	}

	/* (non-Javadoc)
	 * @see org.netxms.nxmc.core.ObjectTab#getTabName()
	 */
	@Override
	protected String getTabName()
	{
		return "Overview";
	}
}
