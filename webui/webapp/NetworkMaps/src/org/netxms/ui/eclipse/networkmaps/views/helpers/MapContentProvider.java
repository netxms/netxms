/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.ui.eclipse.networkmaps.views.helpers;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import org.eclipse.gef4.zest.core.viewers.IGraphEntityRelationshipContentProvider;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.maps.NetworkMapLink;
import org.netxms.client.maps.NetworkMapPage;
import org.netxms.client.maps.elements.NetworkMapDCIContainer;
import org.netxms.client.maps.elements.NetworkMapDCIImage;
import org.netxms.client.maps.elements.NetworkMapDecoration;
import org.netxms.client.maps.elements.NetworkMapElement;
import org.netxms.client.maps.elements.NetworkMapObject;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Content provider for map
 */
public class MapContentProvider implements IGraphEntityRelationshipContentProvider
{
	private ExtendedGraphViewer viewer;
	private NetworkMapPage page;
	private Map<Long, DciValue[]> cachedDciValues = new HashMap<Long, DciValue[]>();
	private NXCSession session = (NXCSession)ConsoleSharedData.getSession();
	private Thread syncThread = null;
	private volatile boolean syncRunning = true;
	
	/**
	 * Constructor
	 */
	public MapContentProvider(ExtendedGraphViewer viewer)
	{
		this.viewer = viewer;
		final Display display = viewer.getControl().getDisplay();
		syncThread = new Thread(new Runnable() {
			@Override
			public void run()
			{
				syncLastValues(display);
			}
		});
		syncThread.setDaemon(true);
		syncThread.start();
	}
	
	/**
	 * Synchronize last values in background
	 */
	private void syncLastValues(Display display)
	{
		try
		{
			Thread.sleep(1000);
		}
		catch(InterruptedException e3)
		{
			return;
		}
		while(syncRunning)
		{
			synchronized(cachedDciValues)
			{
				for(Entry<Long, DciValue[]> e : cachedDciValues.entrySet())
				{
					try
					{
						DciValue[] values = session.getLastValues(e.getKey(), true, false);
						cachedDciValues.put(e.getKey(), values);
					}
					catch(Exception e2)
					{
						e2.printStackTrace();	// for debug
					}
				}
				display.asyncExec(new Runnable() {
					@Override
					public void run()
					{
						if (viewer.getControl().isDisposed())
							return;
						
						synchronized(cachedDciValues)
						{
							for(Entry<Long, DciValue[]> e : cachedDciValues.entrySet())
							{
								if ((e.getValue() == null) || (e.getValue().length == 0))
									continue;
								NetworkMapObject o = page.findObjectElement(e.getKey());
								if (o != null)
									viewer.refresh(o);
							}
							if(page != null && page.getLinks() != null)
							{
   							for(NetworkMapLink e : page.getLinks())
                        {
                           if (!e.hasDciData())
                              continue;
                           viewer.refresh(e);
                        }
							}
							if(page != null && page.getElements() != null)
                     {
                        for(NetworkMapElement e : page.getElements())
                        {
                           if ((!(e instanceof NetworkMapDCIContainer) || !((NetworkMapDCIContainer)e).hasDciData()) && !(e instanceof NetworkMapDCIImage))
                              continue;
                           viewer.updateDecorationFigure(e);
                        }
                     }
						}
					}
				});
			}
			try
			{
				Thread.sleep(30000); 
			}
			catch(InterruptedException e1)
			{
				break;
			}
		}
	}
	
	/**
	 * Get last DCI values for given node
	 * 
	 * @param nodeId
	 * @return
	 */
	public DciValue[] getNodeLastValues(long nodeId)
	{
		DciValue[] values;
		synchronized(cachedDciValues)
		{
			values = cachedDciValues.get(nodeId);
		}
		return values;
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IStructuredContentProvider#getElements(java.lang.Object)
	 */
	@Override
	public Object[] getElements(Object inputElement)
	{
		if (!(inputElement instanceof NetworkMapPage))
			return null;

		List<NetworkMapElement> elements = new ArrayList<NetworkMapElement>(((NetworkMapPage)inputElement).getElements().size());
		for(NetworkMapElement e : ((NetworkMapPage)inputElement).getElements())
			if (!(e instanceof NetworkMapDecoration) && !(e instanceof NetworkMapDCIContainer) && !(e instanceof NetworkMapDCIImage))
				elements.add(e);
		return elements.toArray();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.gef4.zest.core.viewers.IGraphEntityRelationshipContentProvider#getRelationships(java.lang.Object, java.lang.Object)
	 */
	@Override
	public Object[] getRelationships(Object source, Object dest)
	{
	   return page.findLinks((NetworkMapElement)source, (NetworkMapElement)dest).toArray();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IContentProvider#dispose()
	 */
	@Override
	public void dispose()
	{
		syncRunning = false;
		syncThread.interrupt();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IContentProvider#inputChanged(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public void inputChanged(Viewer viewer, Object oldInput, Object newInput)
	{
		synchronized(cachedDciValues)
		{
			cachedDciValues.clear();
		}
		if (newInput instanceof NetworkMapPage)
		{
			page = (NetworkMapPage)newInput;
			synchronized(cachedDciValues)
			{
				for(NetworkMapElement e : page.getElements())
				{
					if (e instanceof NetworkMapObject)
					{
						long id = ((NetworkMapObject)e).getObjectId();
						AbstractObject object = session.findObjectById(id);
						if ((object != null) && (object instanceof AbstractNode))
						{
							cachedDciValues.put(id, null);
						}
					}
				}
			}
		}
		else
		{
			page = null;
		}
	}

	/**
	 * Get map decorations
	 * 
	 * @param inputElement
	 * @return
	 */
	public List<NetworkMapElement> getDecorations(Object inputElement)
	{
		if (!(inputElement instanceof NetworkMapPage))
			return null;

		List<NetworkMapElement> elements = new ArrayList<NetworkMapElement>(((NetworkMapPage)inputElement).getElements().size());
		for(NetworkMapElement e : ((NetworkMapPage)inputElement).getElements())
			if ((e instanceof NetworkMapDecoration) || (e instanceof NetworkMapDCIContainer) || (e instanceof NetworkMapDCIImage))
				elements.add((NetworkMapElement)e);
		return elements;
	}
}
