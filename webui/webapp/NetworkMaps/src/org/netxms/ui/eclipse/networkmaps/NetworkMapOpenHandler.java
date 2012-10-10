/**
 * 
 */
package org.netxms.ui.eclipse.networkmaps;

import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.PlatformUI;
import org.netxms.client.objects.GenericObject;
import org.netxms.client.objects.NetworkMap;
import org.netxms.ui.eclipse.networkmaps.views.PredefinedMap;
import org.netxms.ui.eclipse.objectbrowser.api.ObjectOpenHandler;

/**
 * Open handler for network maps
 */
public class NetworkMapOpenHandler implements ObjectOpenHandler
{
	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectbrowser.api.ObjectOpenHandler#openObject(org.netxms.client.objects.GenericObject)
	 */
	@Override
	public boolean openObject(GenericObject object)
	{
		if (!(object instanceof NetworkMap))
			return false;

		final IWorkbenchWindow window = PlatformUI.getWorkbench().getActiveWorkbenchWindow();
		try
		{
			window.getActivePage().showView(PredefinedMap.ID, Long.toString(object.getObjectId()), IWorkbenchPage.VIEW_ACTIVATE);
		}
		catch(PartInitException e)
		{
			MessageDialog.openError(window.getShell(), "Error", "Error opening view: " + e.getMessage());
		}
		return true;
	}
}
