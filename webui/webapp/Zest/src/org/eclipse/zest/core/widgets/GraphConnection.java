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

import org.eclipse.draw2d.ChopboxAnchor;
import org.eclipse.draw2d.ColorConstants;
import org.eclipse.draw2d.Connection;
import org.eclipse.draw2d.ConnectionRouter;
import org.eclipse.draw2d.Graphics;
import org.eclipse.draw2d.IFigure;
import org.eclipse.draw2d.Label;
import org.eclipse.draw2d.Locator;
import org.eclipse.draw2d.MidpointLocator;
import org.eclipse.draw2d.PolygonDecoration;
import org.eclipse.draw2d.PolylineConnection;
import org.eclipse.draw2d.Shape;
import org.eclipse.draw2d.geometry.Point;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.widgets.Display;
import org.eclipse.zest.core.widgets.internal.LoopAnchor;
import org.eclipse.zest.core.widgets.internal.PolylineArcConnection;
import org.eclipse.zest.core.widgets.internal.RoundedChopboxAnchor;
import org.eclipse.zest.core.widgets.internal.ZestRootLayer;
import org.eclipse.zest.layouts.interfaces.ConnectionLayout;
import org.eclipse.zest.layouts.interfaces.NodeLayout;

/*
 * This is the graph connection model which stores the source and destination
 * nodes and the properties of this connection (color, line width etc).
 * 
 * @author Chris Callendar
 * 
 * @author Ian Bull
 */
public class GraphConnection extends GraphItem {

	private Font font;
	private GraphNode sourceNode;
	private GraphNode destinationNode;

	private double weight;
	private Color color;
	private Color highlightColor;
	private Color foreground;
	private int lineWidth;
	private int lineStyle;
	private final Graph graph;

	private int connectionStyle;
	private int curveDepth;
	private boolean isDisposed = false;

	private Label connectionLabel = null;
	private PolylineArcConnection connectionFigure = null;
	private PolylineArcConnection cachedConnectionFigure = null;
	private Connection sourceContainerConnectionFigure = null;
	private Connection targetContainerConnectionFigure = null;

	/**
	 * The state of visibility set by the user.
	 */
	private boolean visible;

	private IFigure tooltip;

	private boolean highlighted;
	private boolean hasCustomTooltip;

	private ConnectionRouter router = null;

	public GraphConnection(Graph graphModel, int style, GraphNode source,
			GraphNode destination) {
		super(graphModel, style);

		this.connectionStyle |= graphModel.getConnectionStyle();
		this.connectionStyle |= style;
		this.sourceNode = source;
		this.destinationNode = destination;
		this.visible = true;
		this.color = ColorConstants.lightGray;
		this.foreground = ColorConstants.lightGray;
		this.highlightColor = graphModel.DARK_BLUE;
		this.lineWidth = 1;
		this.lineStyle = Graphics.LINE_SOLID;
		setWeight(1.0);
		this.graph = graphModel;
		this.curveDepth = 0;
		this.font = Display.getDefault().getSystemFont();
		registerConnection(source, destination);
	}

	private void registerConnection(GraphNode source, GraphNode destination) {
		if (source.getSourceConnections().contains(this)) {
			source.removeSourceConnection(this);
		}
		if (destination.getTargetConnections().contains(this)) {
			destination.removeTargetConnection(this);
		}
		(source).addSourceConnection(this);
		(destination).addTargetConnection(this);

		if (source.getParent().getItemType() == GraphItem.CONTAINER
				&& destination.getParent().getItemType() == GraphItem.CONTAINER
				&& (source.getParent() == destination.getParent())) {
			// 196189: Edges should not draw on the edge layer if both the src
			// and dest are in the same container
			// https://bugs.eclipse.org/bugs/show_bug.cgi?id=196189
			graph.addConnection(this, false);
		} else {
			graph.addConnection(this, true);
		}

		if ((source.getParent()).getItemType() == GraphItem.CONTAINER) {
			// If the container of the source is a container, we need to draw
			// another
			// arc on that arc layer
			sourceContainerConnectionFigure = doCreateFigure();
			((GraphContainer) source.getParent())
					.addConnectionFigure(sourceContainerConnectionFigure);
			this.setVisible(false);
		}

		if ((destination.getParent()).getItemType() == GraphItem.CONTAINER) { // &&
																				// src_destSameContainer
																				// ==
																				// false)
																				// {
			// If the container of the source is a container, we need to draw
			// another
			// arc on that arc layer
			targetContainerConnectionFigure = doCreateFigure();
			((GraphContainer) destination.getParent())
					.addConnectionFigure(targetContainerConnectionFigure);
			this.setVisible(false);
		}
		graph.registerItem(this);
	}

