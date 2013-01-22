/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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

import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.draw2d.Figure;
import org.eclipse.draw2d.FigureListener;
import org.eclipse.draw2d.FreeformLayer;
import org.eclipse.draw2d.Graphics;
import org.eclipse.draw2d.IFigure;
import org.eclipse.draw2d.Layer;
import org.eclipse.draw2d.MouseEvent;
import org.eclipse.draw2d.MouseListener;
import org.eclipse.draw2d.geometry.Dimension;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.zest.core.viewers.GraphViewer;
import org.eclipse.zest.core.viewers.internal.GraphModelEntityRelationshipFactory;
import org.eclipse.zest.core.viewers.internal.IStylingGraphModelFactory;
import org.eclipse.zest.core.widgets.GraphConnection;
import org.eclipse.zest.core.widgets.custom.CGraphNode;
import org.eclipse.zest.core.widgets.zooming.ZoomManager;
import org.netxms.base.GeoLocation;
import org.netxms.client.maps.elements.NetworkMapDecoration;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.networkmaps.Activator;
import org.netxms.ui.eclipse.osm.tools.MapLoader;
import org.netxms.ui.eclipse.osm.tools.Tile;
import org.netxms.ui.eclipse.osm.tools.TileSet;

/**
 * Workaround for bug #244496
 * (https://bugs.eclipse.org/bugs/show_bug.cgi?id=244496) 
 *
 */
public class ExtendedGraphViewer extends GraphViewer
{
	private static final long serialVersionUID = 1L;
	private static final double[] zoomLevels = { 0.10, 0.25, 0.50, 0.75, 1.00, 1.25, 1.50, 1.75, 2.00, 2.50, 3.00, 4.00 };
	
	private BackgroundFigure backgroundFigure;
	private Image backgroundImage = null;
	private TileSet backgroundTiles = null;
	private GeoLocation backgroundLocation = null;
	private int backgroundZoom;
	private IFigure zestRootLayer;
	private MapLoader mapLoader;
	private Layer backgroundLayer;
	private Layer decorationLayer;
	private Layer indicatorLayer;
	private int crosshairX;
	private int crosshairY;
	private Crosshair crosshairFigure;
	private GridFigure gridFigure;
	private int gridSize = 96;
	private boolean snapToGrid = false;
	private MouseListener snapToGridListener;
	private List<NetworkMapDecoration> mapDecorations;
	private Set<NetworkMapDecoration> selectedDecorations = new HashSet<NetworkMapDecoration>();
	private Map<Long, DecorationFigure> decorationFigures = new HashMap<Long, DecorationFigure>();
	private MouseListener backgroundMouseListener;
	
	/**
	 * @param composite
	 * @param style
	 */
	public ExtendedGraphViewer(Composite composite, int style)
	{
		super(composite, style);
		
		mapLoader = new MapLoader(composite.getDisplay());
		getControl().addDisposeListener(new DisposeListener() {
			private static final long serialVersionUID = 1L;

			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				mapLoader.dispose();
			}
		});
		
		backgroundLayer = new FreeformLayer();
		getGraphControl().getRootLayer().add(backgroundLayer, null, 0);
		backgroundFigure = new BackgroundFigure();
		backgroundFigure.setSize(10, 10);
		backgroundLayer.add(backgroundFigure);
		
		decorationLayer = new FreeformLayer();
		getGraphControl().getRootLayer().add(decorationLayer, null, 1);
		
		indicatorLayer = new FreeformLayer();
		getGraphControl().getRootLayer().add(indicatorLayer, null, 2);
		
		getZoomManager().setZoomLevels(zoomLevels);
		
		for(Object f : getGraphControl().getRootLayer().getChildren())
			if (f.getClass().getName().equals("org.eclipse.zest.core.widgets.internal.ZestRootLayer"))
				zestRootLayer = (IFigure)f;

