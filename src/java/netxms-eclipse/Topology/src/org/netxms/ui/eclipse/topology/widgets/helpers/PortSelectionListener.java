/**
 * 
 */
package org.netxms.ui.eclipse.topology.widgets.helpers;

/**
 * Selection listener for switch port display widget 
 */
public interface PortSelectionListener
{
	/**
	 * Called by widget when user selects port icon.
	 * 
	 * @param port Selected port or null if user clicks on empty space
	 */
	public abstract void portSelected(PortInfo port);
}
