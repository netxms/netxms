/*******************************************************************************
 * Copyright 2005-2010, CHISEL Group, University of Victoria, Victoria, BC,
 * Canada. All rights reserved. This program and the accompanying materials are
 * made available under the terms of the Eclipse Public License v1.0 which
 * accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors: The Chisel Group, University of Victoria 
 *               Mateusz Matela
 ******************************************************************************/
package org.eclipse.zest.core.widgets;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;

import org.eclipse.draw2d.Animation;
import org.eclipse.draw2d.ColorConstants;
import org.eclipse.draw2d.ConnectionRouter;
import org.eclipse.draw2d.CoordinateListener;
import org.eclipse.draw2d.FigureCanvas;
import org.eclipse.draw2d.FreeformLayer;
import org.eclipse.draw2d.FreeformLayout;
import org.eclipse.draw2d.FreeformViewport;
import org.eclipse.draw2d.IFigure;
import org.eclipse.draw2d.LayoutAnimator;
import org.eclipse.draw2d.MouseMotionListener;
import org.eclipse.draw2d.PolylineConnection;
import org.eclipse.draw2d.SWTEventDispatcher;
import org.eclipse.draw2d.ScalableFigure;
import org.eclipse.draw2d.ScalableFreeformLayeredPane;
import org.eclipse.draw2d.ScrollPane;
import org.eclipse.draw2d.TreeSearch;
import org.eclipse.draw2d.geometry.Dimension;
import org.eclipse.draw2d.geometry.Point;
import org.eclipse.draw2d.geometry.Rectangle;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.events.ControlListener;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.ControlPaintHandler;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Item;
import org.eclipse.swt.widgets.Widget;
import org.eclipse.zest.core.widgets.gestures.RotateGestureListener;
import org.eclipse.zest.core.widgets.gestures.ZoomGestureListener;
import org.eclipse.zest.core.widgets.internal.ContainerFigure;
import org.eclipse.zest.core.widgets.internal.ZestRootLayer;
import org.eclipse.zest.core.widgets.zooming.ZoomManager;
import org.eclipse.zest.layouts.LayoutAlgorithm;
import org.eclipse.zest.layouts.dataStructures.DisplayIndependentRectangle;
import org.eclipse.zest.layouts.interfaces.ExpandCollapseManager;

/**
 * @since 1.0
 */
@SuppressWarnings({ "rawtypes", "unchecked" })
public class Graph extends FigureCanvas implements IContainer {
	private static final long serialVersionUID = 1L;
	
	// CLASS CONSTANTS
	public static final int ANIMATION_TIME = 500;
	public static final int FISHEYE_ANIMATION_TIME = 100;

	// @tag CGraph.Colors : These are the colour constants for the graph, they
	// are disposed on clean-up
	public Color LIGHT_BLUE = new Color(null, 216, 228, 248);
	public Color LIGHT_BLUE_CYAN = new Color(null, 213, 243, 255);
	public Color GREY_BLUE = new Color(null, 139, 150, 171);
	public Color DARK_BLUE = new Color(null, 1, 70, 122);
	public Color LIGHT_YELLOW = new Color(null, 255, 255, 206);

	public Color HIGHLIGHT_COLOR = ColorConstants.yellow;
	public Color HIGHLIGHT_ADJACENT_COLOR = ColorConstants.orange;
	public Color DEFAULT_NODE_COLOR = LIGHT_BLUE;

	/**
	 * These are all the children of this graph. These lists contains all nodes
	 * and connections that have added themselves to this graph.
	 */
	private List nodes;
	protected List connections;
	HashSet subgraphFigures;
	private List selectedItems = null;
	private ArrayList fisheyeListeners = new ArrayList();
	private List selectionListeners = null;

	/** This maps all visible nodes to their model element. */
	private HashMap figure2ItemMap = null;

	private int connectionStyle;
	private int nodeStyle;
	private ScalableFreeformLayeredPane fishEyeLayer = null;
	private InternalLayoutContext layoutContext = null;
	private volatile boolean shouldSheduleLayout;
	private volatile Runnable scheduledLayoutRunnable = null;
	private volatile boolean scheduledLayoutClean = false;
	private Dimension preferredSize = null;
	int style = 0;

	private ScalableFreeformLayeredPane rootlayer;
	private ZestRootLayer zestRootLayer;

	private ConnectionRouter defaultConnectionRouter;
	private ZoomManager zoomManager = null;

