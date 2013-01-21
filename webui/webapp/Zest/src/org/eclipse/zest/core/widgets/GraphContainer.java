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
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;

import org.eclipse.draw2d.ActionEvent;
import org.eclipse.draw2d.ActionListener;
import org.eclipse.draw2d.Animation;
import org.eclipse.draw2d.Clickable;
import org.eclipse.draw2d.ColorConstants;
import org.eclipse.draw2d.Figure;
import org.eclipse.draw2d.FreeformLayout;
import org.eclipse.draw2d.FreeformViewport;
import org.eclipse.draw2d.Graphics;
import org.eclipse.draw2d.IFigure;
import org.eclipse.draw2d.Label;
import org.eclipse.draw2d.LayoutAnimator;
import org.eclipse.draw2d.LineBorder;
import org.eclipse.draw2d.ScrollPane;
import org.eclipse.draw2d.ToolbarLayout;
import org.eclipse.draw2d.Triangle;
import org.eclipse.draw2d.Viewport;
import org.eclipse.draw2d.geometry.Dimension;
import org.eclipse.draw2d.geometry.Point;
import org.eclipse.draw2d.geometry.Rectangle;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Widget;
import org.eclipse.zest.core.widgets.internal.AspectRatioFreeformLayer;
import org.eclipse.zest.core.widgets.internal.ContainerFigure;
import org.eclipse.zest.core.widgets.internal.ZestRootLayer;
import org.eclipse.zest.layouts.LayoutAlgorithm;
import org.eclipse.zest.layouts.algorithms.TreeLayoutAlgorithm;
import org.eclipse.zest.layouts.dataStructures.DisplayIndependentRectangle;

/**
 * A Container that can be added to a Graph. Nodes can be added to this
 * container. The container supports collapsing and expanding and has the same
 * properties as the nodes. Containers cannot have custom figures.
 * 
 * @author Ian Bull
 */
@SuppressWarnings({ "rawtypes", "unchecked" })
public class GraphContainer extends GraphNode implements IContainer {
	private static final long serialVersionUID = 1L;

	static class ExpandGraphLabel extends Figure implements ActionListener {

		private boolean isExpanded;
		private Expander expander = new Expander();
		private Color darkerBackground;

		class Expander extends Clickable {
			private Triangle triangle;

			public Expander() {
				setStyle(Clickable.STYLE_TOGGLE);
				triangle = new Triangle();
				triangle.setSize(10, 10);
				triangle.setBackgroundColor(ColorConstants.black);
				triangle.setForegroundColor(ColorConstants.black);
				triangle.setFill(true);
				triangle.setDirection(Triangle.EAST);
				triangle.setLocation(new Point(5, 3));
				this.setLayoutManager(new FreeformLayout());
				this.add(triangle);
				this.setPreferredSize(15, 15);
				this.addActionListener(ExpandGraphLabel.this);
			}

			public void open() {
				triangle.setDirection(Triangle.SOUTH);
			}

			public void close() {
				triangle.setDirection(Triangle.EAST);
			}

		}

		/**
		 * Sets the expander state (the little triangle) to
		 * ExpanderGraphLabel.OPEN or ExpanderGraphLabel.CLOSED
		 * 
		 * @param state
		 */
		public void setExpandedState(boolean expanded) {
			if (expanded) {
				expander.open();
			} else {
				expander.close();
			}
			this.isExpanded = expanded;
		}

		/*
		 * (non-Javadoc)
		 * 
		 * @see
		 * org.eclipse.draw2d.ActionListener#actionPerformed(org.eclipse.draw2d
		 * .ActionEvent)
		 */
		public void actionPerformed(ActionEvent event) {
			if (isExpanded) {
				container.close(true);
			} else {
				container.open(true);
			}
		}

		private final int arcWidth = 8;
		private final Label label;
		private final GraphContainer container;
		private final ToolbarLayout layout;

