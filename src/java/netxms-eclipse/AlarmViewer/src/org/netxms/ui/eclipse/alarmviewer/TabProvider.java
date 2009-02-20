/**
 * 
 */
package org.netxms.ui.eclipse.alarmviewer;

import org.eclipse.swt.custom.CTabFolder;
import org.netxms.ui.eclipse.console.extensionproviders.IObjectTabProvider;
import org.netxms.ui.eclipse.console.extensionproviders.ObjectTab;

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