	/**
	 * Constructor for a Graph. This widget represents the root of the graph,
	 * and can contain graph items such as graph nodes and graph connections.
	 * 
	 * @param parent
	 * @param style
	 */
	public Graph(Composite parent, int style) {
		super(parent, style | SWT.DOUBLE_BUFFERED);
		this.style = style;
		this.setBackground(ColorConstants.white);

		this.setViewport(new FreeformViewport());

		this.getVerticalBar().addSelectionListener(new SelectionAdapter() {
			/**
			 * 
			 */
			private static final long serialVersionUID = 1L;

			public void widgetSelected(SelectionEvent e) {
				Graph.this.redraw();
			}

		});
		this.getHorizontalBar().addSelectionListener(new SelectionAdapter() {
			/**
			 * 
			 */
			private static final long serialVersionUID = 1L;

			public void widgetSelected(SelectionEvent e) {
				Graph.this.redraw();
			}
		});

		// @tag CGraph.workaround : this allows me to handle mouse events
		// outside of the canvas
		this.getLightweightSystem().setEventDispatcher(
				new SWTEventDispatcher() {
					public void dispatchMouseMoved(
							org.eclipse.swt.events.MouseEvent me) {
						super.dispatchMouseMoved(me);

						// If the current event is null, return
						if (getCurrentEvent() == null) {
							return;
						}

						if (getMouseTarget() == null) {
							setMouseTarget(getRoot());
						}
						if ((me.stateMask & SWT.BUTTON_MASK) != 0) {
							// Sometimes getCurrentEvent() returns null
							getMouseTarget().handleMouseDragged(
									getCurrentEvent());
						} else {
							getMouseTarget()
									.handleMouseMoved(getCurrentEvent());
						}
					}
				});

		this.setContents(createLayers());
		DragSupport dragSupport = new DragSupport();
		this.getLightweightSystem().getRootFigure()
				.addMouseListener(dragSupport);
		this.getLightweightSystem().getRootFigure()
				.addMouseMotionListener(dragSupport);

		this.nodes = new ArrayList();
		this.preferredSize = new Dimension(-1, -1);
		this.connectionStyle = ZestStyles.NONE;
		this.nodeStyle = ZestStyles.NONE;
		this.connections = new ArrayList();
		this.subgraphFigures = new HashSet();
		this.selectedItems = new ArrayList();
		this.selectionListeners = new ArrayList();
		this.figure2ItemMap = new HashMap();

		ControlPaintHandler helper = new ControlPaintHandler(this);
		helper.addPaintListener(new PaintListener() {
			/**
			 * 
			 */
			private static final long serialVersionUID = 1L;

			public void paintControl(PaintEvent e) {
				if (shouldSheduleLayout) {
					applyLayoutInternal(true);
					shouldSheduleLayout = false;
				}
			}
		});

		this.addControlListener(new ControlListener() {

			/**
			 * 
			 */
			private static final long serialVersionUID = 1L;

			public void controlResized(ControlEvent e) {
				if (preferredSize.width == -1 || preferredSize.height == -1) {
					getLayoutContext().fireBoundsChangedEvent();
				}
			}

			public void controlMoved(ControlEvent e) {
				// do nothing
			}
		});
		if ((style & (ZestStyles.GESTURES_DISABLED)) == 0) {
			// Only add default gestures if not disabled by style bit
			this.addGestureListener(new ZoomGestureListener());
			this.addGestureListener(new RotateGestureListener());
		}
		this.addDisposeListener(new DisposeListener() {
			/**
			 * 
			 */
			private static final long serialVersionUID = 1L;

			public void widgetDisposed(DisposeEvent e) {
				release();
			}
		});
	}

	/**
	 * This adds a listener to the set of listeners that will be called when a
	 * selection event occurs.
	 * 
	 * @param selectionListener
	 */
	public void addSelectionListener(SelectionListener selectionListener) {
		if (!selectionListeners.contains(selectionListener)) {
			selectionListeners.add(selectionListener);
		}
	}

	public void removeSelectionListener(SelectionListener selectionListener) {
		if (selectionListeners.contains(selectionListener)) {
			selectionListeners.remove(selectionListener);
		}
	}

	/**
	 * Gets a list of the GraphNode children objects under the root node in this
	 * graph. If the root node is null then all the top level nodes are
	 * returned.
	 * 
	 * @return List of GraphNode objects
	 */
	public List getNodes() {
		return nodes;
	}

	/**
	 * Gets the root layer for this graph
	 * 
	 * @return
	 */
	public ScalableFigure getRootLayer() {
		return rootlayer;
	}

	/**
	 * Sets the default connection style.
	 * 
	 * @param connection
	 *            style the connection style to set
	 * @see org.eclipse.mylar.zest.core.widgets.ZestStyles
	 */
	public void setConnectionStyle(int connectionStyle) {
		this.connectionStyle = connectionStyle;
	}

	/**
	 * Gets the default connection style.
	 * 
	 * @return the connection style
	 * @see org.eclipse.mylar.zest.core.widgets.ZestStyles
	 */
	public int getConnectionStyle() {
		return connectionStyle;
	}

	/**
	 * Sets the default node style.
	 * 
	 * @param nodeStyle
	 *            the node style to set
	 * @see org.eclipse.mylar.zest.core.widgets.ZestStyles
	 */
	public void setNodeStyle(int nodeStyle) {
		this.nodeStyle = nodeStyle;
	}

	/**
	 * Gets the default node style.
	 * 
	 * @return the node style
	 * @see org.eclipse.mylar.zest.core.widgets.ZestStyles
	 */
	public int getNodeStyle() {
		return nodeStyle;
	}

	/**
	 * Gets the list of GraphModelConnection objects.
	 * 
	 * @return list of GraphModelConnection objects
	 */
	public List getConnections() {
		return this.connections;
	}

	/**
	 * Changes the selection to the list of items
	 * 
	 * @param l
	 */
	public void setSelection(GraphItem[] items) {
		clearSelection();
		if (items != null) {
			for (int i = 0; i < items.length; i++) {
				if (items[i] != null) {
					select(items[i]);
				}
			}
		}
	}