		public ExpandGraphLabel(GraphContainer container, String text,
				Image image, boolean cacheLabel) {
			this.label = new Label(text) {

				/**
				 * <b>This method is overwritten so that the text is not
				 * truncated.</b><br>
				 * 
				 * {@inheritDoc}
				 * 
				 */
				protected void paintFigure(Graphics graphics) {
					if (isOpaque()) {
						super.paintFigure(graphics);
					}
					Rectangle bounds = getBounds();
					graphics.translate(bounds.x, bounds.y);
					if (getIcon() != null) {
						graphics.drawImage(getIcon(), getIconLocation());
					}
					if (!isEnabled()) {
						graphics.translate(1, 1);
						graphics.setForegroundColor(ColorConstants.buttonLightest);
						graphics.drawText(getSubStringText(), getTextLocation());
						graphics.translate(-1, -1);
						graphics.setForegroundColor(ColorConstants.buttonDarker);
					}
					graphics.drawText(getText(), getTextLocation());
					graphics.translate(-bounds.x, -bounds.y);
				}
			};
			this.setText(text);
			this.setImage(image);
			this.container = container;
			this.setFont(Display.getDefault().getSystemFont());
			layout = new ToolbarLayout(true);
			layout.setSpacing(5);
			layout.setMinorAlignment(ToolbarLayout.ALIGN_CENTER);
			this.setLayoutManager(layout);
			this.add(this.expander);
			this.add(this.label);
		}

		private Color getDarkerBackgroundColor() {
			if (darkerBackground == null) {
				Color baseColor = getBackgroundColor();
				int blue = (int) (baseColor.getBlue() * 0.8 + 0.5);
				int red = (int) (baseColor.getRed() * 0.8 + 0.5);
				int green = (int) (baseColor.getGreen() * 0.8 + 0.5);
				darkerBackground = new Color(Display.getCurrent(), new RGB(red,
						green, blue));
			}
			return darkerBackground;
		}

		/*
		 * (non-Javadoc)
		 * 
		 * @see
		 * org.eclipse.draw2d.Label#paintFigure(org.eclipse.draw2d.Graphics)
		 */
		public void paint(Graphics graphics) {

			graphics.setForegroundColor(getDarkerBackgroundColor());
			graphics.setBackgroundColor(getBackgroundColor());

			graphics.pushState();

			// fill in the background
			Rectangle bounds = getBounds().getCopy();
			Rectangle r = bounds.getCopy();
			r.y += arcWidth / 2;
			r.height -= arcWidth;

			Rectangle top = bounds.getCopy();
			top.height /= 2;
			graphics.setForegroundColor(getBackgroundColor());
			graphics.setBackgroundColor(getBackgroundColor());
			graphics.fillRoundRectangle(top, arcWidth, arcWidth);

			top.y = top.y + top.height;
			graphics.setForegroundColor(darkerBackground);
			graphics.setBackgroundColor(darkerBackground);
			graphics.fillRoundRectangle(top, arcWidth, arcWidth);

			graphics.setBackgroundColor(darkerBackground);
			graphics.setForegroundColor(getBackgroundColor());
			graphics.fillGradient(r, true);

			super.paint(graphics);
			graphics.popState();
			graphics.setForegroundColor(darkerBackground);
			graphics.setBackgroundColor(darkerBackground);
			// paint the border
			bounds.setSize(bounds.width - 1, bounds.height - 1);
			graphics.drawRoundRectangle(bounds, arcWidth, arcWidth);
		}

		public void setBackgroundColor(Color bg) {
			super.setBackgroundColor(bg);
			if (darkerBackground != null) {
				darkerBackground.dispose();
			}
			darkerBackground = null;
		}

		public void setTextT(String string) {
			this.setPreferredSize(null);
			this.label.setText(string);
			this.add(label);
			this.layout.layout(this);
			this.invalidate();
			this.revalidate();
			this.validate();
		}

		public void setText(String string) {
			this.label.setText(string);
		}

		public void setImage(Image image) {
			this.label.setIcon(image);
		}

		public void setFocus() {
			expander.requestFocus();
		}

	}

	static final double SCALED_WIDTH = 300;
	static final double SCALED_HEIGHT = 200;
	private static final int CONTAINER_HEIGHT = 200;
	private static final int MIN_WIDTH = 250;
	private static final int MIN_HEIGHT = 30;
	private static final int ANIMATION_TIME = 100;
	private static final int SUBLAYER_OFFSET = 2;

	private static SelectionListener selectionListener;

	private ExpandGraphLabel expandGraphLabel;

	private List childNodes = null;
	private int childAreaHeight = CONTAINER_HEIGHT;

