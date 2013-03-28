/**
 * 
 */
package org.netxms.ui.eclipse.alarmviewer;

import org.eclipse.ui.IViewPart;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.PlatformUI;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.EntireNetwork;
import org.netxms.client.objects.ServiceRoot;
import org.netxms.client.objects.Subnet;
import org.netxms.client.objects.Zone;
import org.netxms.ui.eclipse.alarmviewer.views.ObjectAlarmBrowser;
import org.netxms.ui.eclipse.objectview.api.ObjectDetailsProvider;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;

/**
 * Object details provider which will show alarm list for given node
 */
public class AlarmDetailsProvider implements ObjectDetailsProvider
{
	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.api.ObjectDetailsProvider#canProvideDetails(org.netxms.client.objects.AbstractObject)
	 */
	@Override
	public boolean canProvideDetails(AbstractObject object)
	{
		return (object instanceof AbstractNode) || (object instanceof Container) ||
			(object instanceof ServiceRoot) || (object instanceof Subnet) ||
			(object instanceof Zone) || (object instanceof EntireNetwork) ||
			(object instanceof Cluster);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.api.ObjectDetailsProvider#provideDetails(org.netxms.client.objects.AbstractObject, org.eclipse.ui.IViewPart)
	 */
	@Override
	public void provideDetails(AbstractObject object, IViewPart viewPart)
	{
		try
		{
			IWorkbenchPage page = (viewPart != null) ? viewPart.getSite().getPage() : PlatformUI.getWorkbench().getActiveWorkbenchWindow().getActivePage();
			page.showView(ObjectAlarmBrowser.ID, Long.toString(object.getObjectId()), IWorkbenchPage.VIEW_ACTIVATE);
		}
		catch(PartInitException e)
		{
			MessageDialogHelper.openError((viewPart != null) ? viewPart.getSite().getShell() : null, Messages.AlarmDetailsProvider_Error, Messages.AlarmDetailsProvider_ErrorOpeningView + e.getMessage());
		}
	}
}