	public void selectAll() {
		setSelection((GraphItem[]) nodes.toArray(new GraphItem[] {}));
	}

	/**
	 * Gets the list of currently selected GraphNodes
	 * 
	 * @return Currently selected graph node
	 */
	public List getSelection() {
		return selectedItems;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.swt.widgets.Widget#toString()
	 */
	public String toString() {
		return "GraphModel {" + nodes.size() + " nodes, " + connections.size()
				+ " connections}";
	}

	/**
	 * Dispose of the nodes and edges when the graph is disposed.
	 */
	public void dispose() {
		release();
		super.dispose();
	}

	/**
	 * Runs the layout on this graph. If the view is not visible layout will be
	 * deferred until after the view is available.
	 */
	public void applyLayout() {
		scheduleLayoutOnReveal(true);
	}

	/**
	 * Apply this graphs's layout cleanly and display all changes.
	 * 
	 * @since 2.0
	 */
	public void applyLayoutNow() {
		getLayoutContext().applyLayout(true);
		layoutContext.flushChanges(false);
	}

	/**
	 * Enables or disables dynamic layout (that is layout algorithm performing
	 * layout in background or when certain events occur). Dynamic layout should
	 * be disabled before doing a long series of changes in the graph to make
	 * sure that layout algorithm won't interfere with these changes.
	 * 
	 * Enabling dynamic layout causes the layout algorithm to be applied even if
	 * it's not actually a dynamic algorithm.
	 * 
	 * @param enabled
	 * 
	 * @since 2.0
	 */
	public void setDynamicLayout(boolean enabled) {
		if (getLayoutContext().isBackgroundLayoutEnabled() != enabled) {
			layoutContext.setBackgroundLayoutEnabled(enabled);
			if (enabled) {
				scheduleLayoutOnReveal(false);
			}
		}
	}

	/**
	 * 
	 * @return true if dynamic layout is enabled (see
	 *         {@link #setDynamicLayout(boolean)})
	 * @since 2.0
	 */
	public boolean isDynamicLayoutEnabled() {
		return getLayoutContext().isBackgroundLayoutEnabled();
	}

	private void release() {
		while (nodes.size() > 0) {
			GraphNode node = (GraphNode) nodes.get(0);
			if (node != null) {
				node.dispose();
			}
		}
		while (connections.size() > 0) {
			GraphConnection connection = (GraphConnection) connections.get(0);
			if (connection != null) {
				connection.dispose();
			}
		}
		LIGHT_BLUE.dispose();
		LIGHT_BLUE_CYAN.dispose();
		GREY_BLUE.dispose();
		DARK_BLUE.dispose();
		LIGHT_YELLOW.dispose();
	}

	private void applyLayoutInternal(boolean clean) {
		if (getLayoutContext().getLayoutAlgorithm() == null) {
			return;
		}
		scheduledLayoutClean = scheduledLayoutClean || clean;
		synchronized (this) {
			if (scheduledLayoutRunnable == null) {
				Display.getDefault().asyncExec(
						scheduledLayoutRunnable = new Runnable() {
							public void run() {
								Animation.markBegin();
								getLayoutContext().applyLayout(
										scheduledLayoutClean);
								layoutContext.flushChanges(false);
								Animation.run(ANIMATION_TIME);
								getLightweightSystem().getUpdateManager()
										.performUpdate();
								synchronized (Graph.this) {
									scheduledLayoutRunnable = null;
									scheduledLayoutClean = false;
								}
							}
						});
			}
		}
	}

	/**
	 * Sets the preferred size of the layout area. Size of ( -1, -1) uses the
	 * current canvas size.
	 * 
	 * @param width
	 * @param height
	 */
	public void setPreferredSize(int width, int height) {
		this.preferredSize = new Dimension(width, height);
		getLayoutContext().fireBoundsChangedEvent();
	}

	/**
	 * @return the preferred size of the layout area.
	 * @since 2.0
	 */
	public Dimension getPreferredSize() {
		if (preferredSize.width < 0 || preferredSize.height < 0) {
			org.eclipse.swt.graphics.Point size = getSize();
			double scale = getZoomManager().getZoom();
			return new Dimension((int) (size.x / scale + 0.5), (int) (size.y
					/ scale + 0.5));
		}
		return preferredSize;
	}

	/**
	 * @noreference This method is not intended to be referenced by clients.
	 */
	public InternalLayoutContext getLayoutContext() {
		if (layoutContext == null) {
			layoutContext = new InternalLayoutContext(this);
		}
		return layoutContext;
	}

	/**
	 * @param algorithm
	 * @since 2.0
	 */
	public void setLayoutAlgorithm(LayoutAlgorithm algorithm,
			boolean applyLayout) {
		getLayoutContext().setLayoutAlgorithm(algorithm);
		if (applyLayout) {
			applyLayout();
		}
	}

	/**
	 * @since 2.0
	 */
	public LayoutAlgorithm getLayoutAlgorithm() {
		return getLayoutContext().getLayoutAlgorithm();
	}

	/**
	 * @since 2.0
	 */
	public void setSubgraphFactory(SubgraphFactory factory) {
		getLayoutContext().setSubgraphFactory(factory);
	}

	/**
	 * @since 2.0
	 */
	public SubgraphFactory getSubgraphFactory() {
		return getLayoutContext().getSubgraphFactory();
	}

	/**
	 * @since 2.0
	 */
	public void setExpandCollapseManager(
			ExpandCollapseManager expandCollapseManager) {
		getLayoutContext().setExpandCollapseManager(expandCollapseManager);
	}

	/**
	 * @since 2.0
	 */
	public ExpandCollapseManager getExpandCollapseManager() {
		return getLayoutContext().getExpandCollapseManager();
	}

	/**
	 * Adds a filter used for hiding elements from layout algorithm.
	 * 
	 * NOTE: If a node or subgraph if filtered out, all connections adjacent to
	 * it should also be filtered out. Otherwise layout algorithm may behave in
	 * an unexpected way.
	 * 
	 * @param filter
	 *            filter to add
	 * @since 2.0
	 */
	public void addLayoutFilter(LayoutFilter filter) {
		getLayoutContext().addFilter(filter);
	}

	/**
	 * Removes given layout filter. If it had not been added to this graph, this
	 * method does nothing.
	 * 
	 * @param filter
	 *            filter to remove
	 * @since 2.0
	 */
	public void removeLayoutFilter(LayoutFilter filter) {
		getLayoutContext().removeFilter(filter);
	}

	/**
	 * Finds a figure at the location X, Y in the graph
	 * 
	 * This point should be translated to relative before calling findFigureAt
	 */
	public IFigure getFigureAt(int x, int y) {
		IFigure figureUnderMouse = this.getContents().findFigureAt(x, y,
				new TreeSearch() {

					public boolean accept(IFigure figure) {
						return true;
					}

					public boolean prune(IFigure figure) {
						IFigure parent = figure.getParent();
						// @tag TODO Zest : change these to from getParent to
						// their
						// actual layer names

						if (parent == fishEyeLayer) {
							// If it node is on the fish eye layer, don't worry
							// about
							// it.
							return true;
						}
						if (parent instanceof ContainerFigure
								&& figure instanceof PolylineConnection) {
							return false;
						}
						if (parent == zestRootLayer
								|| parent == zestRootLayer.getParent()
								|| parent == zestRootLayer.getParent()
										.getParent()) {
							return false;
						}
						GraphItem item = (GraphItem) figure2ItemMap.get(figure);
						if (item != null
								&& item.getItemType() == GraphItem.CONTAINER) {
							return false;
						} else if (figure instanceof FreeformLayer
								|| parent instanceof FreeformLayer
								|| figure instanceof ScrollPane
								|| parent instanceof ScrollPane
								|| parent instanceof ScalableFreeformLayeredPane
								|| figure instanceof ScalableFreeformLayeredPane
								|| figure instanceof FreeformViewport
								|| parent instanceof FreeformViewport) {
							return false;
						}
						return true;
					}

				});
		return figureUnderMouse;

	}

	private class DragSupport implements MouseMotionListener,
			org.eclipse.draw2d.MouseListener {

		Point dragStartLocation = null;
		IFigure draggedSubgraphFigure = null;
		/** locations of dragged items relative to cursor position */
		ArrayList relativeLocations = new ArrayList();
		GraphItem fisheyedItem = null;
		boolean isDragging = false;

		public void mouseDragged(org.eclipse.draw2d.MouseEvent me) {
			if (!isDragging) {
				return;
			}
			if (selectedItems.isEmpty()) {
				IFigure figureUnderMouse = getFigureAt(dragStartLocation.x,
						dragStartLocation.y);
				if (subgraphFigures.contains(figureUnderMouse)) {
					draggedSubgraphFigure = figureUnderMouse;
				}
			}

			Point mousePoint = new Point(me.x, me.y);
			if (!selectedItems.isEmpty() || draggedSubgraphFigure != null) {

				if (relativeLocations.isEmpty()) {
					for (Iterator iterator = selectedItems.iterator(); iterator
							.hasNext();) {
						GraphItem item = (GraphItem) iterator.next();
						if ((item.getItemType() == GraphItem.NODE)
								|| (item.getItemType() == GraphItem.CONTAINER)) {
							relativeLocations.add(getRelativeLocation(item
									.getFigure()));
						}
					}
					if (draggedSubgraphFigure != null) {
						relativeLocations
								.add(getRelativeLocation(draggedSubgraphFigure));
					}
				}

				Iterator locationsIterator = relativeLocations.iterator();
				for (Iterator selectionIterator = selectedItems.iterator(); selectionIterator
						.hasNext();) {
					GraphItem item = (GraphItem) selectionIterator.next();
					if ((item.getItemType() == GraphItem.NODE)
							|| (item.getItemType() == GraphItem.CONTAINER)) {
						Point pointCopy = mousePoint.getCopy();
						Point relativeLocation = (Point) locationsIterator
								.next();

						item.getFigure().getParent()
								.translateToRelative(pointCopy);
						item.getFigure().getParent()
								.translateFromParent(pointCopy);

						((GraphNode) item)
								.setLocation(relativeLocation.x + pointCopy.x,
										relativeLocation.y + pointCopy.y);
					} else {
						// There is no movement for connection
					}
				}
				if (draggedSubgraphFigure != null) {
					Point pointCopy = mousePoint.getCopy();
					draggedSubgraphFigure.getParent().translateToRelative(
							pointCopy);
					draggedSubgraphFigure.getParent().translateFromParent(
							pointCopy);
					Point relativeLocation = (Point) locationsIterator.next();
					pointCopy.x += relativeLocation.x;
					pointCopy.y += relativeLocation.y;

					draggedSubgraphFigure.setLocation(pointCopy);
				}
			}
		}

		private Point getRelativeLocation(IFigure figure) {
			Point location = figure.getBounds().getTopLeft();
			Point mousePointCopy = dragStartLocation.getCopy();
			figure.getParent().translateToRelative(mousePointCopy);
			figure.getParent().translateFromParent(mousePointCopy);
			location.x -= mousePointCopy.x;
			location.y -= mousePointCopy.y;
			return location;
		}

		public void mouseEntered(org.eclipse.draw2d.MouseEvent me) {

		}

		public void mouseExited(org.eclipse.draw2d.MouseEvent me) {

		}

		public void mouseHover(org.eclipse.draw2d.MouseEvent me) {

		}

		/**
		 * This tracks whenever a mouse moves. The only thing we care about is
		 * fisheye(ing) nodes. This means whenever the mouse moves we check if
		 * we need to fisheye on a node or not.
		 */
		public void mouseMoved(org.eclipse.draw2d.MouseEvent me) {
			Point mousePoint = new Point(me.x, me.y);
			getRootLayer().translateToRelative(mousePoint);
			IFigure figureUnderMouse = getFigureAt(mousePoint.x, mousePoint.y);

			if (figureUnderMouse != null) {
				// There is a figure under this mouse
				GraphItem itemUnderMouse = (GraphItem) figure2ItemMap
						.get(figureUnderMouse);
				if (itemUnderMouse == fisheyedItem) {
					return;
				}
				if (fisheyedItem != null) {
					((GraphNode) fisheyedItem).fishEye(false, true);
					fisheyedItem = null;
				}
				if (itemUnderMouse != null
						&& itemUnderMouse.getItemType() == GraphItem.NODE) {
					fisheyedItem = itemUnderMouse;
					IFigure fisheyedFigure = ((GraphNode) itemUnderMouse)
							.fishEye(true, true);
					if (fisheyedFigure == null) {
						// If there is no fisheye figure (this means that the
						// node does not support a fish eye)
						// then remove the fisheyed item
						fisheyedItem = null;
					}
				}
			} else {
				if (fisheyedItem != null) {
					((GraphNode) fisheyedItem).fishEye(false, true);
					fisheyedItem = null;
				}
			}
		}

		public void mouseDoubleClicked(org.eclipse.draw2d.MouseEvent me) {

		}

		public void mousePressed(org.eclipse.draw2d.MouseEvent me) {
			isDragging = true;
			Point mousePoint = new Point(me.x, me.y);
			dragStartLocation = mousePoint.getCopy();

			getRootLayer().translateToRelative(mousePoint);

			if ((me.getState() & SWT.MOD3) != 0) {
				if ((me.getState() & SWT.MOD2) == 0) {
					double scale = getRootLayer().getScale();
					scale *= 1.05;
					getRootLayer().setScale(scale);
					Point newMousePoint = mousePoint.getCopy().scale(1.05);
					Point delta = new Point(newMousePoint.x - mousePoint.x,
							newMousePoint.y - mousePoint.y);
					Point newViewLocation = getViewport().getViewLocation()
							.getCopy().translate(delta);
					getViewport().setViewLocation(newViewLocation);

					clearSelection();
					return;
				} else {
					double scale = getRootLayer().getScale();
					scale /= 1.05;
					getRootLayer().setScale(scale);

					Point newMousePoint = mousePoint.getCopy().scale(1 / 1.05);
					Point delta = new Point(newMousePoint.x - mousePoint.x,
							newMousePoint.y - mousePoint.y);
					Point newViewLocation = getViewport().getViewLocation()
							.getCopy().translate(delta);
					getViewport().setViewLocation(newViewLocation);
					clearSelection();
					return;
				}
			} else {
				boolean hasSelection = selectedItems.size() > 0;
				IFigure figureUnderMouse = getFigureAt(mousePoint.x,
						mousePoint.y);
				getRootLayer().translateFromParent(mousePoint);

				if (figureUnderMouse != null) {
					figureUnderMouse.getParent()
							.translateFromParent(mousePoint);
				}
				// If the figure under the mouse is the canvas, and CTRL/CMD is
				// not being held down, then select nothing
				if (figureUnderMouse == null || figureUnderMouse == Graph.this.zestRootLayer || figureUnderMouse == Graph.this.rootlayer) {
					if ((me.getState() & SWT.MOD1) == 0) {
						clearSelection();
						if (hasSelection) {
							fireWidgetSelectedEvent(null);
							hasSelection = false;
						}
					}
					return;
				}

				GraphItem itemUnderMouse = (GraphItem) figure2ItemMap
						.get(figureUnderMouse);
				if (itemUnderMouse == null) {
					if ((me.getState() & SWT.MOD1) != 0) {
						clearSelection();
						if (hasSelection) {
							fireWidgetSelectedEvent(null);
							hasSelection = false;
						}
					}
					return;
				}
				if (selectedItems.contains(itemUnderMouse)) {
					// We have already selected this node, and CTRL/CMD is being
					// held down, remove this selection
					// @tag Zest.selection : This deselects when you have
					// CTRL/CMD pressed
					if ((me.getState() & SWT.MOD1) != 0) {
						selectedItems.remove(itemUnderMouse);
						(itemUnderMouse).unhighlight();
						fireWidgetSelectedEvent(itemUnderMouse);
					}
					return;
				}

				if ((me.getState() & SWT.MOD1) == 0) {
					clearSelection();
				}

				if (itemUnderMouse.getItemType() == GraphItem.NODE) {
					// @tag Zest.selection : This is where the nodes are
					// selected
					selectedItems.add(itemUnderMouse);
					((GraphNode) itemUnderMouse).highlight();
					fireWidgetSelectedEvent(itemUnderMouse);
				} else if (itemUnderMouse.getItemType() == GraphItem.CONNECTION) {
					selectedItems.add(itemUnderMouse);
					((GraphConnection) itemUnderMouse).highlight();
					fireWidgetSelectedEvent(itemUnderMouse);

				} else if (itemUnderMouse.getItemType() == GraphItem.CONTAINER) {
					selectedItems.add(itemUnderMouse);
					((GraphContainer) itemUnderMouse).highlight();
					fireWidgetSelectedEvent(itemUnderMouse);
				}
			}

		}

		public void mouseReleased(org.eclipse.draw2d.MouseEvent me) {
			isDragging = false;
			relativeLocations.clear();
			draggedSubgraphFigure = null;
		}

	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.swt.widgets.Widget#notifyListeners(int,
	 * org.eclipse.swt.widgets.Event)
	 */
	public void notifyListeners(int eventType, Event event) {
		super.notifyListeners(eventType, event);
		if (eventType == SWT.Selection && event != null) {
			notifySelectionListeners(new SelectionEvent(event));
		}
	}

	private void clearSelection() {
		Iterator iterator = new ArrayList(selectedItems).iterator();
		while (iterator.hasNext()) {
			deselect((GraphItem) iterator.next());
		}
	}

	private void fireWidgetSelectedEvent(Item item) {
		Event swtEvent = new Event();
		swtEvent.item = item;
		swtEvent.widget = this;
		notifySelectionListeners(new SelectionEvent(swtEvent));
	}

	private void notifySelectionListeners(SelectionEvent event) {
		Iterator iterator = selectionListeners.iterator();
		while (iterator.hasNext()) {
			((SelectionListener) iterator.next()).widgetSelected(event);
		}
	}

	private void deselect(GraphItem item) {
		selectedItems.remove(item);
		item.unhighlight();
		setNodeSelected(item, false);
	}

	private void select(GraphItem item) {
		selectedItems.add(item);
		item.highlight();
		setNodeSelected(item, true);
	}

	private void setNodeSelected(GraphItem item, boolean selected) {
		if (item instanceof GraphNode) {
			((GraphNode) item).setSelected(selected);
		}
	}

	/**
	 * Converts the list of GraphModelConnection objects into an array and
	 * returns it.
	 * 
	 * @return GraphModelConnection[]
	 */
	GraphConnection[] getConnectionsArray() {
		GraphConnection[] connsArray = new GraphConnection[connections.size()];
		connsArray = (GraphConnection[]) connections.toArray(connsArray);
		return connsArray;
	}

	/**
	 * Clear the graph of all its content.
	 * 
	 * @since 2.0
	 */
	public void clear() {
		for (Iterator i = new ArrayList(connections).iterator(); i.hasNext();) {
			removeConnection((GraphConnection) i.next());
		}
		for (Iterator i = new HashSet(subgraphFigures).iterator(); i.hasNext();) {
			removeSubgraphFigure((IFigure) i.next());
		}
		for (Iterator i = new ArrayList(nodes).iterator(); i.hasNext();) {
			removeNode((GraphNode) i.next());
		}
	}

	void removeConnection(GraphConnection connection) {
		IFigure figure = connection.getConnectionFigure();
		PolylineConnection sourceContainerConnectionFigure = connection
				.getSourceContainerConnectionFigure();
		PolylineConnection targetContainerConnectionFigure = connection
				.getTargetContainerConnectionFigure();
		connection.removeFigure();
		this.getConnections().remove(connection);
		this.selectedItems.remove(connection);
		figure2ItemMap.remove(figure);
		if (sourceContainerConnectionFigure != null) {
			figure2ItemMap.remove(sourceContainerConnectionFigure);
		}
		if (targetContainerConnectionFigure != null) {
			figure2ItemMap.remove(targetContainerConnectionFigure);
		}
		getLayoutContext().fireConnectionRemovedEvent(connection.getLayout());
	}

	void removeNode(GraphNode node) {
		IFigure figure = node.getNodeFigure();
		if (figure.getParent() != null) {
			figure.getParent().remove(figure);
		}
		this.getNodes().remove(node);
		this.selectedItems.remove(node);
		figure2ItemMap.remove(figure);
		node.getLayout().dispose();
	}

	void addConnection(GraphConnection connection, boolean addToEdgeLayer) {
		this.getConnections().add(connection);
		if (addToEdgeLayer) {
			zestRootLayer.addConnection(connection.getFigure());
		}
		getLayoutContext().fireConnectionAddedEvent(connection.getLayout());
	}

	/**
	 * @noreference This method is not intended to be referenced by clients.
	 */
	public void addNode(GraphNode node) {
		this.getNodes().add(node);
		zestRootLayer.addNode(node.getFigure());
		getLayoutContext().fireNodeAddedEvent(node.getLayout());
	}

	/**
	 * @noreference This method is not intended to be referenced by clients.
	 */
	public void addSubgraphFigure(IFigure figure) {
		zestRootLayer.addSubgraph(figure);
		subgraphFigures.add(figure);
	}

	void removeSubgraphFigure(IFigure figure) {
		subgraphFigures.remove(figure);
		figure.getParent().remove(figure);
	}

	void registerItem(GraphItem item) {
		if (item.getItemType() == GraphItem.NODE) {
			IFigure figure = item.getFigure();
			figure2ItemMap.put(figure, item);
		} else if (item.getItemType() == GraphItem.CONNECTION) {
			IFigure figure = item.getFigure();
			figure2ItemMap.put(figure, item);
			if (((GraphConnection) item).getSourceContainerConnectionFigure() != null) {
				figure2ItemMap.put(((GraphConnection) item)
						.getSourceContainerConnectionFigure(), item);
			}
			if (((GraphConnection) item).getTargetContainerConnectionFigure() != null) {
				figure2ItemMap.put(((GraphConnection) item)
						.getTargetContainerConnectionFigure(), item);
			}
		} else if (item.getItemType() == GraphItem.CONTAINER) {
			IFigure figure = item.getFigure();
			figure2ItemMap.put(figure, item);
		} else {
			throw new RuntimeException("Unknown item type: "
					+ item.getItemType());
		}
	}

	/**
	 * Schedules a layout to be performed after the view is revealed (or
	 * immediately, if the view is already revealed).
	 * 
	 * @param clean
	 */
	private void scheduleLayoutOnReveal(final boolean clean) {

		final boolean[] isVisibleSync = new boolean[1];
		Display.getDefault().syncExec(new Runnable() {
			public void run() {
				isVisibleSync[0] = isVisible();
			}
		});

		if (isVisibleSync[0]) {
			applyLayoutInternal(clean);
		} else {
			shouldSheduleLayout = true;
		}
	}

	private ScalableFigure createLayers() {
		rootlayer = new ScalableFreeformLayeredPane();
		rootlayer.setLayoutManager(new FreeformLayout());
		zestRootLayer = new ZestRootLayer();

		zestRootLayer.setLayoutManager(new FreeformLayout());

		fishEyeLayer = new ScalableFreeformLayeredPane();
		fishEyeLayer.setLayoutManager(new FreeformLayout());

		rootlayer.add(zestRootLayer);
		rootlayer.add(fishEyeLayer);

		zestRootLayer.addLayoutListener(LayoutAnimator.getDefault());
		fishEyeLayer.addLayoutListener(LayoutAnimator.getDefault());

		rootlayer.addCoordinateListener(new CoordinateListener() {
			public void coordinateSystemChanged(IFigure source) {
				if (preferredSize.width == -1 && preferredSize.height == -1) {
					getLayoutContext().fireBoundsChangedEvent();
				}
			}
		});

		return rootlayer;
	}

	/**
	 * This removes the fisheye from the graph. It uses an animation to make the
	 * fisheye shrink, and then it finally clears the fisheye layer. This
	 * assumes that there is ever only 1 node on the fisheye layer at any time.
	 * 
	 * @param fishEyeFigure
	 *            The fisheye figure
	 * @param regularFigure
	 *            The regular figure (i.e. the non fisheye version)
	 */
	void removeFishEye(final IFigure fishEyeFigure,
			final IFigure regularFigure, boolean animate) {

		if (!fishEyeLayer.getChildren().contains(fishEyeFigure)) {
			return;
		}
		if (animate) {
			Animation.markBegin();
		}

		Rectangle bounds = regularFigure.getBounds().getCopy();
		regularFigure.translateToAbsolute(bounds);

		double scale = rootlayer.getScale();
		fishEyeLayer.setScale(1 / scale);
		fishEyeLayer.translateToRelative(bounds);
		fishEyeLayer.translateFromParent(bounds);

		fishEyeLayer.setConstraint(fishEyeFigure, bounds);

		for (Iterator iterator = fisheyeListeners.iterator(); iterator
				.hasNext();) {
			FisheyeListener listener = (FisheyeListener) iterator.next();
			listener.fisheyeRemoved(this, regularFigure, fishEyeFigure);
		}

		if (animate) {
			Animation.run(FISHEYE_ANIMATION_TIME * 2);
		}
		this.getRootLayer().getUpdateManager().performUpdate();
		fishEyeLayer.removeAll();

	}

	/**
	 * Replaces the old fisheye figure with a new one.
	 * 
	 * @param oldFigure
	 * @param newFigure
	 */
	boolean replaceFishFigure(IFigure oldFigure, IFigure newFigure) {
		if (this.fishEyeLayer.getChildren().contains(oldFigure)) {
			Rectangle bounds = oldFigure.getBounds();
			newFigure.setBounds(bounds);
			this.fishEyeLayer.remove(oldFigure);
			this.fishEyeLayer.add(newFigure);

			for (Iterator iterator = fisheyeListeners.iterator(); iterator
					.hasNext();) {
				FisheyeListener listener = (FisheyeListener) iterator.next();
				listener.fisheyeReplaced(this, oldFigure, newFigure);
			}

			return true;
		}
		return false;
	}

	/**
	 * Add a fisheye version of the node. This works by animating the change
	 * from the original node to the fisheyed one, and then placing the fisheye
	 * node on the fisheye layer.
	 * 
	 * @param startFigure
	 *            The original node
	 * @param endFigure
	 *            The fisheye figure
	 * @param newBounds
	 *            The final size of the fisheyed figure
	 */
	void fishEye(IFigure startFigure, IFigure endFigure, Rectangle newBounds,
			boolean animate) {

		fishEyeLayer.removeAll();

		if (animate) {
			Animation.markBegin();
		}

		double scale = rootlayer.getScale();
		fishEyeLayer.setScale(1 / scale);

		fishEyeLayer.translateToRelative(newBounds);
		fishEyeLayer.translateFromParent(newBounds);

		Rectangle bounds = startFigure.getBounds().getCopy();
		startFigure.translateToAbsolute(bounds);
		// startFigure.translateToRelative(bounds);
		fishEyeLayer.translateToRelative(bounds);
		fishEyeLayer.translateFromParent(bounds);

		endFigure.setLocation(bounds.getLocation());
		endFigure.setSize(bounds.getSize());
		fishEyeLayer.add(endFigure);
		fishEyeLayer.setConstraint(endFigure, newBounds);

		for (Iterator iterator = fisheyeListeners.iterator(); iterator
				.hasNext();) {
			FisheyeListener listener = (FisheyeListener) iterator.next();
			listener.fisheyeAdded(this, startFigure, endFigure);
		}

		if (animate) {
			Animation.run(FISHEYE_ANIMATION_TIME);
		}
		this.getRootLayer().getUpdateManager().performUpdate();
	}

	/**
	 * Adds a listener that will be notified when fisheyed figures change in
	 * this graph.
	 * 
	 * @param listener
	 *            listener to add
	 * @since 2.0
	 */
	public void addFisheyeListener(FisheyeListener listener) {
		fisheyeListeners.add(listener);
	}

	/**
	 * @since 2.0
	 */
	public void removeFisheyeListener(FisheyeListener listener) {
		fisheyeListeners.remove(listener);
	}

	public int getItemType() {
		return GraphItem.GRAPH;
	}

	GraphItem getGraphItem(IFigure figure) {
		return (GraphItem) figure2ItemMap.get(figure);
	}

	/**
	 * @since 2.0
	 */
	public void setExpanded(GraphNode node, boolean expanded) {
		layoutContext.setExpanded(node.getLayout(), expanded);
		rootlayer.invalidate();
	}

	/**
	 * @since 2.0
	 */
	public boolean canExpand(GraphNode node) {
		return layoutContext.canExpand(node.getLayout());
	}

	/**
	 * @since 2.0
	 */
	public boolean canCollapse(GraphNode node) {
		return layoutContext.canCollapse(node.getLayout());
	}

	public Graph getGraph() {
		return this;
	}

	/**
	 * @since 2.0
	 */
	public Widget getItem() {
		return this;
	}

	/**
	 * @since 2.0
	 */
	public DisplayIndependentRectangle getLayoutBounds() {
		Dimension preferredSize = this.getPreferredSize();
		return new DisplayIndependentRectangle(0, 0, preferredSize.width,
				preferredSize.height);
	}

	/**
	 * Sets the default connection router for the graph view, but does not apply
	 * it retroactively.
	 * 
	 * @param defaultConnectionRouter
	 * @since 2.0
	 */
	void setDefaultConnectionRouter(ConnectionRouter defaultConnectionRouter) {
		this.defaultConnectionRouter = defaultConnectionRouter;
	}

	/**
	 * Returns the default connection router for the graph view.
	 * 
	 * @return the default connection router; may be null.
	 * @since 2.0
	 */
	ConnectionRouter getDefaultConnectionRouter() {
		return defaultConnectionRouter;
	}

	/**
	 * Sets the default connection router for all connections that have no
	 * connection routers attached to them.
	 * 
	 * @since 2.0
	 */
	void applyConnectionRouter() {
		// for (GraphConnection conn : getConnections()){
		Iterator iterator = getConnections().iterator();
		while (iterator.hasNext()) {
			GraphConnection conn = (GraphConnection) iterator.next();
			conn.getConnectionFigure().setConnectionRouter(
					defaultConnectionRouter);
		}
		this.getRootLayer().getUpdateManager().performUpdate();
	}

	/**
	 * Updates the connection router and applies to to all existing connections
	 * that have no connection routers set to them.
	 * 
	 * @param connectionRouter
	 * @since 2.0
	 */
	public void setRouter(ConnectionRouter connectionRouter) {
		setDefaultConnectionRouter(connectionRouter);
		applyConnectionRouter();
	}

	/**
	 * Returns the ZoomManager component used for scaling the graph widget.
	 * 
	 * @return the ZoomManager component
	 * @since 2.0
	 */
	// @tag zest.bug.156286-Zooming.fix.experimental : expose the zoom manager
	// for new actions.
	public ZoomManager getZoomManager() {
		if (zoomManager == null) {
			zoomManager = new ZoomManager(getRootLayer(), getViewport());
		}
		return zoomManager;
	}
}