	private ZestRootLayer zestLayer;
	private ScrollPane scrollPane;
	private LayoutAlgorithm layoutAlgorithm;
	private boolean isExpanded = false;
	private AspectRatioFreeformLayer scalledLayer;
	private InternalLayoutContext layoutContext;

	/**
	 * Creates a new GraphContainer. A GraphContainer may contain nodes, and has
	 * many of the same properties as a graph node.
	 * 
	 * @param graph
	 *            The graph that the container is being added to
	 * @param style
	 * 
	 * @since 2.0
	 */
	public GraphContainer(Graph graph, int style) {
		super(graph, style);
		initModel(graph, "", null);
		close(false);
		childNodes = new ArrayList();
		registerToParent(graph);
	}

	/**
	 * @deprecated Since Zest 2.0, use {@link #GraphContainer(Graph, int)},
	 *             {@link #setText(String)}, and {@link #setImage(Image)}
	 */
	public GraphContainer(Graph graph, int style, String text, Image image) {
		this(graph, style);
		setText(text);
		setImage(image);
	}

	/**
	 * @deprecated Since Zest 2.0, use {@link #GraphContainer(Graph, int)}.
	 */
	public GraphContainer(GraphContainer container, int style) {
		this(container.getGraph(), style);
	}

	/**
	 * Custom figures cannot be set on a GraphContainer.
	 */
	public void setCustomFigure(IFigure nodeFigure) {
		throw new RuntimeException(
				"Operation not supported:  Containers cannot have custom figures");
	}

	/**
	 * Close this node.
	 * 
	 * @param animate
	 */
	public void close(boolean animate) {
		if (animate) {
			Animation.markBegin();
		}
		isExpanded = false;

		expandGraphLabel.setExpandedState(false);
		Rectangle newBounds = scrollPane.getBounds().getCopy();
		newBounds.height = 0;

		scrollPane.setSize(scrollPane.getSize().width, 0);
		updateFigureForModel(this.zestLayer);
		scrollPane.setVisible(false);
		List children = this.zestLayer.getChildren();
		for (Iterator iterator = children.iterator(); iterator.hasNext();) {
			IFigure child = (IFigure) iterator.next();
			GraphItem item = getGraph().getGraphItem(child);
			item.setVisible(false);
		}
		Rectangle containerBounds = new Rectangle(this.getLocation(),
				new Dimension(this.getSize().width, CONTAINER_HEIGHT
						+ this.expandGraphLabel.getSize().height));
		moveNodesUp(containerBounds, this);
		if (animate) {
			Animation.run(ANIMATION_TIME);
		}
		updateFigureForModel(nodeFigure);
	}

	private static void addNodeToOrderedList(List orderedNodeList,
			GraphNode node) {
		Iterator orderedNodeIterator = orderedNodeList.iterator();
		int counter = 0;
		while (orderedNodeIterator.hasNext()) {
			// Look through the list of nodes below and find the right spot for
			// this
			GraphNode nextOrderedNode = (GraphNode) orderedNodeIterator.next();
			if (nextOrderedNode.getLocation().y
					+ nextOrderedNode.getBounds().height > node.getLocation().y
					+ node.getBounds().height) {
				break;
			}
			counter++;
		}
		// Place this in the right location
		orderedNodeList.add(counter, node);
	}

	/**
	 * Gets all the nodes below the yValue. The nodes are returned in order.
	 * 
	 * @param nodes
	 * @param yValue
	 * @return
	 */
	private static List getOrderedNodesBelowY(List nodes, int yValue,
			GraphNode yValueNode) {
		Iterator iterator = nodes.iterator();
		LinkedList orderedNode = new LinkedList();
		while (iterator.hasNext()) {
			GraphNode node = (GraphNode) iterator.next();
			if (node == yValueNode) {
				continue;
			}
			if (node.getLocation().y + node.getBounds().height > yValue) {
				// This node is below the container
				addNodeToOrderedList(orderedNode, node);
			}
		}
		// Convert this to an arrayList for faster access
		List arrayList = new ArrayList();
		iterator = orderedNode.iterator();
		while (iterator.hasNext()) {
			arrayList.add(iterator.next());
		}
		return arrayList;
	}

	/**
	 * Checks if the node intersects the stripe between left and right
	 * 
	 * @param left
	 * @param right
	 * @param node
	 * @return
	 */
	private static boolean nodeInStripe(int left, int right, GraphNode node) {
		return (node.getBounds().x < right && node.getBounds().x
				+ node.getBounds().width > left);
	}