		final Runnable timer = new Runnable() {
			@Override
			public void run()
			{
				if (getGraphControl().isDisposed())
					return;

				if (backgroundLocation != null)
					reloadMapBackground();
				
				if (gridFigure != null)
					gridFigure.setSize(getGraphControl().getRootLayer().getSize());
			}
		};
		
		zestRootLayer.addFigureListener(new FigureListener() {
			@Override
			public void figureMoved(IFigure source)
			{
				getGraphControl().getDisplay().timerExec(-1, timer);
				getGraphControl().getDisplay().timerExec(1000, timer);
			}
		});
		
		backgroundMouseListener = new MouseListener() {
			@Override
			public void mouseReleased(MouseEvent me)
			{
			}
			
			@Override
			public void mousePressed(MouseEvent me)
			{
				clearDecorationSelection();
				ExtendedGraphViewer.this.setSelection(new StructuredSelection(), true);
			}
			
			@Override
			public void mouseDoubleClicked(MouseEvent me)
			{
			}
		};
		backgroundFigure.addMouseListener(backgroundMouseListener);
		
		getGraphControl().addSelectionListener(new SelectionListener() {
			private static final long serialVersionUID = 1L;

			@Override
			public void widgetSelected(SelectionEvent e)
			{
				clearDecorationSelection();
			}
			
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		});
		
