/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Raden Solutions
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

import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.draw2d.AbstractBorder;
import org.eclipse.draw2d.Figure;
import org.eclipse.draw2d.FreeformLayer;
import org.eclipse.draw2d.Graphics;
import org.eclipse.draw2d.IFigure;
import org.eclipse.draw2d.Label;
import org.eclipse.draw2d.Layer;
import org.eclipse.draw2d.MouseEvent;
import org.eclipse.draw2d.MouseListener;
import org.eclipse.draw2d.MouseMotionListener;
import org.eclipse.draw2d.SWTEventDispatcher;
import org.eclipse.draw2d.SWTGraphics;
import org.eclipse.draw2d.ScalableFigure;
import org.eclipse.draw2d.ToolTipHelper;
import org.eclipse.draw2d.geometry.Dimension;
import org.eclipse.draw2d.geometry.Insets;
import org.eclipse.gef4.zest.core.viewers.GraphViewer;
import org.eclipse.gef4.zest.core.viewers.internal.GraphModelEntityRelationshipFactory;
import org.eclipse.gef4.zest.core.viewers.internal.IStylingGraphModelFactory;
import org.eclipse.gef4.zest.core.widgets.GraphConnection;
import org.eclipse.gef4.zest.core.widgets.IDecorationFigure;
import org.eclipse.gef4.zest.core.widgets.IDecorationLayer;
import org.eclipse.gef4.zest.core.widgets.ZestStyles;
import org.eclipse.gef4.zest.core.widgets.custom.CGraphNode;
import org.eclipse.gef4.zest.core.widgets.zooming.ZoomListener;
import org.eclipse.gef4.zest.core.widgets.zooming.ZoomManager;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.base.GeoLocation;
import org.netxms.client.maps.NetworkMapLink;
import org.netxms.client.maps.elements.NetworkMapDCIContainer;
import org.netxms.client.maps.elements.NetworkMapDCIImage;
import org.netxms.client.maps.elements.NetworkMapDecoration;
import org.netxms.client.maps.elements.NetworkMapElement;
import org.netxms.client.maps.elements.NetworkMapTextBox;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.networkmaps.algorithms.ManualLayout;
import org.netxms.nxmc.modules.worldmap.tools.MapLoader;
import org.netxms.nxmc.modules.worldmap.tools.Tile;
import org.netxms.nxmc.modules.worldmap.tools.TileSet;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.ThemeEngine;
import org.netxms.nxmc.tools.ColorCache;
import org.xnap.commons.i18n.I18n;

/**
 * Workaround for bug #244496
 * (https://bugs.eclipse.org/bugs/show_bug.cgi?id=244496) 
 */
public class ExtendedGraphViewer extends GraphViewer
{
   public static final double[] zoomLevels = { 0.10, 0.25, 0.50, 0.75, 1.00, 1.25, 1.50, 1.75, 2.00 };

   private I18n i18n = LocalizationHelper.getI18n(ExtendedGraphViewer.class);
   private View view;
   private FigureChangeCallback moveCallback;
   private ExtendedSWTEventDispatcher eventDispatcher;
	private BackgroundFigure backgroundFigure;
	private Image backgroundImage = null;
   private TileSet backgroundTiles = null;
	private GeoLocation backgroundLocation = null;
	private int backgroundZoom;
	private MapLoader mapLoader;
	private Layer backgroundLayer;
	private Layer decorationLayer;
	private Layer indicatorLayer;
   private Layer controlLayer;
	private int crosshairX;
	private int crosshairY;
	private Crosshair crosshairFigure;
	private GridFigure gridFigure;
	private int gridSize = 96;
	private boolean snapToGrid = false;
	private MouseListener snapToGridListener;
	private List<NetworkMapElement> mapDecorations;
	private Set<NetworkMapElement> selectedDecorations = new HashSet<NetworkMapElement>();
	private Map<Long, DecorationLayerAbstractFigure> decorationFigures = new HashMap<Long, DecorationLayerAbstractFigure>();
	private ColorCache colors;
	private Image iconBack;
	private OverlayButton backButton = null;
	private boolean draggingEnabled = true;
   private boolean centeredBackground = false;
   private boolean fitBackground = false;
   private boolean dragStarted = false;
   private boolean showMapSize = false;
   private volatile int blockRefresh;
   private Point rightClickLocation = null;
   private boolean backgroundDragActive = false;
   private org.eclipse.draw2d.geometry.Point backgroundDragLast = null;

