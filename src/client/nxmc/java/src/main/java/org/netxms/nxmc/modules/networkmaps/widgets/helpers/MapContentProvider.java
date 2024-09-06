/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.networkmaps.widgets.helpers;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Set;
import org.eclipse.gef4.zest.core.viewers.IGraphEntityRelationshipContentProvider;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.maps.MapObjectDisplayMode;
import org.netxms.client.maps.NetworkMapLink;
import org.netxms.client.maps.NetworkMapPage;
import org.netxms.client.maps.elements.NetworkMapDCIContainer;
import org.netxms.client.maps.elements.NetworkMapDCIImage;
import org.netxms.client.maps.elements.NetworkMapDecoration;
import org.netxms.client.maps.elements.NetworkMapElement;
import org.netxms.client.maps.elements.NetworkMapObject;
import org.netxms.client.maps.elements.NetworkMapTextBox;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Registry;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Content provider for map
 */
public class MapContentProvider implements IGraphEntityRelationshipContentProvider
{
   private static final Logger logger = LoggerFactory.getLogger(MapContentProvider.class);

	private ExtendedGraphViewer viewer;
	private NetworkMapPage page;
	private Map<Long, DciValue[]> cachedDciValues = new HashMap<Long, DciValue[]>();
   private NXCSession session = Registry.getSession();
	private Thread syncThread = null;
	private volatile boolean syncRunning = true;
	private MapLabelProvider labelProvider;

	/**
	 * Constructor
	 * @param labelProvider 
	 */
	public MapContentProvider(ExtendedGraphViewer viewer, MapLabelProvider labelProvider)
	{
	   this.labelProvider = labelProvider;
		this.viewer = viewer;
		final Display display = viewer.getControl().getDisplay();
      syncThread = new Thread(() -> syncData(display), "MapContentProvider");
		syncThread.setDaemon(true);
		syncThread.start();
	}

	/**
    * Synchronize data in background
    */
	private void syncData(Display display)
	{
		try
		{
			Thread.sleep(1000);
		}
      catch(InterruptedException e)
		{
			return;
		}

		while(syncRunning)
		{
         Set<Long> dataSyncSet = new HashSet<Long>();
			synchronized(cachedDciValues)
			{
            if ((labelProvider.getObjectFigureType() == MapObjectDisplayMode.LARGE_LABEL) && !cachedDciValues.isEmpty())
               dataSyncSet.addAll(cachedDciValues.keySet());
         }
         if (!dataSyncSet.isEmpty())
         {
            try
            {
               final Map<Long, DciValue[]> values = session.getTooltipLastValues(cachedDciValues.keySet());
               display.asyncExec(new Runnable() {
                  @Override
                  public void run()
                  {
                     if (viewer.getControl().isDisposed())
                        return;

                     synchronized(cachedDciValues)
                     {
                        cachedDciValues.putAll(values);

                        if (labelProvider.getObjectFigureType() == MapObjectDisplayMode.LARGE_LABEL)
                        {
                           for(Entry<Long, DciValue[]> e : cachedDciValues.entrySet())
                           {
                              if ((e.getValue() == null) || (e.getValue().length == 0))
                                 continue;
                              NetworkMapObject o = page.findObjectElement(e.getKey());
                              if (o != null)
                                 viewer.refresh(o);
                           }
                        }
                        if (page != null && page.getLinks() != null)
                        {
                           for(NetworkMapLink e : page.getLinks())
                           {
                              if (!e.hasDciData())
                                 continue;
                              viewer.refresh(e);
                           }
                        }
                        if (page != null && page.getElements() != null)
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
            catch(Exception e)
            {
               logger.error("Exception in map data synchronization task", e);
            }
         }
			try
			{
				Thread.sleep(30000); 
			}
         catch(InterruptedException e)
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
		synchronized(cachedDciValues)
		{
         return cachedDciValues.get(nodeId);
		}
	}

   /**
    * @see org.eclipse.jface.viewers.IStructuredContentProvider#getElements(java.lang.Object)
    */
	@Override
	public Object[] getElements(Object inputElement)
	{
		if (!(inputElement instanceof NetworkMapPage))
			return null;

		List<NetworkMapElement> elements = new ArrayList<NetworkMapElement>(((NetworkMapPage)inputElement).getElements().size());
		for(NetworkMapElement e : ((NetworkMapPage)inputElement).getElements())
			if (!(e instanceof NetworkMapDecoration) && !(e instanceof NetworkMapDCIContainer) && !(e instanceof NetworkMapDCIImage) && !(e instanceof NetworkMapTextBox))
				elements.add(e);
		return elements.toArray();
	}

   /**
    * @see org.eclipse.gef4.zest.core.viewers.IGraphEntityRelationshipContentProvider#getRelationships(java.lang.Object, java.lang.Object)
    */
	@Override
	public Object[] getRelationships(Object source, Object dest)
	{
	   return page.findLinks((NetworkMapElement)source, (NetworkMapElement)dest).toArray();
	}

   /**
    * @see org.eclipse.jface.viewers.IContentProvider#dispose()
    */
	@Override
	public void dispose()
	{
		syncRunning = false;
		syncThread.interrupt();
	}

   /**
    * @see org.eclipse.jface.viewers.IContentProvider#inputChanged(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
	@Override
	public void inputChanged(Viewer viewer, Object oldInput, Object newInput)
	{
      if (oldInput instanceof NetworkMapPage && newInput instanceof NetworkMapPage)
      {
         page = (NetworkMapPage)newInput;
         NetworkMapPage oldPage = (NetworkMapPage)oldInput;
         synchronized(cachedDciValues)
         {
            for(NetworkMapElement e : oldPage.getElements())
            {
               if (e instanceof NetworkMapObject)
               {
                  boolean found = false;
                  long id = ((NetworkMapObject)e).getObjectId();                  
                  for(NetworkMapElement e2 : page.getElements())
                  {
                     if (e2 instanceof NetworkMapObject)
                     {
                        if (id == ((NetworkMapObject)e2).getObjectId())
                        {
                           found = true;
                           break;
                        }
                     }
                  }
                  if (!found)
                  {
                     cachedDciValues.remove(id);
                  }
                  
               }
            }
         }
         
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
                  AbstractObject object = session.findObjectById(id, true);
						if ((object != null) && (object instanceof AbstractNode) && !cachedDciValues.containsKey(id))
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
      {
			if ((e instanceof NetworkMapDecoration) || (e instanceof NetworkMapDCIContainer) ||
			    (e instanceof NetworkMapDCIImage) || (e instanceof NetworkMapTextBox))
         {
				elements.add(e);
         }
      }
		return elements;
	}
}