	void removeFigure() {
		if (connectionFigure.getParent() != null) {
			connectionFigure.getParent().remove(connectionFigure);
		}
		connectionFigure = null;
		if (sourceContainerConnectionFigure != null) {
			sourceContainerConnectionFigure.getParent().remove(
					sourceContainerConnectionFigure);
			sourceContainerConnectionFigure = null;
		}
		if (targetContainerConnectionFigure != null) {
			targetContainerConnectionFigure.getParent().remove(
					targetContainerConnectionFigure);
			targetContainerConnectionFigure = null;
		}

	}

	public void dispose() {
		super.dispose();
		this.isDisposed = true;
		(getSource()).removeSourceConnection(this);
		(getDestination()).removeTargetConnection(this);
		graph.removeConnection(this);
		if (sourceContainerConnectionFigure != null) {
			sourceContainerConnectionFigure.getParent().remove(
					sourceContainerConnectionFigure);
		}
		if (targetContainerConnectionFigure != null) {
			targetContainerConnectionFigure.getParent().remove(
					targetContainerConnectionFigure);
		}
	}

	public boolean isDisposed() {
		return isDisposed;
	}

	public Connection getConnectionFigure() {
		if (connectionFigure == null) {
			connectionFigure = doCreateFigure();
		}
		return connectionFigure;
	}

	/**
	 * Gets the external connection object.
	 * 
	 * @return Object
	 * @deprecated Use {@link #getData()} instead
	 */
	public Object getExternalConnection() {
		return this.getData();
	}

	/**
	 * Returns a string like 'source -> destination'
	 * 
	 * @return String
	 */
	public String toString() {
		StringBuffer buffer = new StringBuffer("GraphModelConnection: ");
		buffer.append(sourceNode != null ? sourceNode.getText() : "null");
		buffer.append(isDirected() ? " --> " : " --- ");
		buffer.append(destinationNode != null ? destinationNode.getText()
				: "null");
		buffer.append("  (weight=").append(getWeightInLayout()).append(")");
		return buffer.toString();
	}

	/**
	 * Returns the style of this connection. Valid styles are those that begin
	 * with CONNECTION in ZestStyles.
	 * 
	 * @return the style of this connection.
	 * @see #ZestStyles
	 */
	public int getConnectionStyle() {
		return connectionStyle;
	}

	/**
	 * Returns the style of this connection. Valid styles are those that begin
	 * with CONNECTION in ZestStyles.
	 * 
	 * @return the style of this connection.
	 * @see #ZestStyles
	 */
	public void setConnectionStyle(int style) {
		this.connectionStyle = style;
		updateFigure(this.connectionFigure);
	}

	/**
	 * Gets the weight of this connection. The weight must be in {-1, [0-1]}. A
	 * weight of -1 means that there is no force/tension between the nodes. A
	 * weight of 0 results in the maximum spring length being used (farthest
	 * apart). A weight of 1 results in the minimum spring length being used
	 * (closest together).
	 * 
	 * @see org.eclipse.mylar.zest.layouts.LayoutRelationship#getWeightInLayout()
	 * @return the weight: {-1, [0 - 1]}.
	 */
	public double getWeightInLayout() {
		return weight;
	}

	/**
	 * Gets the font for the label on this connection
	 * 
	 * @return
	 */
	public Font getFont() {
		return this.font;
	}

	/**
	 * Sets the font for the label on this connection.
	 * 
	 */
	public void setFont(Font f) {
		this.font = f;
	}

	/**
	 * Sets the weight for this connection. The weight must be in {-1, [0-1]}. A
	 * weight of -1 means that there is no force/tension between the nodes. A
	 * weight of 0 results in the maximum spring length being used (farthest
	 * apart). A weight of 1 results in the minimum spring length being used
	 * (closest together).
	 * 
	 */
	public void setWeight(double weight) {
		if (weight < 0) {
			this.weight = -1;
		} else if (weight > 1) {
			this.weight = 1;
		} else {
			this.weight = weight;
		}
	}

