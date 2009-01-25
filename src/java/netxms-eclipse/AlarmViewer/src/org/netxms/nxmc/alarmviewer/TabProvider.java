/**
 * 
 */
package org.netxms.nxmc.alarmviewer;

import org.eclipse.swt.custom.CTabFolder;
import org.netxms.nxmc.core.extensionproviders.IObjectTabProvider;
import org.netxms.nxmc.core.extensionproviders.ObjectTab;

/**
 * @author Victor
 *
 */
public class TabProvider implements IObjectTabProvider
{
	/* (non-Javadoc)
	 * @see org.netxms.nxmc.core.extensionproviders.IObjectTabProvider#createObjectTab(org.eclipse.swt.custom.CTabFolder)
	 */
	@Override
	public ObjectTab createObjectTab(CTabFolder tabFolder)
	{
		return new ObjectTabAlarms(tabFolder);
	}
}