	void pack(Graph g) {
		GraphNode highestNode = getHighestNode(g);
		moveNodesUp(highestNode.getBounds(), highestNode);
	}

	/**
	 * 
	 * @param g
	 * @return
	 */
	static GraphNode getHighestNode(Graph g) {
		Iterator iterator = g.getNodes().iterator();
		GraphNode lowest /* highest on the screen */= null;

		while (iterator.hasNext()) {
			GraphNode node = (GraphNode) iterator.next();
			if (lowest == null || lowest.getBounds().y > node.getBounds().y) {
				lowest = node;
			}
		}
		return lowest;

	}

	/**
	 * Move the nodes below this node up
	 * 
	 * @param containerBounds
	 * @param graphContainer
	 */
	private void moveNodesUp(Rectangle containerBounds, GraphNode graphContainer) {

		// Get all nodes below this container, in order
		List orderedNodesBelowY = getOrderedNodesBelowY(parent.getGraph()
				.getNodes(), containerBounds.y, graphContainer);
		int leftSide = containerBounds.x;
		int rightSide = containerBounds.x + containerBounds.width;
		List nodesToConsider = new LinkedList();
		for (int i = 0; i < orderedNodesBelowY.size(); i++) {
			nodesToConsider.add(orderedNodesBelowY.get(i));
		}
		addNodeToOrderedList(orderedNodesBelowY, graphContainer);

		while (nodesToConsider.size() > 0) {
			GraphNode node = (GraphNode) nodesToConsider.get(0);
			if (nodeInStripe(leftSide, rightSide, node)) {
				leftSide = Math.min(leftSide, node.getBounds().x);
				rightSide = Math.max(rightSide,
						node.getBounds().x + node.getBounds().width);
				// If this node is in the stripe, move it up
				// the previous node
				GraphNode previousNode = null;
				int i = 0;
				for (; i < orderedNodesBelowY.size(); i++) {
					if (orderedNodesBelowY.get(i) == node) {
						break;
					}
				}
				int j = i - 1;
				while (j >= 0) {
					GraphNode pastNode = (GraphNode) orderedNodesBelowY.get(j);
					// if (nodeInStripe(leftSide, rightSide, pastNode)) {
					if (nodeInStripe(node.getBounds().x, node.getBounds().x
							+ node.getBounds().width, pastNode)) {
						previousNode = pastNode;
						break;
					}
					j--;
				}
				if (previousNode == null) {
					previousNode = graphContainer;
				}
				int previousLocation = previousNode.getBounds().y
						+ previousNode.getBounds().height + 2;

				orderedNodesBelowY.remove(i);
				node.setLocation(node.getLocation().x, previousLocation);
				addNodeToOrderedList(orderedNodesBelowY, node);

			}
			nodesToConsider.remove(node);
		}
	}

	/**
	 * Open the container. This opens the graph container to show the nodes
	 * within and update the twistie
	 */
	public void open(boolean animate) {
		if (animate) {
			Animation.markBegin();
		}
		isExpanded = true;

		expandGraphLabel.setExpandedState(true);

		scrollPane.setSize(computeChildArea());
		scrollPane.setVisible(true);

		List children = this.zestLayer.getChildren();
		for (Iterator iterator = children.iterator(); iterator.hasNext();) {
			IFigure child = (IFigure) iterator.next();
			GraphItem item = getGraph().getGraphItem(child);
			item.setVisible(true);
		}

		updateFigureForModel(nodeFigure);

		Rectangle containerBounds = new Rectangle(this.getLocation(),
				new Dimension(this.getSize().width, CONTAINER_HEIGHT
						+ this.expandGraphLabel.getSize().height));
		moveNodesDown(containerBounds, this);
		moveNodesUp(containerBounds, this);
		if (animate) {
			Animation.run(ANIMATION_TIME);
		}
		this.getFigure().getUpdateManager().performValidation();
	}

