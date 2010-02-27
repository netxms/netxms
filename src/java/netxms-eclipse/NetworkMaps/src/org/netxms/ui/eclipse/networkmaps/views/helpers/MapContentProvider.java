package org.netxms.ui.eclipse.networkmaps.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.zest.core.viewers.IGraphEntityContentProvider;
import org.netxms.client.NXCSession;
import org.netxms.client.maps.NetworkMapPage;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.shared.NXMCSharedData;

/**
 * Content provider for map
 * 
 * @author Victor
 */
public class MapContentProvider implements IGraphEntityContentProvider
{
	private NetworkMapPage page;
	private NXCSession session;
	
	/**
	 * Create map content provider object
	 */
	public MapContentProvider()
	{
		session = NXMCSharedData.getInstance().getSession();
	}
	
	@Override
	public Object[] getElements(Object inputElement)
	{
		if (!(inputElement instanceof NetworkMapPage))
			return null;
		
		return ((NetworkMapPage)inputElement).getResolvedObjects(session);
	}

	@Override
	public Object[] getConnectedTo(Object entity)
	{
		if (!(entity instanceof GenericObject) || (page == null))
			return null;
		
		return page.getConnectedObjects(((GenericObject)entity).getObjectId(), session);
	}

	@Override
	public void dispose()
	{
	}

	@Override
	public void inputChanged(Viewer viewer, Object oldInput, Object newInput)
	{
		if (newInput instanceof NetworkMapPage)
			page = (NetworkMapPage)newInput;
		else
			page = null;
	}
}
