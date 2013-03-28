/**
 * 
 */
package org.netxms.ui.eclipse.networkmaps;

import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.PlatformUI;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.NetworkMap;
import org.netxms.ui.eclipse.networkmaps.views.PredefinedMap;
import org.netxms.ui.eclipse.objectbrowser.api.ObjectOpenHandler;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;

/**
 * Open handler for network maps
 */
public class NetworkMapOpenHandler implements ObjectOpenHandler
{
	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectbrowser.api.ObjectOpenHandler#openObject(org.netxms.client.objects.AbstractObject)
	 */
	@Override
	public boolean openObject(AbstractObject object)
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
			MessageDialogHelper.openError(window.getShell(), "Error", "Error opening view: " + e.getMessage());
		}
		return true;
	}
}
