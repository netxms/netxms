package org.netxms.ui.eclipse.dashboard.api;

import org.eclipse.swt.widgets.Composite;

/**
 * Creation data for custom dashboard element
 */
public class DashboardElementCreationData
{
	public Composite parent;
	public String data;

	/**
	 * @param parent
	 * @param data
	 */
	public DashboardElementCreationData(Composite parent, String data)
	{
		super();
		this.parent = parent;
		this.data = data;
	}
}
