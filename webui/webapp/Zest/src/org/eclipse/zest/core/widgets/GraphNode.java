/*******************************************************************************
 * Copyright 2005-2010, CHISEL Group, University of Victoria, Victoria, BC,
 * Canada. All rights reserved. This program and the accompanying materials are
 * made available under the terms of the Eclipse Public License v1.0 which
 * accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors: The Chisel Group, University of Victoria Mateusz Matela
 ******************************************************************************/
package org.eclipse.zest.core.widgets;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import org.eclipse.draw2d.Animation;
import org.eclipse.draw2d.ColorConstants;
import org.eclipse.draw2d.FigureListener;
import org.eclipse.draw2d.IFigure;
import org.eclipse.draw2d.Label;
import org.eclipse.draw2d.geometry.Dimension;
import org.eclipse.draw2d.geometry.Insets;
import org.eclipse.draw2d.geometry.Point;
import org.eclipse.draw2d.geometry.PrecisionPoint;
import org.eclipse.draw2d.geometry.Rectangle;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.widgets.Display;
import org.eclipse.zest.core.widgets.internal.GraphLabel;
import org.eclipse.zest.core.widgets.internal.ZestRootLayer;

/**
 * Simple node class which has the following properties: color, size, location,
 * and a label. It also has a list of connections and anchors.
 * 
 * @author Chris Callendar
 * 
 * @author Del Myers
 * 
 * @author Ian Bull
 */
public class GraphNode extends GraphItem {
	public static final int HIGHLIGHT_NONE = 0;
	public static final int HIGHLIGHT_ON = 1;

	private int nodeStyle;

	private List /* IGraphModelConnection */sourceConnections;
	private List /* IGraphModelConnection */targetConnections;

	private Color foreColor;
	private Color backColor;
	private Color highlightColor;
	private Color borderColor;
	private Color borderHighlightColor;
	private int borderWidth;
	private PrecisionPoint currentLocation;
	protected Dimension size;
	private Font font;
	private boolean cacheLabel;
	private boolean visible = true;

	protected Graph graph;
	protected IContainer parent;

	/** The internal node. */
	protected Object internalNode;
	private boolean selected;
	protected int highlighted = HIGHLIGHT_NONE;
	private IFigure tooltip;
	protected IFigure nodeFigure;

	private boolean isDisposed = false;
	private boolean hasCustomTooltip;

	public GraphNode(IContainer graphModel, int style) {
		// TODO remove cast when deprecated API is removed
		this(graphModel, style, (IFigure) null);
	}

	public GraphNode(IContainer graphModel, int style, String text) {
		this(graphModel, style, text, null, null);
	}

	/**
	 * @deprecated Since Zest 2.0, use {@link #GraphNode(IContainer, int)} and
	 *             {@link #setData(Object)}
	 */
	public GraphNode(IContainer graphModel, int style, Object data) {
		this(graphModel, style, "" /* text */, null /* image */, data);
	}

	/**
	 * @deprecated Since Zest 2.0, use {@link #GraphNode(IContainer, int)},
	 *             {@link #setText(String)}, and {@link #setImage(Image)}
	 */
	public GraphNode(IContainer graphModel, int style, String text, Image image) {
		this(graphModel, style, text, image, null);
	}

	protected GraphNode(IContainer graphModel, int style, IFigure figure) {
		this(graphModel, style, "", null, figure);
	}

	// TODO change Object to IFigure when deprecated API is removed
	private GraphNode(IContainer graphModel, int style, String text,
			Image image, Object data) {
		super(graphModel, style, data);
		initModel(graphModel, text, image);
		if (nodeFigure == null) {
			initFigure();
		}

		this.parent.addNode(this);
		this.parent.getGraph().registerItem(this);
	}

	protected void initFigure() {
		nodeFigure = createFigureForModel();
	}

	static int count = 0;

	protected void initModel(IContainer graphModel, String text, Image image) {
		this.nodeStyle |= graphModel.getGraph().getNodeStyle();
		this.parent = graphModel;
		this.sourceConnections = new ArrayList();
		this.targetConnections = new ArrayList();
		this.foreColor = graphModel.getGraph().DARK_BLUE;
		this.backColor = graphModel.getGraph().LIGHT_BLUE;
		this.highlightColor = graphModel.getGraph().HIGHLIGHT_COLOR;
		this.borderColor = ColorConstants.lightGray;
		this.borderHighlightColor = ColorConstants.blue;
		this.borderWidth = 1;
		this.currentLocation = new PrecisionPoint(0, 0);
		this.size = new Dimension(-1, -1);
		this.font = Display.getDefault().getSystemFont();
		this.graph = graphModel.getGraph();
		this.cacheLabel = false;
		this.setText(text);
		if (image != null) {
			this.setImage(image);
		}

		if (font == null) {
			font = Display.getDefault().getSystemFont();
		}

	}