	/**
	 * Returns the color of this connection.
	 * 
	 * @return Color
	 */
	public Color getLineColor() {
		return color;
	}

	/**
	 * Sets the highlight color.
	 * 
	 * @param color
	 *            the color to use for highlighting.
	 */
	public void setHighlightColor(Color color) {
		this.highlightColor = color;
	}

	/**
	 * @return the highlight color
	 */
	public Color getHighlightColor() {
		return highlightColor;
	}

	/**
	 * Perminently sets the color of this line to the given color. This will
	 * become the color of the line when it is not highlighted. If you would
	 * like to temporarily change the color of the line, use changeLineColor.
	 * 
	 * @param color
	 *            the color to be set.
	 * @see changeLineColor(Color color)
	 */
	public void setLineColor(Color color) {
		this.foreground = color;
		changeLineColor(foreground);
	}

	/**
	 * Sets the connection color.
	 * 
	 * @param color
	 */
	public void changeLineColor(Color color) {
		this.color = color;
		updateFigure(connectionFigure);
	}

	/**
	 * Sets the tooltip on this node. This tooltip will display if the mouse
	 * hovers over the node. Setting the tooltip has no effect if a custom
	 * figure has been set.
	 */
	public void setTooltip(IFigure tooltip) {
		hasCustomTooltip = true;
		this.tooltip = tooltip;
		updateFigure(connectionFigure);
	}

	/**
	 * Gets the current tooltip for this node. The tooltip returned is
	 * meaningless if a custom figure has been set.
	 */
	public IFigure getTooltip() {
		return this.tooltip;
	}

	/**
	 * Returns the connection line width.
	 * 
	 * @return int
	 */
	public int getLineWidth() {
		return lineWidth;
	}

	/**
	 * Sets the connection line width.
	 * 
	 * @param lineWidth
	 */
	public void setLineWidth(int lineWidth) {
		this.lineWidth = lineWidth;
		updateFigure(connectionFigure);
	}

	/**
	 * Returns the connection line style.
	 * 
	 * @return int
	 */
	public int getLineStyle() {
		return lineStyle;
	}

	/**
	 * Sets the connection line style.
	 * 
	 * @param lineStyle
	 */
	public void setLineStyle(int lineStyle) {
		this.lineStyle = lineStyle;
		updateFigure(connectionFigure);
	}

	/**
	 * Gets the source node for this relationship
	 * 
	 * @return GraphModelNode
	 */
	public GraphNode getSource() {
		return this.sourceNode;
	}

	/**
	 * Gets the target node for this relationship
	 * 
	 * @return GraphModelNode
	 */
	public GraphNode getDestination() {
		return this.destinationNode;
	}

	/**
	 * Highlights this node. Uses the default highlight color.
	 */
	public void highlight() {
		if (highlighted) {
			return;
		}
		IFigure parentFigure = connectionFigure.getParent();
		if (parentFigure instanceof ZestRootLayer) {
			((ZestRootLayer) parentFigure)
					.highlightConnection(connectionFigure);
		}
		highlighted = true;
		updateFigure(connectionFigure);
	}

	/**
	 * Unhighlights this node. Uses the default color.
	 */
	public void unhighlight() {
		if (!highlighted) {
			return;
		}
		IFigure parentFigure = connectionFigure.getParent();
		if (parentFigure instanceof ZestRootLayer) {
			((ZestRootLayer) parentFigure)
					.unHighlightConnection(connectionFigure);
		}
		highlighted = false;
		updateFigure(connectionFigure);
	}

	/**
	 * Returns true if this connection is highlighted, false otherwise
	 * 
	 * @return
	 */
	public boolean isHighlighted() {
		return highlighted;
	}

	/**
	 * Gets the graph model that this connection is in
	 * 
	 * @return The graph model that this connection is contained in
	 */
	public Graph getGraphModel() {
		return this.graph;
	}

