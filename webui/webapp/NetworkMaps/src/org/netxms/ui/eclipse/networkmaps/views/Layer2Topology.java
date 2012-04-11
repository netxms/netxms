/**
 * 
 */
package org.netxms.ui.eclipse.networkmaps.views;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.netxms.client.maps.NetworkMapPage;
import org.netxms.client.maps.elements.NetworkMapObject;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.networkmaps.Activator;

/**
 * Layer 2 topology view for given object
 *
 */
public class Layer2Topology extends NetworkMap
{
	public static final String ID = "org.netxms.ui.eclipse.networkmaps.views.Layer2Topology";
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
	 */
	@Override
	public void init(IViewSite site) throws PartInitException
	{
		super.init(site);
		setPartName("Layer 2 Topology - " + rootObject.getObjectName());
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.networkmaps.views.NetworkMap#buildMapPage()
	 */
	@Override
	protected void buildMapPage()
	{
		if (mapPage == null)
			mapPage = new NetworkMapPage();
		
		new ConsoleJob("Get layer 2 topology for " + rootObject.getObjectName(), this, Activator.PLUGIN_ID, Activator.PLUGIN_ID) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				NetworkMapPage page = session.queryLayer2Topology(rootObject.getObjectId());
				replaceMapPage(page, getDisplay());
			}

			@Override
			protected void jobFailureHandler()
			{
				// On failure, create map with root object only
				NetworkMapPage page = new NetworkMapPage();
				page.addElement(new NetworkMapObject(mapPage.createElementId(), rootObject.getObjectId()));
				replaceMapPage(page, getDisplay());
			}

			@Override
			protected String getErrorMessage()
			{
				return "Cannot get layer 2 topology for " + rootObject.getObjectName();
			}
		}.start();
	}
}