	/**
	 * A simple toString that we can use for debugging
	 */
	public String toString() {
		return "GraphModelNode: " + getText();
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.mylar.zest.core.widgets.GraphItem#dispose()
	 */
	public void dispose() {
		if (isFisheyeEnabled) {
			this.fishEye(false, false);
		}
		super.dispose();
		this.isDisposed = true;
		while (getSourceConnections().size() > 0) {
			GraphConnection connection = (GraphConnection) getSourceConnections()
					.get(0);
			if (!connection.isDisposed()) {
				connection.dispose();
			} else {
				removeSourceConnection(connection);
			}
		}
		while (getTargetConnections().size() > 0) {
			GraphConnection connection = (GraphConnection) getTargetConnections()
					.get(0);
			if (!connection.isDisposed()) {
				connection.dispose();
			} else {
				removeTargetConnection(connection);
			}
		}
		graph.removeNode(this);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.swt.widgets.Widget#isDisposed()
	 */
	public boolean isDisposed() {
		return isDisposed;
	}

	/**
	 * Determines if this node has a fixed size or if it is packed to the size
	 * of its contents. To set a node to pack, set its size (-1, -1)
	 * 
	 * @return
	 */
	public boolean isSizeFixed() {
		return !(this.size.width < 0 && this.size.height < 0);
	}

	/**
	 * Returns a new list of the source connections (GraphModelConnection
	 * objects).
	 * 
	 * @return List a new list of GraphModelConnect objects
	 */
	public List getSourceConnections() {
		return new ArrayList(sourceConnections);
	}

	/**
	 * Returns a new list of the target connections (GraphModelConnection
	 * objects).
	 * 
	 * @return List a new list of GraphModelConnect objects
	 */
	public List getTargetConnections() {
		return new ArrayList(targetConnections);
	}

	/**
	 * Returns the bounds of this node. It is just the combination of the
	 * location and the size.
	 * 
	 * @return Rectangle
	 */
	Rectangle getBounds() {
		return new Rectangle(getLocation(), getSize());
	}

	/**
	 * Returns a copy of the node's location.
	 * 
	 * @return Point
	 */
	public Point getLocation() {
		return currentLocation;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.eclipse.mylar.zest.core.internal.graphmodel.IGraphModelNode#isSelected
	 * ()
	 */
	public boolean isSelected() {
		return selected;
	}

	/**
	 * Sets the current location for this node.
	 */
	public void setLocation(double x, double y) {
		if (currentLocation.preciseX() != x || currentLocation.preciseY() != y) {
			currentLocation.setPreciseX(x);
			currentLocation.setPreciseY(y);
			refreshBounds();
			if (getGraphModel().isDynamicLayoutEnabled()) {
				parent.getLayoutContext().fireNodeMovedEvent(this.getLayout());
			}
		}
	}

	/**
	 * Returns a copy of the node's size.
	 * 
	 * @return Dimension
	 */
	public Dimension getSize() {
		if (size.height < 0 && size.width < 0 && nodeFigure != null) {
			return nodeFigure.getSize().getCopy();
		}
		return size.getCopy();
	}

	/**
	 * Get the foreground colour for this node
	 */
	public Color getForegroundColor() {
		return foreColor;
	}

	/**
	 * Set the foreground colour for this node
	 */
	public void setForegroundColor(Color c) {
		this.foreColor = c;
		updateFigureForModel(nodeFigure);
	}

	/**
	 * Get the background colour for this node. This is the color the node will
	 * be if it is not currently highlighted. This color is meaningless if a
	 * custom figure has been set.
	 */
	public Color getBackgroundColor() {
		return backColor;
	}

	/**
	 * Permanently sets the background color (unhighlighted). This color has no
	 * effect if a custom figure has been set.
	 * 
	 * @param c
	 */
	public void setBackgroundColor(Color c) {
		backColor = c;
		updateFigureForModel(nodeFigure);
	}

	/**
	 * Sets the tooltip on this node. This tooltip will display if the mouse
	 * hovers over the node. Setting the tooltip has no effect if a custom
	 * figure has been set.
	 */
	public void setTooltip(IFigure tooltip) {
		hasCustomTooltip = true;
		this.tooltip = tooltip;
		updateFigureForModel(nodeFigure);
	}

	/**
	 * Gets the current tooltip for this node. The tooltip returned is
	 * meaningless if a custom figure has been set.
	 */
	public IFigure getTooltip() {
		return this.tooltip;
	}

	/**
	 * Sets the border color.
	 * 
	 * @param c
	 *            the border color.
	 */
	public void setBorderColor(Color c) {
		borderColor = c;
		updateFigureForModel(nodeFigure);
	}

	/**
	 * Sets the highlighted border color.
	 * 
	 * @param c
	 *            the highlighted border color.
	 */
	public void setBorderHighlightColor(Color c) {
		this.borderHighlightColor = c;
		updateFigureForModel(nodeFigure);
	}

	/**
	 * Get the highlight colour for this node
	 */
	public Color getHighlightColor() {
		return highlightColor;
	}

	/**
	 * Set the highlight colour for this node
	 */
	public void setHighlightColor(Color c) {
		this.highlightColor = c;
	}

	/**
	 * Highlights the node changing the background color and border color. The
	 * source and destination connections are also highlighted, and the adjacent
	 * nodes are highlighted too in a different color.
	 */
	public void highlight() {
		if (highlighted == HIGHLIGHT_ON) {
			return;
		}
		IFigure parentFigure = nodeFigure.getParent();
		if (parentFigure instanceof ZestRootLayer) {
			((ZestRootLayer) parentFigure).highlightNode(nodeFigure);
		}
		highlighted = HIGHLIGHT_ON;
		updateFigureForModel(getNodeFigure());
	}

	/**
	 * Restores the nodes original background color and border width.
	 */
	public void unhighlight() {

		if (highlighted == HIGHLIGHT_NONE) {
			return;
		}

		IFigure parentFigure = nodeFigure.getParent();
		if (parentFigure instanceof ZestRootLayer) {
			((ZestRootLayer) parentFigure).unHighlightNode(nodeFigure);
		}

		highlighted = HIGHLIGHT_NONE;
		updateFigureForModel(nodeFigure);

	}

	void refreshBounds() {
		Point loc = this.getLocation();
		Dimension size = this.getSize();
		Rectangle bounds = new Rectangle(loc, size);

		if (nodeFigure == null || nodeFigure.getParent() == null) {
			return; // node figure has not been created yet
		}
		nodeFigure.getParent().setConstraint(nodeFigure, bounds);

		if (isFisheyeEnabled) {
			Rectangle fishEyeBounds = calculateFishEyeBounds();
			if (fishEyeBounds != null) {
				fishEyeFigure.getParent().translateToRelative(fishEyeBounds);
				fishEyeFigure.getParent().translateFromParent(fishEyeBounds);
				fishEyeFigure.getParent().setConstraint(fishEyeFigure,
						fishEyeBounds);
			}
		}
	}

	public Color getBorderColor() {
		return borderColor;
	}

	public int getBorderWidth() {
		return borderWidth;
	}

	public void setBorderWidth(int width) {
		this.borderWidth = width;
		updateFigureForModel(nodeFigure);
	}

	public Font getFont() {
		return font;
	}

	public void setFont(Font font) {
		this.font = font;
		updateFigureForModel(nodeFigure);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.swt.widgets.Item#setText(java.lang.String)
	 */
	public void setText(String string) {
		if (string == null) {
			string = "";
		}
		super.setText(string);

		updateFigureForModel(this.nodeFigure);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.eclipse.swt.widgets.Item#setImage(org.eclipse.swt.graphics.Image)
	 */
	public void setImage(Image image) {
		super.setImage(image);
		updateFigureForModel(nodeFigure);
	}

	/**
	 * Gets the graphModel that this node is contained in
	 * 
	 * @return The graph model that this node is contained in
	 */
	public Graph getGraphModel() {
		return this.graph;
	}

	/**
	 * @return the nodeStyle
	 */
	public int getNodeStyle() {
		return nodeStyle;
	}

	/**
	 * @param nodeStyle
	 *            the nodeStyle to set
	 */
	public void setNodeStyle(int nodeStyle) {
		this.nodeStyle = nodeStyle;
		this.cacheLabel = ((this.nodeStyle & ZestStyles.NODES_CACHE_LABEL) > 0) ? true
				: false;
	}

	public void setSize(double width, double height) {
		if ((width != size.width) || (height != size.height)) {
			size.width = (int) width;
			size.height = (int) height;
			refreshBounds();
		}
	}

	public Color getBorderHighlightColor() {
		return borderHighlightColor;
	}

	public boolean cacheLabel() {
		return this.cacheLabel;
	}

	public void setCacheLabel(boolean cacheLabel) {
		this.cacheLabel = cacheLabel;
	}

	IFigure getNodeFigure() {
		return this.nodeFigure;
	}

	public void setVisible(boolean visible) {
		this.visible = visible;
		this.getFigure().setVisible(visible);
		for (Iterator iterator2 = sourceConnections.iterator(); iterator2
				.hasNext();) {
			GraphConnection connection = (GraphConnection) iterator2.next();
			connection.setVisible(visible);
		}

		for (Iterator iterator2 = targetConnections.iterator(); iterator2
				.hasNext();) {
			GraphConnection connection = (GraphConnection) iterator2.next();
			connection.setVisible(visible);
		}
	}

	public boolean isVisible() {
		return visible;
	}

	public int getStyle() {
		return super.getStyle() | this.getNodeStyle();
	}

	/***************************************************************************
	 * PRIVATE MEMBERS
	 **************************************************************************/

	private IFigure fishEyeFigure = null;
	private boolean isFisheyeEnabled;

	protected IFigure fishEye(boolean enable, boolean animate) {
		if (isDisposed) {
			// If a fisheyed figure is still left on the canvas, we could get
			// called once more after the dispose is called. Since we cleaned
			// up everything on dispose, we can just return null here.
			return null;
		}
		if (!checkStyle(ZestStyles.NODES_FISHEYE)) {
			return null;
		}
		if (enable) {
			// Create the fish eye label
			fishEyeFigure = createFishEyeFigure();

			Rectangle rectangle = calculateFishEyeBounds();

			if (rectangle == null) {
				return null;
			}

			// Add the fisheye
			this.getGraphModel().fishEye(nodeFigure, fishEyeFigure, rectangle,
					true);
			if (fishEyeFigure != null) {
				isFisheyeEnabled = true;
			}
			return fishEyeFigure;

		} else {
			isFisheyeEnabled = false;
			this.getGraphModel().removeFishEye(fishEyeFigure, nodeFigure,
					animate);
			return null;
		}
	}

	IContainer getParent() {
		return parent;
	}

	boolean isHighlighted() {
		return highlighted > 0;
	}

	protected void updateFigureForModel(IFigure currentFigure) {
		if (currentFigure == null) {
			return;
		}

		IFigure figure = currentFigure;
		IFigure toolTip;

		if (figure instanceof ILabeledFigure) {
			// update label text/icon for figures implementing ILabeledFigure
			ILabeledFigure labeledFigure = (ILabeledFigure) figure;
			if (!checkStyle(ZestStyles.NODES_HIDE_TEXT)
					&& !labeledFigure.getText().equals(this.getText())) {
				labeledFigure.setText(this.getText());
			}
			if (labeledFigure.getIcon() != getImage()) {
				labeledFigure.setIcon(getImage());
			}
		}

		if (figure instanceof IStyleableFigure) {
			// update styles (colors, border) for figures implementing
			// IStyleableFigure
			IStyleableFigure styleableFigure = (IStyleableFigure) figure;
			if (highlighted == HIGHLIGHT_ON) {
				styleableFigure.setForegroundColor(getForegroundColor());
				styleableFigure.setBackgroundColor(getHighlightColor());
				styleableFigure.setBorderColor(getBorderHighlightColor());
			} else {
				styleableFigure.setForegroundColor(getForegroundColor());
				styleableFigure.setBackgroundColor(getBackgroundColor());
				styleableFigure.setBorderColor(getBorderColor());
			}

			styleableFigure.setBorderWidth(getBorderWidth());

			if (figure.getFont() != getFont()) {
				figure.setFont(getFont());
			}
		}

		if (this.getTooltip() == null && hasCustomTooltip == false) {
			// if we have a custom tooltip, don't try and create our own.
			toolTip = new Label();
			((Label) toolTip).setText(getText());
		} else {
			toolTip = this.getTooltip();
		}
		figure.setToolTip(toolTip);

		if (isFisheyeEnabled) {
			IFigure newFisheyeFigure = createFishEyeFigure();
			if (graph.replaceFishFigure(this.fishEyeFigure, newFisheyeFigure)) {
				this.fishEyeFigure = newFisheyeFigure;
			}
		}

		refreshBounds();
	}

	protected IFigure createFigureForModel() {
		GraphNode node = this;
		boolean cacheLabel = (this).cacheLabel();
		final GraphLabel label = new GraphLabel(node.getText(),
				node.getImage(), cacheLabel);
		label.setFont(this.font);
		if (checkStyle(ZestStyles.NODES_HIDE_TEXT)) {
			label.setText("");
		}
		updateFigureForModel(label);
		label.addFigureListener(new FigureListener() {
			private Dimension previousSize = label.getBounds().getSize();

			public void figureMoved(IFigure source) {
				if (Animation.isAnimating() || getLayout().isMinimized()) {
					return;
				}
				Rectangle newBounds = nodeFigure.getBounds();
				if (!newBounds.getSize().equals(previousSize)) {
					previousSize = newBounds.getSize();
					if (size.width >= 0 && size.height >= 0) {
						size = newBounds.getSize();
					}
					currentLocation = new PrecisionPoint(nodeFigure.getBounds()
							.getTopLeft());
					parent.getLayoutContext().fireNodeResizedEvent(getLayout());
				} else if (currentLocation.x != newBounds.x
						|| currentLocation.y != newBounds.y) {
					currentLocation = new PrecisionPoint(nodeFigure.getBounds()
							.getTopLeft());
					parent.getLayoutContext().fireNodeMovedEvent(getLayout());
				}
			}
		});
		return label;
	}

	private IFigure createFishEyeFigure() {
		GraphNode node = this;
		boolean cacheLabel = this.cacheLabel();
		GraphLabel label = new GraphLabel(node.getText(), node.getImage(),
				cacheLabel);

		if (highlighted == HIGHLIGHT_ON) {
			label.setForegroundColor(getForegroundColor());
			label.setBackgroundColor(getHighlightColor());
			label.setBorderColor(getBorderHighlightColor());
		} else {
			label.setForegroundColor(getForegroundColor());
			label.setBackgroundColor(getBackgroundColor());
			label.setBorderColor(getBorderColor());
		}

		label.setBorderWidth(getBorderWidth());
		label.setFont(getFont());

		return label;
	}

	private Rectangle calculateFishEyeBounds() {
		// Get the current Bounds
		Rectangle rectangle = nodeFigure.getBounds().getCopy();

		// Calculate how much we have to expand the current bounds to get to the
		// new bounds
		Dimension newSize = fishEyeFigure.getPreferredSize();
		Rectangle currentSize = rectangle.getCopy();
		nodeFigure.translateToAbsolute(currentSize);
		int expandedH = Math.max((newSize.height - currentSize.height) / 2 + 1,
				0);
		int expandedW = Math
				.max((newSize.width - currentSize.width) / 2 + 1, 0);
		Dimension expandAmount = new Dimension(expandedW, expandedH);
		nodeFigure.translateToAbsolute(rectangle);
		rectangle.expand(new Insets(expandAmount.height, expandAmount.width,
				expandAmount.height, expandAmount.width));
		if (expandedH <= 0 && expandedW <= 0) {
			return null;
		}
		return rectangle;
	}

	void addSourceConnection(GraphConnection connection) {
		this.sourceConnections.add(connection);
	}

	void addTargetConnection(GraphConnection connection) {
		this.targetConnections.add(connection);
	}

	void removeSourceConnection(GraphConnection connection) {
		this.sourceConnections.remove(connection);
	}

	void removeTargetConnection(GraphConnection connection) {
		this.targetConnections.remove(connection);
	}

	/**
	 * Sets the node as selected.
	 */
	void setSelected(boolean selected) {
		if (selected == isSelected()) {
			return;
		}
		if (selected) {
			highlight();
		} else {
			unhighlight();
		}
		this.selected = selected;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.mylar.zest.core.widgets.IGraphItem#getItemType()
	 */
	public int getItemType() {
		return NODE;
	}

	/**
	 * @noreference This method is not intended to be referenced by clients.
	 */
	public IFigure getFigure() {
		if (this.nodeFigure == null) {
			initFigure();
		}
		return this.getNodeFigure();
	}

	private InternalNodeLayout layout;

	/**
	 * @noreference This method is not intended to be referenced by clients.
	 */
	public InternalNodeLayout getLayout() {
		if (layout == null) {
			layout = new InternalNodeLayout(this);
		}
		return layout;
	}

	void applyLayoutChanges() {
		if (layout != null) {
			layout.applyLayout();
		}
	}
}
