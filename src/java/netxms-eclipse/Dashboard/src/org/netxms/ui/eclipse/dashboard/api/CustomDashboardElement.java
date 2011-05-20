/**
 * 
 */
package org.netxms.ui.eclipse.dashboard.api;

import org.eclipse.swt.widgets.Composite;

/**
 * Base class for custom dashboard widgets
 */
public abstract class CustomDashboardElement extends Composite
{
	/**
	 * @param parent
	 * @param style
	 * @param config
	 */
	public CustomDashboardElement(Composite parent, int style, String config)
	{
		super(parent, style);
	}
}
