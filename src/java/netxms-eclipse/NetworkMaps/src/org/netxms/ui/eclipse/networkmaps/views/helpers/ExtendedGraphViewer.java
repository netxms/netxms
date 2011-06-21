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

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.draw2d.Figure;
import org.eclipse.draw2d.Graphics;
import org.eclipse.jface.action.Action;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.events.ControlListener;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.zest.core.viewers.GraphViewer;
import org.eclipse.zest.core.viewers.internal.ZoomManager;
import org.netxms.client.GeoLocation;
import org.netxms.ui.eclipse.imagelibrary.Activator;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.osm.tools.MapLoader;
import org.netxms.ui.eclipse.osm.tools.TileSet;

/**
 * Workaround for bug #244496
 * (https://bugs.eclipse.org/bugs/show_bug.cgi?id=244496) 
 *
 */
@SuppressWarnings("restriction")
public class ExtendedGraphViewer extends GraphViewer
{
	private static final double[] zoomLevels = { 0.10, 0.25, 0.50, 0.75, 1.00, 1.25, 1.50, 1.75, 2.00, 2.50, 3.00, 4.00 };
	
	private BackgroundFigure backgroundFigure;
	private Image backgroundImage = null;
	private GeoLocation backgroundLocation;
	private int backgroundZoom;
	
	/**
	 * @param composite
	 * @param style
	 */
	public ExtendedGraphViewer(Composite composite, int style)
	{
		super(composite, style);
		getZoomManager().setZoomLevels(zoomLevels);
		backgroundFigure = new BackgroundFigure();
		backgroundFigure.setSize(10, 10);
		getGraphControl().getRootLayer().add(backgroundFigure, 0);
		getGraphControl().addControlListener(new ControlListener() {
			@Override
			public void controlResized(ControlEvent e)
			{
			}
			
			@Override
			public void controlMoved(ControlEvent e)
			{
				// TODO Auto-generated method stub
				
			}
		});
	}
	
	/**
	 * Set background image for graph
	 * 
	 * @param image new image or null to clear background
	 */
	public void setBackgroundImage(Image image)
	{
		backgroundImage = image;
		if (image != null)
		{
			Rectangle r = image.getBounds();
			backgroundFigure.setSize(r.width, r.height);
		}
		else
		{
			backgroundFigure.setSize(10, 10);
		}
		backgroundLocation = null;
		getGraphControl().redraw();
	}
	
	/**
	 * @param location
	 * @param zoom
	 */
	public void setBackgroundImage(final GeoLocation location, final int zoom)
	{
		if ((backgroundLocation != null) && (backgroundImage != null))
			backgroundImage.dispose();
		
		backgroundImage = null;
		backgroundLocation = location;
		backgroundZoom = zoom;
		backgroundFigure.setSize(10, 10);
		getGraphControl().redraw();
		
		final Point mapSize = getGraphControl().getSize();
		ConsoleJob job = new ConsoleJob("Download map tiles", null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final TileSet tiles = MapLoader.getAllTiles(mapSize, location, MapLoader.TOP_LEFT, zoom);
				Display.getDefault().asyncExec(new Runnable() {
					@Override
					public void run()
					{
						backgroundFigure.setSize(mapSize.x, mapSize.y);
						drawTiles(tiles);
						getGraphControl().redraw();
					}
				});
			}

			@Override
			protected String getErrorMessage()
			{
				return "Cannot download map tiles";
			}
		};
		job.setUser(false);
		job.start();
	}

	/**
	 * @param tileSet
	 */
	private void drawTiles(TileSet tileSet)
	{
		if ((backgroundLocation != null) && (backgroundImage != null))
			backgroundImage.dispose();
		
		if ((tileSet == null) || (tileSet.tiles == null) || (tileSet.tiles.length == 0))
		{
			backgroundImage = null;
			return;
		}

		final Image[][] tiles = tileSet.tiles;

		Point size = getGraphControl().getSize();
		backgroundImage = new Image(getGraphControl().getDisplay(), size.x, size.y);
		GC gc = new GC(backgroundImage);

		int x = tileSet.xOffset;
		int y = tileSet.yOffset;
		for(int i = 0; i < tiles.length; i++)
		{
			for(int j = 0; j < tiles[i].length; j++)
			{
				gc.drawImage(tiles[i][j], x, y);
				x += 256;
				if (x >= size.x)
				{
					x = tileSet.xOffset;
					y += 256;
				}
			}
		}

		gc.dispose();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.StructuredViewer#internalRefresh(java.lang.Object, boolean)
	 */
	@Override
	protected void internalRefresh(Object element, boolean updateLabels)
	{
		if (getInput() == null)
		{
			return;
		}
		if (element == getInput())
		{
			getFactory().refreshGraph(getGraphControl());
		} 
		else
		{
			getFactory().refresh(getGraphControl(), element, updateLabels);
		}
	}
	
	/**
	 * Zoom to next level
	 */
	public void zoomIn()
	{
		getZoomManager().zoomIn();
	}
	
	/**
	 * Zoom to previous level
	 */
	public void zoomOut()
	{
		getZoomManager().zoomOut();
	}
	
	/**
	 * Create zoom actions
	 * @return
	 */
	public Action[] createZoomActions()
	{
		final ZoomManager zoomManager = getZoomManager();
		final Action[] actions = new Action[zoomLevels.length];
		for(int i = 0; i < zoomLevels.length; i++)
		{
			actions[i] = new ZoomAction(zoomLevels[i], zoomManager);
			if (zoomLevels[i] == 1.00)
				actions[i].setChecked(true);
		}
		return actions;
	}

	/**
	 * Additional figure used to display custom background for graph.
	 */
	private class BackgroundFigure extends Figure
	{
		/* (non-Javadoc)
		 * @see org.eclipse.draw2d.Figure#paintFigure(org.eclipse.draw2d.Graphics)
		 */
		@Override
		protected void paintFigure(Graphics gc)
		{
			if (backgroundImage != null)
				gc.drawImage(backgroundImage, 0, 0);
		}
	}
}
