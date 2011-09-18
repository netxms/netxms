/**
 * 
 */
package org.netxms.ui.eclipse.dashboard.widgets.internal;

/**
 * Abstract base class for all dashboard element configs
 *
 */
public abstract class DashboardElementConfig
{
	/**
	 * Create XML from configuration.
	 * 
	 * @return XML document
	 * @throws Exception if the schema for the object is not valid
	 */
	public abstract String createXml() throws Exception;
}