	/**
	 * 
	 * @param containerBounds
	 * @param graphContainer
	 */
	private void moveNodesDown(Rectangle containerBounds,
			GraphContainer graphContainer) {

		// Find all nodes below here
		List nodesBelowHere = getOrderedNodesBelowY(parent.getGraph()
				.getNodes(), containerBounds.y, graphContainer);
		Iterator nodesBelowHereIterator = nodesBelowHere.iterator();
		List nodesToMove = new LinkedList();
		int left = containerBounds.x;
		int right = containerBounds.x + containerBounds.width;
		while (nodesBelowHereIterator.hasNext()) {
			GraphNode node = (GraphNode) nodesBelowHereIterator.next();
			if (nodeInStripe(left, right, node)) {
				nodesToMove.add(node);
				left = Math.min(left, node.getBounds().x);
				right = Math.max(right, node.getBounds().x
						+ node.getBounds().width);
			}
		}
		List intersectingNodes = intersectingNodes(containerBounds,
				nodesToMove, graphContainer);
		int delta = getMaxMovement(containerBounds, intersectingNodes);
		if (delta > 0) {
			shiftNodesDown(nodesToMove, delta);
		}

	}

	/**
	 * Checks all the nodes in the list of nodesToCheck to see if they intersect
	 * with the bounds set
	 * 
	 * @param node
	 * @param nodesToCheck
	 * @return
	 */
	private List intersectingNodes(Rectangle bounds, List nodesToCheck,
			GraphNode node) {
		List result = new LinkedList();
		Iterator nodes = nodesToCheck.iterator();
		while (nodes.hasNext()) {
			GraphNode nodeToCheck = (GraphNode) nodes.next();
			if (node == nodeToCheck) {
				continue;
			}
			if (bounds.intersects(nodeToCheck.getBounds())) {
				result.add(nodeToCheck);
			}
		}
		return result;
	}

	/**
	 * Gets the max distance the intersecting nodes need to be shifted to make
	 * room for the expanding node
	 * 
	 * @param bounds
	 * @param nodesToMove
	 * @return
	 */
	private int getMaxMovement(Rectangle bounds, List nodesToMove) {
		Iterator iterator = nodesToMove.iterator();
		int maxMovement = 0;
		while (iterator.hasNext()) {
			GraphNode node = (GraphNode) iterator.next();
			int yValue = node.getLocation().y;
			int distanceFromBottom = (bounds.y + bounds.height) - yValue;
			maxMovement = Math.max(maxMovement, distanceFromBottom);
		}
		return maxMovement + 3;
	}

	/**
	 * Shifts a collection of nodes down.
	 * 
	 * @param nodesToShift
	 * @param amount
	 */
	private void shiftNodesDown(List nodesToShift, int amount) {
		Iterator iterator = nodesToShift.iterator();
		while (iterator.hasNext()) {
			GraphNode node = (GraphNode) iterator.next();

			node.setLocation(node.getLocation().x, node.getLocation().y
					+ amount);
		}
	}

	/**
	 * Gets the graph that this container has been added to.
	 */
	public Graph getGraph() {
		return this.graph;
	}

	/**
	 * @since 2.0
	 */
	public Widget getItem() {
		return this;
	}

	public int getItemType() {
		return CONTAINER;
	}