	/**
	 * Sets the curve depth of the arc. The curve depth is defined as the
	 * maximum distance from any point on the chord (i.e. a vector normal to the
	 * chord with magnitude d).
	 * 
	 * If 0 is set, a Polyline Connection will be used, otherwise a
	 * PolylineArcConnectoin will be used. Negative depths are also supported.
	 * 
	 * @param depth
	 *            The depth of the curve
	 */
	public void setCurveDepth(int depth) {
		if (this.curveDepth == 0 && depth != 0 || this.curveDepth != 0
				&& depth == 0) {
			// There is currently no curve, so we have to create
			// a curved connection
			this.cachedConnectionFigure = connectionFigure;
			graph.removeConnection(this);
			this.curveDepth = depth;
			this.connectionFigure = doCreateFigure();
			registerConnection(sourceNode, destinationNode);
			updateFigure(this.connectionFigure);
		} else {
			this.curveDepth = depth;
			updateFigure(this.connectionFigure);
		}
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.mylar.zest.core.widgets.IGraphItem#getItemType()
	 */
	public int getItemType() {
		return CONNECTION;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.eclipse.mylar.zest.core.internal.graphmodel.GraphItem#setVisible(
	 * boolean)
	 */
	public void setVisible(boolean visible) {
		if (getSource().isVisible() && getDestination().isVisible() && visible) {
			this.getFigure().setVisible(visible);
			if (sourceContainerConnectionFigure != null) {
				sourceContainerConnectionFigure.setVisible(visible);
			}
			if (targetContainerConnectionFigure != null) {
				targetContainerConnectionFigure.setVisible(visible);
			}
			this.visible = visible;
		} else {
			this.getFigure().setVisible(false);
			if (sourceContainerConnectionFigure != null) {
				sourceContainerConnectionFigure.setVisible(false);
			}
			if (targetContainerConnectionFigure != null) {
				targetContainerConnectionFigure.setVisible(false);
			}
			this.visible = false;
		}

	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.mylar.zest.core.widgets.IGraphItem#isVisible()
	 */
	public boolean isVisible() {
		return visible;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.swt.widgets.Item#setText(java.lang.String)
	 */
	public void setText(String string) {
		super.setText(string);

		if (this.connectionFigure != null) {
			updateFigure(this.connectionFigure);
		}
	}

	/**
	 * @since 2.0
	 */
	public boolean isDirected() {
		return ZestStyles.checkStyle(connectionStyle,
				ZestStyles.CONNECTIONS_DIRECTED);
	}

	/**
	 * @since 2.0
	 */
	public void setDirected(boolean directed) {
		if (directed) {
			setConnectionStyle(connectionStyle
					| ZestStyles.CONNECTIONS_DIRECTED);
		} else {
			setConnectionStyle(connectionStyle
					& (-1 - ZestStyles.CONNECTIONS_DIRECTED));
		}
	}

	PolylineConnection getSourceContainerConnectionFigure() {
		return (PolylineConnection) sourceContainerConnectionFigure;
	}

	PolylineConnection getTargetContainerConnectionFigure() {
		return (PolylineConnection) targetContainerConnectionFigure;
	}

	private void updateFigure(PolylineArcConnection connection) {
		if (sourceContainerConnectionFigure != null) {
			doUpdateFigure(sourceContainerConnectionFigure);
		}
		if (targetContainerConnectionFigure != null) {
			doUpdateFigure(targetContainerConnectionFigure);
		}
		doUpdateFigure(connection);
	}

	private void doUpdateFigure(Connection connection) {
		if (connection == null || this.isDisposed()) {
			return;
		}
		Shape connectionShape = (Shape) connection;

		connectionShape.setLineStyle(getLineStyle());

		if (this.getText() != null || this.getImage() != null) {
			if (this.getImage() != null) {
				this.connectionLabel.setIcon(this.getImage());
			}
			if (this.getText() != null) {
				this.connectionLabel.setText(this.getText());
			}
			this.connectionLabel.setFont(this.getFont());
		}

		if (highlighted) {
			connectionShape.setForegroundColor(getHighlightColor());
			connectionShape.setLineWidth(getLineWidth() * 2);
		} else {
			connectionShape.setForegroundColor(getLineColor());
			connectionShape.setLineWidth(getLineWidth());
		}
		if (connection instanceof PolylineArcConnection) {
			PolylineArcConnection arcConnection = (PolylineArcConnection) connection;
			arcConnection.setDepth(curveDepth);
		}
		if (connectionFigure != null) {
			applyConnectionRouter(connectionFigure);
		}
		if ((connectionStyle & ZestStyles.CONNECTIONS_DIRECTED) > 0) {
			PolygonDecoration decoration = new PolygonDecoration();
			if (getLineWidth() < 3) {
				decoration.setScale(9, 3);
			} else {
				double logLineWith = getLineWidth() / 2.0;
				decoration.setScale(7 * logLineWith, 3 * logLineWith);
			}
			if (connection instanceof PolylineConnection) {
				((PolylineArcConnection) connection)
						.setTargetDecoration(decoration);
			}
		}

		IFigure toolTip;
		if (this.getTooltip() == null && getText() != null
				&& getText().length() > 0 && hasCustomTooltip == false) {
			toolTip = new Label();
			((Label) toolTip).setText(getText());
		} else {
			toolTip = this.getTooltip();
		}
		connection.setToolTip(toolTip);
	}

	private PolylineArcConnection doCreateFigure() {
		PolylineArcConnection connectionFigure = cachedOrNewConnectionFigure();
		ChopboxAnchor sourceAnchor = null;
		ChopboxAnchor targetAnchor = null;
		this.connectionLabel = new Label();
		Locator labelLocator = null;

		if (getSource() == getDestination()) {
			// If this is a self loop, create a looped arc and put the locator
			// at the top of the connection
			sourceAnchor = new LoopAnchor(getSource().getNodeFigure());
			targetAnchor = new LoopAnchor(getDestination().getNodeFigure());
			labelLocator = new MidpointLocator(connectionFigure, 0) {
				protected Point getReferencePoint() {
					Point p = Point.SINGLETON;
					p.x = getConnection().getPoints().getPoint(getIndex()).x;
					p.y = (int) (getConnection().getPoints().getPoint(
							getIndex()).y - (curveDepth * 1.5));
					getConnection().translateToAbsolute(p);
					return p;
				}
			};
		} else {
			if (curveDepth != 0) {
				connectionFigure.setDepth(this.curveDepth);
			}
			applyConnectionRouter(connectionFigure);
			sourceAnchor = new RoundedChopboxAnchor(
					getSource().getNodeFigure(), 8);
			targetAnchor = new RoundedChopboxAnchor(getDestination()
					.getNodeFigure(), 8);
			labelLocator = new MidpointLocator(connectionFigure, 0);
		}

		connectionFigure.setSourceAnchor(sourceAnchor);
		connectionFigure.setTargetAnchor(targetAnchor);
		connectionFigure.add(this.connectionLabel, labelLocator);

		doUpdateFigure(connectionFigure);
		return connectionFigure;
	}

	private PolylineArcConnection cachedOrNewConnectionFigure() {
		return cachedConnectionFigure == null ? new PolylineArcConnection()
				: cachedConnectionFigure;
	}

	IFigure getFigure() {
		return this.getConnectionFigure();
	}

	private InternalConnectionLayout layout;

	InternalConnectionLayout getLayout() {
		if (layout == null) {
			layout = new InternalConnectionLayout();
		}
		return layout;
	}

	class InternalConnectionLayout implements ConnectionLayout {
		private boolean visible = GraphConnection.this.isVisible();

		public NodeLayout getSource() {
			return sourceNode.getLayout();
		}

		public NodeLayout getTarget() {
			return destinationNode.getLayout();
		}

		public double getWeight() {
			return GraphConnection.this.getWeightInLayout();
		}

		public boolean isDirected() {
			return !ZestStyles.checkStyle(getConnectionStyle(),
					ZestStyles.CONNECTIONS_DIRECTED);
		}

		public boolean isVisible() {
			return visible;
		}

		public void setVisible(boolean visible) {
			graph.getLayoutContext().checkChangesAllowed();
			this.visible = visible;
		}

		void applyLayout() {
			if (GraphConnection.this.isVisible() != this.visible) {
				GraphConnection.this.setVisible(this.visible);
			}
		}
	}

	void applyLayoutChanges() {
		if (layout != null) {
			layout.applyLayout();
		}
	}

	/**
	 * Applies the connection router with a possible fallback to the default
	 * connection router to the graph
	 * 
	 * @param conn
	 * @since 2.0
	 */
	void applyConnectionRouter(Connection conn) {
		if (router != null) {
			conn.setConnectionRouter(router);
		} else if (graph.getDefaultConnectionRouter() != null) {
			conn.setConnectionRouter(graph.getDefaultConnectionRouter());
		}
	}

	/**
	 * Sets the connection router of the connection
	 * 
	 * @param router
	 * @since 2.0
	 */
	public void setRouter(ConnectionRouter router) {
		this.router = router;
	}

}
