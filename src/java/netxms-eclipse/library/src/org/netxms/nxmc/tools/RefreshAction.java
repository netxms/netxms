/**
 * 
 */
package org.netxms.nxmc.tools;

import org.eclipse.jface.action.Action;
import org.eclipse.swt.SWT;
import org.netxms.nxmc.library.Activator;

/**
 * @author victor
 *
 */
public class RefreshAction extends Action
{
	/**
	 * Constructor
	 */
	public RefreshAction()
	{
		super("Refresh", Activator.getImageDescriptor("icons/refresh.png"));
		setAccelerator(SWT.F5);
	}
}