	/**
	 * @since 2.0
	 */
	public void setLayoutAlgorithm(LayoutAlgorithm algorithm,
			boolean applyLayout) {
		this.layoutAlgorithm = algorithm;
		this.layoutAlgorithm.setLayoutContext(getLayoutContext());
		if (applyLayout) {
			applyLayout();
		}
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
	 * @since 2.0
	 */
	public DisplayIndependentRectangle getLayoutBounds() {
		double width = GraphContainer.SCALED_WIDTH - 10;
		double height = GraphContainer.SCALED_HEIGHT - 10;
		return new DisplayIndependentRectangle(25, 25, width - 50, height - 50);
	}

	public void applyLayout() {
		if (layoutAlgorithm == null) {
			setLayoutAlgorithm(new TreeLayoutAlgorithm(), false);
		}
		Animation.markBegin();
		layoutAlgorithm.applyLayout(true);
		layoutContext.flushChanges(false);
		Animation.run(ANIMATION_TIME);
		getFigure().getUpdateManager().performUpdate();
	}

	/**
	 * Get the scale for this container. This is the scale applied to the
	 * children contained within
	 * 
	 * @return
	 */
	public double getScale() {
		return this.scalledLayer.getScale();
	}

	/**
	 * Set the scale for this container. This is the scale applied to the
	 * children contained within.
	 * 
	 * @param scale
	 */
	public void setScale(double scale) {
		this.scalledLayer.setScale(scale);
	}

	/***************************************************************************
	 * NON API MEMBERS
	 **************************************************************************/
	protected void initFigure() {
		nodeFigure = createContainerFigure();
	}

	/**
	 * This is a small class to help represent the size of the container. It
	 * should only be used in the computeContainerSize method.
	 */
	class ContainerDimension {
		int width;
		int labelHeight;
		int expandedHeight;
	}

	/**
	 * Computes size of the scroll pane that the child nodes will be placed in.
	 * 
	 * @return
	 */
	private Dimension computeChildArea() {
		ContainerDimension containerDimension = computeContainerSize();
		Dimension dimension = new Dimension();
		dimension.width = containerDimension.width;
		dimension.height = containerDimension.expandedHeight
				- containerDimension.labelHeight + SUBLAYER_OFFSET;
		return dimension;
	}

	/**
	 * Computes the desired size of the container. This method uses the minimum
	 * size, label size and setSize to compute the size.
	 * 
	 * @return
	 */
	private ContainerDimension computeContainerSize() {
		ContainerDimension dimension = new ContainerDimension();
		int labelHeight = expandGraphLabel.getPreferredSize().height;
		int labelWidth = expandGraphLabel.getPreferredSize().width;
		if (labelWidth < MIN_WIDTH) {
			labelWidth = MIN_WIDTH;
			expandGraphLabel.setPreferredSize(labelWidth, labelHeight);
		}

		dimension.labelHeight = Math.max(labelHeight, MIN_HEIGHT);
		dimension.width = Math.max(labelWidth, this.size.width);
		dimension.expandedHeight = Math.max(dimension.labelHeight
				+ childAreaHeight - SUBLAYER_OFFSET, this.size.height);

		return dimension;
	}

	private double computeHeightScale() {
		Dimension childArea = computeChildArea();
		double heightScale = childArea.height / SCALED_HEIGHT;
		return heightScale;
	}

	private double computeWidthScale() {
		Dimension childArea = computeChildArea();
		double widthScale = childArea.width / SCALED_WIDTH;
		return widthScale;
	}

	private IFigure createContainerFigure() {
		GraphContainer node = this;
		IFigure containerFigure = new ContainerFigure();
		containerFigure.setOpaque(true);

		containerFigure.addLayoutListener(LayoutAnimator.getDefault());

		containerFigure.setLayoutManager(new FreeformLayout());
		expandGraphLabel = new ExpandGraphLabel(this, node.getText(),
				node.getImage(), false);
		expandGraphLabel.setText(getText());
		expandGraphLabel.setImage(getImage());
		ContainerDimension containerDimension = computeContainerSize();

		scrollPane = new ScrollPane();
		scrollPane.addLayoutListener(LayoutAnimator.getDefault());

		Viewport viewport = new FreeformViewport();

		scrollPane.setViewport(viewport);
		viewport.addLayoutListener(LayoutAnimator.getDefault());
		scrollPane.setScrollBarVisibility(ScrollPane.AUTOMATIC);

		scalledLayer = new AspectRatioFreeformLayer("debug label");
		scalledLayer.addLayoutListener(LayoutAnimator.getDefault());
		scalledLayer.setScale(computeWidthScale(), computeHeightScale());
		zestLayer = new ZestRootLayer();
		zestLayer.addLayoutListener(LayoutAnimator.getDefault());
		scalledLayer.add(zestLayer);

		zestLayer.setLayoutManager(new FreeformLayout());
		scrollPane.setSize(computeChildArea());
		scrollPane.setLocation(new Point(0, containerDimension.labelHeight
				- SUBLAYER_OFFSET));
		scrollPane.setForegroundColor(ColorConstants.gray);

		expandGraphLabel.setBackgroundColor(getBackgroundColor());
		expandGraphLabel.setForegroundColor(getForegroundColor());
		expandGraphLabel.setLocation(new Point(0, 0));

		containerFigure.add(scrollPane);
		containerFigure.add(expandGraphLabel);

		scrollPane.getViewport().setContents(scalledLayer);
		scrollPane.setBorder(new LineBorder());

		return containerFigure;
	}

	private void registerToParent(IContainer parent) {
		if (parent.getItemType() == GRAPH) {
			createSelectionListener();
			parent.getGraph().addSelectionListener(selectionListener);
		}
	}

	private void createSelectionListener() {
		if (selectionListener == null) {
			selectionListener = new SelectionListener() {
				/**
				 * 
				 */
				private static final long serialVersionUID = 1L;

				public void widgetSelected(SelectionEvent e) {
					if (e.item instanceof GraphContainer) {
						// set focus to expand label so that pressing space
						// opens/closes
						// the last selected container
						((GraphContainer) e.item).expandGraphLabel.setFocus();
					}
				}

				public void widgetDefaultSelected(SelectionEvent e) {
					// ignore
				}
			};

		}
	}

	protected void updateFigureForModel(IFigure currentFigure) {

		if (expandGraphLabel == null) {
			initFigure();
		}
		expandGraphLabel.setTextT(getText());
		expandGraphLabel.setImage(getImage());
		expandGraphLabel.setFont(getFont());

		if (highlighted == HIGHLIGHT_ON) {
			expandGraphLabel.setForegroundColor(getForegroundColor());
			expandGraphLabel.setBackgroundColor(getHighlightColor());
		} else {
			expandGraphLabel.setForegroundColor(getForegroundColor());
			expandGraphLabel.setBackgroundColor(getBackgroundColor());
		}

		ContainerDimension containerDimension = computeContainerSize();

		expandGraphLabel.setSize(containerDimension.width,
				containerDimension.labelHeight);
		if (isExpanded) {
			setSize(containerDimension.width, containerDimension.expandedHeight);
		} else {
			setSize(containerDimension.width, containerDimension.labelHeight);
		}
		scrollPane.setLocation(new Point(expandGraphLabel.getLocation().x,
				expandGraphLabel.getLocation().y
						+ containerDimension.labelHeight - SUBLAYER_OFFSET));

	}

	void refreshBounds() {
		if (nodeFigure == null || nodeFigure.getParent() == null) {
			return; // node figure has not been created yet
		}
		GraphNode node = this;
		Point loc = node.getLocation();

		ContainerDimension containerDimension = computeContainerSize();
		Dimension size = new Dimension();

		expandGraphLabel.setSize(containerDimension.width,
				containerDimension.labelHeight);
		this.childAreaHeight = computeChildArea().height;
		if (isExpanded) {
			size.width = containerDimension.width;
			size.height = containerDimension.expandedHeight;
		} else {
			size.width = containerDimension.width;
			size.height = containerDimension.labelHeight;
		}
		Rectangle bounds = new Rectangle(loc, size);
		nodeFigure.getParent().setConstraint(nodeFigure, bounds);
		scrollPane.setLocation(new Point(expandGraphLabel.getLocation().x,
				expandGraphLabel.getLocation().y
						+ containerDimension.labelHeight - SUBLAYER_OFFSET));
		scrollPane.setSize(computeChildArea());
		scalledLayer.setScale(computeWidthScale(), computeHeightScale());
	}

	/**
	 * @noreference This method is not intended to be referenced by clients.
	 */
	public void addSubgraphFigure(IFigure figure) {
		zestLayer.addSubgraph(figure);
		graph.subgraphFigures.add(figure);
	}

	void addConnectionFigure(IFigure figure) {
		nodeFigure.add(figure);
	}

	/**
	 * @noreference This method is not intended to be referenced by clients.
	 */
	public void addNode(GraphNode node) {
		zestLayer.addNode(node.getNodeFigure());
		this.childNodes.add(node);
		node.setVisible(isExpanded);
	}

	public List getNodes() {
		return this.childNodes;
	}

	/**
	 * @since 2.0
	 */
	public List getConnections() {
		return filterConnections(getGraph().getConnections());

	}

	private List filterConnections(List connections) {
		List result = new ArrayList();
		for (Iterator iterator = connections.iterator(); iterator.hasNext();) {
			GraphConnection connection = (GraphConnection) iterator.next();
			if (connection.getSource().getParent() == this
					&& connection.getDestination().getParent() == this) {
				result.add(connection);
			}
		}
		return result;
	}

	/**
	 * @since 2.0
	 */
	public LayoutAlgorithm getLayoutAlgorithm() {
		return layoutAlgorithm;
	}
}