		snapToGridListener = new MouseListener() {
			@Override
			public void mouseReleased(MouseEvent me)
			{
				if (snapToGrid && (getGraphControl().getRootLayer().findFigureAt(me.x, me.y) != null))
					alignToGrid(true);
			}
			
			@Override
			public void mousePressed(MouseEvent me)
			{
				for(Object o : getGraphControl().getGraph().getNodes())
				{
					if (!(o instanceof CGraphNode))
						continue;
					CGraphNode n = (CGraphNode)o;
					((ObjectFigure)n.getFigure()).readMovedState();
				}
			}
			
			@Override
			public void mouseDoubleClicked(MouseEvent me)
			{
			}
		};
	}

	/* (non-Javadoc)
	 * @see org.eclipse.zest.core.viewers.GraphViewer#inputChanged(java.lang.Object, java.lang.Object)
	 */
	@Override
	protected void inputChanged(Object input, Object oldInput)
	{
		super.inputChanged(input, oldInput);
		
		decorationLayer.removeAll();
		decorationFigures.clear();
		if ((getContentProvider() instanceof MapContentProvider) && (getLabelProvider() instanceof MapLabelProvider))
		{
			mapDecorations = ((MapContentProvider)getContentProvider()).getDecorations(input);
			if (mapDecorations != null)
			{
				MapLabelProvider lp = (MapLabelProvider)getLabelProvider();
				for(NetworkMapDecoration d : mapDecorations)
				{
					DecorationFigure figure = (DecorationFigure)lp.getFigure(d);
					figure.setLocation(new org.eclipse.draw2d.geometry.Point(d.getX(), d.getY()));
					decorationLayer.add(figure);
					decorationFigures.put(d.getId(), figure);
				}
			}
		}
	}
	
	/**
	 * Set selection on decoration layer. Intended to be called from decoration figure.
	 * 
	 * @param d decoartion object
	 * @param addToExisting if true, add to existing selection
	 */
	protected void setDecorationSelection(NetworkMapDecoration d, boolean addToExisting)
	{
		if (!addToExisting)
		{
			clearDecorationSelection();
			getGraphControl().setSelection(null);
		}
		selectedDecorations.add(d);

		SelectionChangedEvent event = new SelectionChangedEvent(this, getSelection());
		fireSelectionChanged(event);
		firePostSelectionChanged(event);
	}
	
	/**
	 * Clear selection on decoration layer
	 */
	private void clearDecorationSelection()
	{
		for(NetworkMapDecoration d : selectedDecorations)
		{
			DecorationFigure f = decorationFigures.get(d.getId());
			if (f != null)
				f.setSelected(false);
		}
		selectedDecorations.clear();
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.zest.core.viewers.GraphViewer#setSelectionToWidget(java.util.List, boolean)
	 */
	@SuppressWarnings("rawtypes")
	@Override
	protected void setSelectionToWidget(List l, boolean reveal)
	{
		selectedDecorations.clear();
		if (l != null)
		{
			clearDecorationSelection();
			for(Object o : l)
			{
				if (o instanceof NetworkMapDecoration)
				{
					selectedDecorations.add((NetworkMapDecoration)o);
					DecorationFigure f = decorationFigures.get(((NetworkMapDecoration)o).getId());
					if (f != null)
						f.setSelected(true);
				}
			}
		}
		super.setSelectionToWidget(l, reveal);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.zest.core.viewers.AbstractStructuredGraphViewer#getSelectionFromWidget()
	 */
	@SuppressWarnings({ "rawtypes", "unchecked" })
	@Override
	protected List getSelectionFromWidget()
	{
		List selection = super.getSelectionFromWidget();
		for(NetworkMapDecoration d : selectedDecorations)
			selection.add(d);
		return selection;
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
		reloadMapBackground();
	}
	
	/**
	 * Reload map background
	 */
	private void reloadMapBackground()
	{
		final Rectangle controlSize = getGraphControl().getClientArea();
		final org.eclipse.draw2d.geometry.Rectangle rootLayerSize = zestRootLayer.getClientArea();
		final Point mapSize = new Point(Math.min(controlSize.width, rootLayerSize.width), Math.max(controlSize.height, rootLayerSize.height)); 
		ConsoleJob job = new ConsoleJob("Download map tiles", null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final TileSet tiles = mapLoader.getAllTiles(mapSize, backgroundLocation, MapLoader.TOP_LEFT, backgroundZoom, false);
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						if ((backgroundLocation == null) || getGraphControl().isDisposed())
							return;
						
						final org.eclipse.draw2d.geometry.Rectangle rootLayerSize = zestRootLayer.getClientArea();
						if ((mapSize.x != rootLayerSize.width) || (mapSize.y != rootLayerSize.height))
							return;

						if (backgroundImage != null)
						{
							backgroundImage.dispose();
							backgroundImage = null;
						}
						backgroundTiles = tiles;
						backgroundFigure.setSize(mapSize.x, mapSize.y);
						backgroundFigure.repaint();					
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
	 * Show crosshair at given location
	 * 
	 * @param x
	 * @param y
	 */
	public void showCrosshair(int x, int y)
	{
		if (crosshairFigure == null)
		{
			crosshairFigure = new Crosshair();
			indicatorLayer.add(crosshairFigure);
			crosshairFigure.setSize(getGraphControl().getRootLayer().getSize());
		}
		crosshairX = x;
		crosshairY = y;
		crosshairFigure.repaint();
	}
	
	/**
	 * Hide crosshair
	 */
	public void hideCrosshair()
	{
		if (crosshairFigure != null)
		{
			indicatorLayer.remove(crosshairFigure);
			crosshairFigure = null;
		}
	}
	
	/**
	 * Show/hide grid
	 * 
	 * @param show
	 */
	public void showGrid(boolean show)
	{
		if (show)
		{
			if (gridFigure == null)
			{
				gridFigure = new GridFigure();
				backgroundLayer.add(gridFigure, null, 1);
				gridFigure.setSize(getGraphControl().getRootLayer().getSize());
			}
		}
		else
		{
			if (gridFigure != null)
			{
				backgroundLayer.remove(gridFigure);
				gridFigure = null;
			}
		}
	}
	
	/**
	 * @return
	 */
	public boolean isGridVisible()
	{
		return gridFigure != null;
	}
	
	/**
	 * Align objects to grid
	 */
	public void alignToGrid(boolean movedOnly)
	{
		for(Object o : getGraphControl().getGraph().getNodes())
		{
			if (!(o instanceof CGraphNode))
				continue;
			CGraphNode n = (CGraphNode)o;
			if (movedOnly)
			{
				ObjectFigure f = (ObjectFigure)n.getFigure();
				if (!f.readMovedState() || !f.isElementSelected())
					continue;
			}
			org.eclipse.draw2d.geometry.Point p = n.getLocation();
			Dimension size = n.getSize();

			int dx = p.x % gridSize;
			if (dx < gridSize / 2)
				dx = - dx;
			else
				dx = gridSize - dx;
			dx += (gridSize - size.width) / 2;
			
			int dy = p.y % gridSize;
			if (dy < gridSize / 2)
				dy = - dy;
			else
				dy = gridSize - dy;
			dy += (gridSize - size.height) / 2;
			
			n.setLocation(p.x + dx, p.y + dy);
		}
	}
	
	/**
	 * Set snap-to-grid flag
	 * 
	 * @param snap
	 */
	public void setSnapToGrid(boolean snap)
	{
		if (snap == snapToGrid)
			return;
		
		snapToGrid = snap;
		if (snap)
		{
			getGraphControl().getLightweightSystem().getRootFigure().addMouseListener(snapToGridListener);			
		}
		else
		{
			getGraphControl().getLightweightSystem().getRootFigure().removeMouseListener(snapToGridListener);			
		}
	}
	
	/**
	 * Get snap-to-grid flag
	 * 
	 * @return
	 */
	public boolean isSnapToGrid()
	{
		return snapToGrid;
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.zest.core.viewers.GraphViewer#getFactory()
	 */
	@Override
	protected IStylingGraphModelFactory getFactory()
	{
		return new GraphModelEntityRelationshipFactory(this) {
			@Override
			public void styleConnection(GraphConnection conn)
			{
				// do nothing here - this will override default behavior
				// to make connection curved if there are multiple connections
				// between nodes
			}
		};
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
			if (backgroundTiles != null)
				drawTiles(gc, backgroundTiles);
		}
		
		/**
		 * @param tileSet
		 */
		private void drawTiles(Graphics gc, TileSet tileSet)
		{
			if ((tileSet == null) || (tileSet.tiles == null) || (tileSet.tiles.length == 0))
				return;

			final Tile[][] tiles = tileSet.tiles;

			Dimension size = backgroundFigure.getSize();
			backgroundImage = new Image(getGraphControl().getDisplay(), size.width, size.height);

			int x = tileSet.xOffset;
			int y = tileSet.yOffset;
			for(int i = 0; i < tiles.length; i++)
			{
				for(int j = 0; j < tiles[i].length; j++)
				{
					gc.drawImage(tiles[i][j].getImage(), x, y);
					x += 256;
					if (x >= size.width)
					{
						x = tileSet.xOffset;
						y += 256;
					}
				}
			}
		}		
	}
	
	/**
	 * Crosshair
	 */
	private class Crosshair extends Figure
	{
		/**
		 * 
		 */
		public Crosshair()
		{
			setOpaque(false);
		}
		
		/* (non-Javadoc)
		 * @see org.eclipse.draw2d.Figure#paintFigure(org.eclipse.draw2d.Graphics)
		 */
		@Override
		protected void paintFigure(Graphics gc)
		{
			gc.setLineStyle(Graphics.LINE_DOT);
			Dimension size = getSize();
			gc.drawLine(0, crosshairY, size.width, crosshairY);
			gc.drawLine(crosshairX, 0, crosshairX, size.height);
		}
	}

	/**
	 * Grid
	 */
	private class GridFigure extends Figure
	{
		/**
		 * 
		 */
		public GridFigure()
		{
			setOpaque(false);
			addMouseListener(backgroundMouseListener);
		}
		
		/* (non-Javadoc)
		 * @see org.eclipse.draw2d.Figure#paintFigure(org.eclipse.draw2d.Graphics)
		 */
		@Override
		protected void paintFigure(Graphics gc)
		{
			gc.setLineStyle(Graphics.LINE_DOT);
			Dimension size = getSize();
			for(int x = gridSize; x < size.width; x += gridSize)
				gc.drawLine(x, 0, x, size.height);
			for(int y = gridSize; y < size.height; y += gridSize)
				gc.drawLine(0, y, size.width, y);
		}
	}
}
