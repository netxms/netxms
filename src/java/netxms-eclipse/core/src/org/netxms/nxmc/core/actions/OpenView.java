/**
 * 
 */
package org.netxms.nxmc.core.actions;

import org.eclipse.jface.action.Action;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.PartInitException;
import org.netxms.nxmc.core.Activator;

/**
 * @author Victor
 *
 */
public class OpenView extends Action
{
	private final IWorkbenchWindow window;
	private int instanceNum = 0;
	private final String viewId;
	private boolean isSingleInstance;
	
	public OpenView(IWorkbenchWindow window, String label, String viewId,
	                      String cmdId, String icon, boolean isSingleInstance) 
	{
		this.window = window;
		this.viewId = viewId;
		this.isSingleInstance = isSingleInstance;
		setText(label);
        
		// The id is used to refer to the action in a menu or toolbar
		setId(cmdId);
        
		// Associate the action with a pre-defined command, to allow key bindings.
		setActionDefinitionId(cmdId);
		if (icon != null)
			setImageDescriptor(Activator.getImageDescriptor(icon));
	}
	
	@Override
	public void run() 
	{
		if(window != null)
		{	
			try 
			{
				if (isSingleInstance)
					window.getActivePage().showView(viewId);
				else
					window.getActivePage().showView(viewId, Integer.toString(instanceNum++), IWorkbenchPage.VIEW_ACTIVATE);
			} 
			catch (PartInitException e) 
			{
				MessageDialog.openError(window.getShell(), "Error", "Error opening view: " + e.getMessage());
			}
		}
	}
}