	/**
	 * @param composite
	 * @param style
	 */
   public ExtendedGraphViewer(Composite composite, int style, View view, FigureChangeCallback moveCallback)
	{
      super(composite, style | ZestStyles.GESTURES_DISABLED);

      this.view = view;
      this.moveCallback = moveCallback;

      eventDispatcher = new ExtendedSWTEventDispatcher();
      graph.getLightweightSystem().setEventDispatcher(eventDispatcher);

		colors = new ColorCache(graph);

		final ScalableFigure rootLayer = graph.getRootLayer();

      iconBack = ResourceManager.getImage("icons/netmap/back.png");

		mapLoader = new MapLoader(composite.getDisplay());
		graph.addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				mapLoader.dispose();
				iconBack.dispose();
            if (backgroundTiles != null)
               backgroundTiles.dispose();
			}
		});

      backgroundLayer = new FreeformLayer();
      rootLayer.add(backgroundLayer, null, 0);           
      backgroundFigure = new BackgroundFigure();
      backgroundFigure.setSize(-1, -1);
      backgroundLayer.add(backgroundFigure);
      backgroundLayer.addMouseMotionListener(new MouseMotionListener() {
         @Override
         public void mouseMoved(MouseEvent me)
         {
         }

         @Override
         public void mouseHover(MouseEvent me)
         {
         }

         @Override
         public void mouseExited(MouseEvent me)
         {
         }

         @Override
         public void mouseEntered(MouseEvent me)
         {
         }

         @Override
         public void mouseDragged(MouseEvent me)
         {
            if (!backgroundDragActive)
               return;

            int dx = backgroundDragLast.x - me.x; // movement in figure coordinates
            int dy = backgroundDragLast.y - me.y;
            if ((dx == 0) && (dy == 0))
               return;

            org.eclipse.draw2d.geometry.Point newViewLocation = graph.getViewport().getViewLocation().getCopy().translate(new org.eclipse.draw2d.geometry.Point(dx, dy));
            if (newViewLocation.x < 0)
               newViewLocation.x = 0;
            if (newViewLocation.y < 0)
               newViewLocation.y = 0;

            Dimension scaledSize = backgroundLayer.getSize();
            scaledSize.performScale(getZoom());
            org.eclipse.draw2d.geometry.Rectangle viewportArea = graph.getViewport().getClientArea();
            if (newViewLocation.x > scaledSize.width - viewportArea.width)
               newViewLocation.x = scaledSize.width - viewportArea.width;
            if (newViewLocation.y > scaledSize.height - viewportArea.height)
               newViewLocation.y = scaledSize.height - viewportArea.height;

            graph.getViewport().setViewLocation(newViewLocation);
            backgroundDragLast.setLocation(me.x, me.y);
         }
      });
      backgroundLayer.addMouseListener(new MouseListener() {
         @Override
         public void mouseReleased(MouseEvent me)
         {
            if (me.button != 1)
               return;

            backgroundDragActive = false;
            backgroundDragLast = null;
            graph.setCursor(null);
         }

         @Override
         public void mousePressed(MouseEvent me)
         {
            if (me.button != 1)
               return;

            backgroundDragActive = true;
            backgroundDragLast = new org.eclipse.draw2d.geometry.Point(me.x, me.y);
            graph.setCursor(graph.getDisplay().getSystemCursor(SWT.CURSOR_SIZEALL));
         }

         @Override
         public void mouseDoubleClicked(MouseEvent me)
         {
         }
      });

		decorationLayer = new FreeformLayer();
		decorationLayer.setOpaque(false);
		rootLayer.add(decorationLayer, null, 1);

		indicatorLayer = new FreeformLayer();
		rootLayer.add(indicatorLayer, null, 2);

      controlLayer = new FreeformLayer();
      rootLayer.add(controlLayer, null);

		getZoomManager().setZoomLevels(zoomLevels);
		getZoomManager().addZoomListener(new ZoomListener() {         
         @Override
         public void zoomChanged(double zoom)
         {
            ExtendedGraphViewer.this.refresh(true);
         }
      });

      graph.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				clearDecorationSelection(false);
			}
		});

		MouseListener backgroundMouseListener = new MouseListener() {
			@Override
			public void mouseReleased(MouseEvent me)
			{
			}

			@Override
			public void mousePressed(MouseEvent me)
			{
			   if (me.button == 3)
            {
               org.eclipse.draw2d.geometry.Point newLocation = me.getLocation().getTranslated(graph.getHorizontalBar().getSelection(), graph.getVerticalBar().getSelection()).scale(1/getZoom());
               rightClickLocation = new Point(newLocation.x, newLocation.y);
            }
				org.eclipse.draw2d.geometry.Point mousePoint = new org.eclipse.draw2d.geometry.Point(me.x, me.y);
				graph.getRootLayer().translateToRelative(mousePoint);
				IFigure figureUnderMouse = graph.getFigureAt(mousePoint.x, mousePoint.y);
				if ((figureUnderMouse == null) || 
				    !(figureUnderMouse instanceof DecorationLayerAbstractFigure))
				{
					if ((me.getState() & SWT.MOD1) == 0) 
					{
						clearDecorationSelection(true);
					}
					return;
				}
			}
			
			@Override
			public void mouseDoubleClicked(MouseEvent me)
			{
			}
		};
      graph.getLightweightSystem().getRootFigure().addMouseListener(backgroundMouseListener);
      graph.getZestRootLayer().addMouseListener(backgroundMouseListener);

		snapToGridListener = new MouseListener() {
			@Override
			public void mouseReleased(MouseEvent me)
			{
            org.eclipse.draw2d.geometry.Point mousePoint = new org.eclipse.draw2d.geometry.Point(me.x, me.y);
            graph.getRootLayer().translateToParent(mousePoint);  //Scales mouse coordinates to zoomed map coordinates
				if (snapToGrid && (graph.getRootLayer().findFigureAt(mousePoint.x, mousePoint.y) != null))
					alignToGrid(true);
			}

			@Override
			public void mousePressed(MouseEvent me)
			{
				for(Object o : graph.getNodes())
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

		if (moveCallback != null )
		{
		   MouseListener moveMouseListener = new MouseListener() {
            @Override
            public void mouseReleased(MouseEvent me)
            {
               if (dragStarted)
               {
                  if (!(getLayoutAlgorithm() instanceof ManualLayout))
                  {
                     dragStarted = false;
                     unblockRefresh();
                     return;
                  }
            
                  org.eclipse.draw2d.geometry.Point mousePoint = new org.eclipse.draw2d.geometry.Point(me.x, me.y);
                  graph.getRootLayer().translateToParent(mousePoint); //Scales mouse coordinates to zoomed map coordinates    
                  IFigure figure = graph.getRootLayer().findFigureAt(mousePoint.x, mousePoint.y);
                  if (figure != null)
                  {
                     if (figure instanceof Label || figure instanceof ObjectFigure || figure instanceof NodeLastValuesFigure)
                     {
                        for (Object object : graph.getSelection())
                        {
                           figure = ((CGraphNode)object).getFigure();
                           moveCallback.onMove(((ObjectFigure)figure).getMapElement());
                        }
                     }
                  }
                  unblockRefresh();
               }
               dragStarted = false;
            }
            
            @Override
            public void mousePressed(MouseEvent me)
            {
            }
            
            @Override
            public void mouseDoubleClicked(MouseEvent me)
            {
            }
         };
         MouseMotionListener moveMouseMoutionListener = new MouseMotionListener() {
            
            @Override
            public void mouseMoved(MouseEvent me)
            {
            }
            
            @Override
            public void mouseHover(MouseEvent me)
            {
            }
            
            @Override
            public void mouseExited(MouseEvent me)
            {
            }
            
            @Override
            public void mouseEntered(MouseEvent me)
            {
            }
            
            @Override
            public void mouseDragged(MouseEvent me)
            {
               if (!dragStarted)
                  blockRefresh();
               dragStarted = true;
            }
         };
         graph.getZestRootLayer().addMouseListener(moveMouseListener);
         graph.getZestRootLayer().addMouseMotionListener(moveMouseMoutionListener);
		}
	}   

   /**
    * Block map from refresh
    */
   public void blockRefresh()
   {
     blockRefresh++;
   }

	/**
	 * Unblock map from refresh
	 */
	public void unblockRefresh()
   {
	   blockRefresh--;
   }
	

	/**
	 * Reset refresh bloack
	 */
   public void resetRefreshBlock()
   {
      blockRefresh = 0;
   } 

   /**
	 * Update decoration figure
	 * 
	 * @param d map decoration element
	 */
	public void updateDecorationFigure(NetworkMapElement d)
	{
		DecorationLayerAbstractFigure figure = decorationFigures.get(d.getId());
		if (figure != null)
			figure.refresh();
	}

   /**
    * @see org.eclipse.gef4.zest.core.viewers.GraphViewer#inputChanged(java.lang.Object, java.lang.Object)
    */
	@Override
	protected void inputChanged(Object input, Object oldInput)
	{
		boolean dynamicLayoutEnabled = graph.isDynamicLayoutEnabled();
		graph.setDynamicLayout(false);
		super.inputChanged(input, oldInput);
		graph.setDynamicLayout(dynamicLayoutEnabled);

		decorationLayer.removeAll();
		decorationFigures.clear();
		if ((getContentProvider() instanceof MapContentProvider) && (getLabelProvider() instanceof MapLabelProvider))
		{
			mapDecorations = ((MapContentProvider)getContentProvider()).getDecorations(input);
			if (mapDecorations != null)
			{
				MapLabelProvider lp = (MapLabelProvider)getLabelProvider();
				for(NetworkMapElement d : mapDecorations)
				{
			      DecorationLayerAbstractFigure figure = (DecorationLayerAbstractFigure)lp.getFigure(d);
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
   protected void setDecorationSelection(NetworkMapElement d, boolean addToExisting)
	{
		if (!addToExisting)
		{
			clearDecorationSelection(false);
			graph.setSelection(null);
		}
		selectedDecorations.add(d);

		SelectionChangedEvent event = new SelectionChangedEvent(this, getSelection());
		fireSelectionChanged(event);
		firePostSelectionChanged(event);
	}
	
	/**
	 * Clear selection on decoration layer
	 */
	private void clearDecorationSelection(boolean sendEvent)
	{
		if (selectedDecorations.size() == 0)
			return;

		for(NetworkMapElement d : selectedDecorations)
		{
		   DecorationLayerAbstractFigure f = decorationFigures.get(d.getId());
			if (f != null)
				f.setSelected(false);
		}
		selectedDecorations.clear();

		if (sendEvent)
		{
			SelectionChangedEvent event = new SelectionChangedEvent(this, getSelection());
			fireSelectionChanged(event);
			firePostSelectionChanged(event);
		}
	}

   /**
    * @see org.eclipse.gef4.zest.core.viewers.GraphViewer#setSelectionToWidget(java.util.List, boolean)
    */
	@SuppressWarnings("rawtypes")
	@Override
	protected void setSelectionToWidget(List l, boolean reveal)
	{
		selectedDecorations.clear();
		if (l != null)
		{
			clearDecorationSelection(false);
			for(Object o : l)
			{
            if ((o instanceof NetworkMapDecoration) || (o instanceof NetworkMapDCIContainer) || 
                (o instanceof NetworkMapDCIImage) || (o instanceof NetworkMapTextBox))
				{
					selectedDecorations.add((NetworkMapElement)o);
					DecorationLayerAbstractFigure f = decorationFigures.get(((NetworkMapElement)o).getId());
					if (f != null)
						f.setSelected(true);
				}
			}
		}
		super.setSelectionToWidget(l, reveal);
	}

   /**
    * @see org.eclipse.gef4.zest.core.viewers.AbstractStructuredGraphViewer#getSelectionFromWidget()
    */
	@SuppressWarnings({ "rawtypes", "unchecked" })
	@Override
	protected List getSelectionFromWidget()
	{
		List selection = super.getSelectionFromWidget();
		for(NetworkMapElement d : selectedDecorations)
			selection.add(d);
		return selection;
	}

	/**
	 * Set background color for graph
	 * 
	 * @param backgroundColor new background color
	 */
	public void setBackgroundColor(RGB color)
	{
		Color c = colors.create(color);
		graph.setBackground(c);
		graph.getLightweightSystem().getRootFigure().setBackgroundColor(c);		
	}

   /**
    * Set background image for graph
    * 
    * @param image new image or null to clear background
    * @param centered true to center background image
    * @param fit 
    */
   public void setBackgroundImage(Image image, boolean centered, boolean fit)
   {
      if (backgroundImage == image && centeredBackground == centered && fitBackground == fit) //nothing to update
         return;

      backgroundImage = image;
      centeredBackground = centered;
      fitBackground = fit;
      backgroundLocation = null;
      backgroundTiles = null;
      graph.redraw();
   }

	/**
	 * @param location
	 * @param zoom
	 */
	public void setBackgroundImage(final GeoLocation location, final int zoom)
	{
	   if ((backgroundLocation != null) && backgroundLocation.equals(location) && (zoom == backgroundZoom)) //nothing to update
	      return;
	   
      if ((backgroundLocation != null) && (backgroundImage != null))
			backgroundImage.dispose();

		backgroundImage = null;
      backgroundTiles = null;
      centeredBackground = false;
      fitBackground = false;
		backgroundLocation = location;
		backgroundZoom = zoom;
		graph.redraw();
		reloadMapBackground();
	}
	
	/**
	 * Reload map background
	 */
	private void reloadMapBackground()
	{
      Dimension controlSize = backgroundFigure.getSize();
		final Point mapSize = new Point(controlSize.width, controlSize.height); 
      Job job = new Job(i18n.tr("Download map tiles"), view) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
			{
				final TileSet tiles = mapLoader.getAllTiles(mapSize, backgroundLocation, MapLoader.TOP_LEFT, backgroundZoom, false);
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						if ((backgroundLocation == null) || graph.isDisposed())
							return;

						if (backgroundImage != null)
                  {
                     backgroundImage.dispose();
                     backgroundImage = null;
                  }
                  if (backgroundTiles != null)
                  {
                     backgroundTiles.dispose();
                  }
                  backgroundTiles = tiles;
						backgroundFigure.repaint();
						//drawTiles(tiles);
						//graph.redraw();
					}
				});
			}

			@Override
			protected String getErrorMessage()
			{
            return i18n.tr("Cannot download map tiles");
			}
		};
		job.setUser(false);
		job.start();
	}

   /**
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
			getFactory().refreshGraph(graph);
		} 
		else
		{
			getFactory().refresh(graph, element, updateLabels);
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
    * Zoom to fit available screen area
    */
   public void zoomFit()
   {
      getZoomManager().setZoom(1.0);
      Rectangle visibleArea = getGraphControl().getClientArea();
      org.eclipse.draw2d.geometry.Rectangle mapArea = getGraphControl().getRootLayer().getBounds();
      double dx = (double)visibleArea.width / (double)mapArea.width;
      double dy = (double)visibleArea.height / (double)mapArea.height;
      getZoomManager().setZoom(Math.min(dx, dy));
   }
   
	/**
	 * Zoom to specific level
	 * 
	 * @param zoomLevel
	 */
	public void zoomTo(double zoomLevel)
	{
		getZoomManager().setZoom(zoomLevel);
	}
	
	/**
	 * Get current zoom level
	 * 
	 * @return
	 */
	public double getZoom()
	{
	   return getZoomManager().getZoom();
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
			if (zoomLevels[i] == getZoom())
			{
				actions[i].setChecked(true);
			}
         if (zoomLevels[i] == 1.00)
         {
            if (view != null)
               view.addKeyBinding("M1+0", actions[i]);
         }
		}
		return actions;
	}

	/**
    * Show "back" button
    * 
	 * @param action
	 */
	public void showBackButton(final Runnable action)
	{
	   if (backButton != null)
	   {
	      backButton.setAction(action);
	   }
	   else
	   {
	      backButton = new OverlayButton(iconBack, action);
         controlLayer.add(backButton);
         backButton.setLocation(new org.eclipse.draw2d.geometry.Point(10, 10));
	   }
	}
	
	/**
	 * Hide "back" button
	 */
	public void hideBackButton()
	{
      if (backButton != null)
      {
         controlLayer.remove(backButton);
         backButton = null;
      }
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
			crosshairFigure.setSize(graph.getRootLayer().getSize());
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
	 * If map is fit to screen
	 * 
	 * @return true if map is fit to screen map
	 */
	boolean isFitToScreen()
	{
	   return backgroundFigure.getSize().equals(new Dimension(-1, -1));
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
				if (isFitToScreen())
				   gridFigure.setSize(backgroundLayer.getSize());
				else
               gridFigure.setSize(backgroundFigure.getSize());	
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
    * Show/hide map size border
    * 
    * @param show
    */
   public void showSize(boolean show)
   {
      showMapSize = show;
      if (show)
      {
         backgroundFigure.setBorder(new AbstractBorder() {
            @Override
            public void paint(IFigure figure, Graphics graphics, Insets insets)
            {
               tempRect.setBounds(getPaintRectangle(figure, insets));
               tempRect.shrink(1, 1);
               graphics.setLineWidth(1);
               graphics.setLineStyle(SWT.LINE_DASH);
               graphics.setLineDash(new int[] { 4, 8 });
               graphics.setForegroundColor(ThemeEngine.getForegroundColor("Map.Border"));
               graphics.drawRectangle(tempRect);
            }
   
            @Override
            public Insets getInsets(IFigure figure)
            {
               return new Insets(1, 1, 1, 1);
            }
         });
      }
      else
      {
         backgroundFigure.setBorder(null);
      }
   }

   /**
    * If map size border is visible 
    * 
    * @return true if map size border is visible
    */
   public boolean isSizeVisible()
   {
      return showMapSize;
   }
   
	
	/**
	 * Align objects to grid
	 */
	public void alignToGrid(boolean movedOnly)
	{
		for(Object o : graph.getNodes())
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
			graph.getZestRootLayer().addMouseListener(snapToGridListener);
		}
		else
		{
		   graph.getZestRootLayer().removeMouseListener(snapToGridListener);			
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
	
	/**
    * @return the draggingEnabled
    */
   public boolean isDraggingEnabled()
   {
      return draggingEnabled;
   }

   /**
    * @return if refresh is blocked by any action
    */
   public boolean isRefreshBlocked()
   {
      return blockRefresh > 0;
   }

   /**
    * @param draggingEnabled the draggingEnabled to set
    */
   public void setDraggingEnabled(boolean draggingEnabled)
   {
      this.draggingEnabled = draggingEnabled;
      graph.setDraggingEnabled(draggingEnabled);
   }

   /**
    * Resize tooltip's shell. Usually called by tooltip figure after re-layout.
    */
   void resizeToolTipShell()
   {
      eventDispatcher.resizeToolTipShell();
   }

   /**
    * @see org.eclipse.gef4.zest.core.viewers.GraphViewer#getFactory()
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
	 * Take snapshot of network map
	 * 
	 * @return
	 */
	public Image takeSnapshot()
	{
	   IFigure rootLayer = graph.getRootLayer();
	   Dimension d = rootLayer.getSize();
      Image image = new Image(graph.getDisplay(), d.width, d.height);
	   GC gc = new GC(image);
	   Graphics g = new SWTGraphics(gc);
	   rootLayer.paint(g);
	   g.dispose();
      gc.dispose();
      return image;
	}

	/**
	 * Additional figure used to display custom background for graph.
	 */
   private class BackgroundFigure extends Figure implements IDecorationLayer
	{
      /**
       * @see org.eclipse.draw2d.Figure#paintFigure(org.eclipse.draw2d.Graphics)
       */
		@Override
		protected void paintFigure(Graphics gc)
		{
         if (backgroundTiles != null)
         {
            drawTiles(gc, backgroundTiles);
         }
         else if (backgroundImage != null)
         {
            if (centeredBackground)
            {
               int backgroundLayerWidth = backgroundFigure.getSize().width();
               int backgroundLayerHeight = backgroundFigure.getSize().height();
               int x = (backgroundLayerWidth / 2) - (backgroundImage.getBounds().width / 2);
               int y = (backgroundLayerHeight / 2) - (backgroundImage.getBounds().height / 2);
               gc.drawImage(backgroundImage, new org.eclipse.draw2d.geometry.Point(x, y));               
            }
            else if (fitBackground)
            {
               int backgroundLayerWidth = backgroundFigure.getSize().width();
               int backgroundLayerHeight = backgroundFigure.getSize().height();
               gc.drawImage(backgroundImage, 0, 0, backgroundImage.getBounds().width, backgroundImage.getBounds().height, 0, 0,
                     backgroundLayerWidth, backgroundLayerHeight);                             
            }
            else
            {
               gc.drawImage(backgroundImage, getLocation());
            }
         }
		}

	   /**
	    * @param tileSet
	    */
	   private void drawTiles(Graphics gc, TileSet tileSet)
	   {
	      if ((backgroundLocation != null) && (backgroundImage != null))
	         backgroundImage.dispose();
	      
	      if ((tileSet == null) || (tileSet.tiles == null) || (tileSet.tiles.length == 0))
	      {
	         backgroundImage = null;
	         return;
	      }

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
       * Create crosshair figure
       */
		public Crosshair()
		{
			setOpaque(false);
		}
		
      /**
       * @see org.eclipse.draw2d.Figure#paintFigure(org.eclipse.draw2d.Graphics)
       */
		@Override
		protected void paintFigure(Graphics gc)
		{
			gc.setLineStyle(SWT.LINE_DOT);
			Dimension size = getSize();
			gc.drawLine(0, crosshairY, size.width, crosshairY);
			gc.drawLine(crosshairX, 0, crosshairX, size.height);
		}
	}

	/**
	 * Grid
	 */
	private class GridFigure extends Figure implements IDecorationLayer, IDecorationFigure
	{
		/**
		 * Create new grid figure
		 */
		public GridFigure()
		{
			setOpaque(false);
		}

      /**
       * @see org.eclipse.draw2d.Figure#paintFigure(org.eclipse.draw2d.Graphics)
       */
		@Override
		protected void paintFigure(Graphics gc)
		{
			gc.setLineStyle(SWT.LINE_DOT);
			Dimension size = getSize();
			for(int x = gridSize; x < size.width; x += gridSize)
				gc.drawLine(x, 0, x, size.height);
			for(int y = gridSize; y < size.height; y += gridSize)
				gc.drawLine(0, y, size.width, y);
		}
	}
	
	/**
	 * Overlay button
	 */
   private class OverlayButton extends Figure
	{
	   private Image icon;
	   private Runnable action;
	   
	   /**
	    * @param icon
	    * @param action
	    */
	   public OverlayButton(Image icon, Runnable action)
	   {
	      this.icon = icon;
	      this.action = action;
	      addMouseListener(new MouseListener() {
            @Override
            public void mousePressed(MouseEvent me)
            {
               OverlayButton.this.action.run();
            }

            @Override
            public void mouseReleased(MouseEvent me)
            {
            }

            @Override
            public void mouseDoubleClicked(MouseEvent me)
            {
            }
	      });
	      setSize(getPreferredSize());
	   }

      /**
       * @see org.eclipse.draw2d.Figure#getPreferredSize(int, int)
       */
      @Override
      public Dimension getPreferredSize(int wHint, int hHint)
      {
         return new Dimension(icon.getImageData().width, icon.getImageData().height).expand(6, 6);
      }

      /**
       * @see org.eclipse.draw2d.Figure#paintFigure(org.eclipse.draw2d.Graphics)
       */
      @Override
      protected void paintFigure(Graphics gc)
      {
         org.eclipse.draw2d.geometry.Point pos = getLocation();
         Dimension size = getSize();
         gc.setLineWidth(1);
         gc.setForegroundColor(colors.create(86, 20, 152));
         gc.setBackgroundColor(gc.getForegroundColor());
         gc.setAlpha(64);
         gc.fillRectangle(pos.x, pos.y, size.width - 1, size.height - 1);
         gc.setAlpha(255);
         gc.drawRectangle(pos.x, pos.y, size.width - 1, size.height - 1);
         gc.drawImage(icon, pos.x + 3, pos.y + 3);
      }

      /**
       * @param action
       */
      public void setAction(Runnable action)
	   {
	      this.action = action;
	   }
	}

   /**
    * Extended SWT event dispatcher - provides access to tooltip helper's shell
    */
   private static class ExtendedSWTEventDispatcher extends SWTEventDispatcher
   {
      private ExtendedToolTipHelper toolTipHelper;

      /**
       * @see org.eclipse.draw2d.SWTEventDispatcher#getToolTipHelper()
       */
      @Override
      protected ToolTipHelper getToolTipHelper()
      {
         if (toolTipHelper == null)
            toolTipHelper = new ExtendedToolTipHelper(control);
         return toolTipHelper;
      }

      /**
       * Resize tooltip shell
       */
      public void resizeToolTipShell()
      {
         if (toolTipHelper.isShowing())
            toolTipHelper.resizeShell();
      }
   }

   /**
    * Extended tooltip helper - provides access to tooltip helper's shell
    */
   private static class ExtendedToolTipHelper extends ToolTipHelper
   {
      public ExtendedToolTipHelper(Control c)
      {
         super(c);
      }

      /**
       * Resize tooltip shell
       */
      public void resizeShell()
      {
         Dimension shellSize = getLightweightSystem().getRootFigure().getPreferredSize().getExpanded(getShellTrimSize());
         getShell().setSize(shellSize.width, shellSize.height);
      }
   }

   /**
    * On link change
    */
   public void onLinkChange(NetworkMapLink element)
   {
      if (moveCallback != null)
      {
         moveCallback.onLinkChange(element);
      }
   }

   /**
    * Function called on decoration move
    * 
    * @param element moved decoration element
    */
   public void onDecorationMove(NetworkMapElement element)
   {
      if (moveCallback != null )
      {
         moveCallback.onMove(element);
      }
   }

   /**
    * Set map size
    * 
    * @param width map width
    * @param height map height
    */
   public void setMapSize(int width, int height)
   {
      graph.setPreferredSize(width, height);
      backgroundFigure.setSize(width, height);
      if (showMapSize)
      {
         backgroundFigure.setBorder(new AbstractBorder() {
            @Override
            public void paint(IFigure figure, Graphics graphics, Insets insets)
            {
               tempRect.setBounds(getPaintRectangle(figure, insets));
               tempRect.shrink(1, 1);
               graphics.setLineWidth(1);
               graphics.setLineStyle(SWT.LINE_DASH);
               graphics.setLineDash(new int[] { 4, 8 });
               graphics.setForegroundColor(ThemeEngine.getForegroundColor("Map.Border"));
               graphics.drawRectangle(tempRect);
            }
   
            @Override
            public Insets getInsets(IFigure figure)
            {
               return new Insets(1, 1, 1, 1);
            }
         });
      }
   }

   /**
    * Get current map size
    * 
    * @return map size
    */
   public Dimension getMapSize()
   {
      if (graph.getPreferredSize().height == -1)
      {
         Rectangle visibleArea = getGraphControl().getClientArea();
         return new Dimension(visibleArea.width, visibleArea.height);
      }
      else
         return graph.getPreferredSize();
   }

   /**
    * Get zoom index for font size calculation
    * 
    * @return zoom index in array
    */
   public int getCurrentZoomIndex()
   {
      for (int i = 0; i < zoomLevels.length; i++)
      {
         if (getZoom() <= zoomLevels[i])
         {
            return i;
         }
      }
      return 0;
   }
   
   /**
    * Get last right click location
    */
   public Point getRightClickLocation()
   {
      return rightClickLocation ;
   }

   /**
    * @param location
    * @return
    */
   public Point translateToRelative(Point location)
   {
      org.eclipse.draw2d.geometry.Point mousePoint = new org.eclipse.draw2d.geometry.Point(location.x, location.y);
      graph.getRootLayer().translateToRelative(mousePoint);
      return new Point(mousePoint.x, mousePoint.y);
   }
   
   /**
    * Get horizontal bar current position
    * 
    * @return current horizontal bar position
    */
   public int getHorizontalBarSelection()
   {
      return graph.getHorizontalBar().getSelection();
   }
   
   /**
    * Get vertical bar current position
    * 
    * @return current vertical bar position
    */
   public int getVerticalBarSelection()
   {
      return graph.getVerticalBar().getSelection();
   }  
}
