/*******************************************************************************
 * Copyright (c) 2009-2010 Mateusz Matela and others. All rights reserved. This
 * program and the accompanying materials are made available under the terms of
 * the Eclipse Public License v1.0 which accompanies this distribution, and is
 * available at http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors: Mateusz Matela - initial API and implementation
 *               Ian Bull
 ******************************************************************************/
package org.eclipse.zest.core.widgets;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;

import org.eclipse.draw2d.FigureListener;
import org.eclipse.draw2d.IFigure;
import org.eclipse.draw2d.geometry.Dimension;
import org.eclipse.draw2d.geometry.Point;
import org.eclipse.swt.widgets.Item;
import org.eclipse.zest.layouts.dataStructures.DisplayIndependentDimension;
import org.eclipse.zest.layouts.dataStructures.DisplayIndependentPoint;
import org.eclipse.zest.layouts.interfaces.ConnectionLayout;
import org.eclipse.zest.layouts.interfaces.EntityLayout;
import org.eclipse.zest.layouts.interfaces.NodeLayout;
import org.eclipse.zest.layouts.interfaces.SubgraphLayout;

class InternalNodeLayout implements NodeLayout {

	/**
	 * This listener is added to nodes' figures as a workaround for the problem
	 * of minimized nodes leaving single on the graph pixels when zoomed out
	 */
	private final static FigureListener figureListener = new FigureListener() {
		public void figureMoved(IFigure source) {
			// hide figures of minimized nodes
			GraphNode node = (GraphNode) figureToNode.get(source);
			if (node.getLayout().isMinimized() && source.getSize().equals(0, 0)) {
				source.setVisible(false);
			} else {
				source.setVisible(node.isVisible());
			}
		}
	};
	private final static HashMap figureToNode = new HashMap();

	private DisplayIndependentPoint location;
	private DisplayIndependentDimension size;
	private boolean minimized = false;
	private final GraphNode node;
	private final InternalLayoutContext ownerLayoutContext;
	private DefaultSubgraph subgraph;
	private boolean isDisposed = false;

	public InternalNodeLayout(GraphNode graphNode) {
		this.node = graphNode;
		this.ownerLayoutContext = node.parent.getLayoutContext();
		graphNode.nodeFigure.addFigureListener(figureListener);
		figureToNode.put(graphNode.nodeFigure, graphNode);
	}

	public DisplayIndependentPoint getLocation() {
		if (location == null) {
			refreshLocation();
		}
		return new DisplayIndependentPoint(location);
	}

	public DisplayIndependentDimension getSize() {
		if (size == null) {
			refreshSize();
		}
		return new DisplayIndependentDimension(size);
	}

	public SubgraphLayout getSubgraph() {
		return subgraph;
	}

	public boolean isMovable() {
		return true;
	}

	public boolean isPrunable() {
		return ownerLayoutContext.isPruningEnabled();
	}

	public boolean isPruned() {
		return subgraph != null;
	}

	public boolean isResizable() {
		return (node.parent.getItem().getStyle() & ZestStyles.NODES_NO_LAYOUT_RESIZE) == 0;
	}

	public void prune(SubgraphLayout subgraph) {
		if (subgraph != null && !(subgraph instanceof DefaultSubgraph)) {
			throw new RuntimeException(
					"InternalNodeLayout can be pruned only to instance of DefaultSubgraph.");
		}
		ownerLayoutContext.checkChangesAllowed();
		if (subgraph == this.subgraph) {
			return;
		}
		if (this.subgraph != null) {
			SubgraphLayout subgraph2 = this.subgraph;
			this.subgraph = null;
			subgraph2.removeNodes(new NodeLayout[] { this });
		}
		if (subgraph != null) {
			this.subgraph = (DefaultSubgraph) subgraph;
			subgraph.addNodes(new NodeLayout[] { this });
		}
	}

	public void setLocation(double x, double y) {
		if (!ownerLayoutContext.isLayoutItemFiltered(this.getNode())) {
			ownerLayoutContext.checkChangesAllowed();
			internalSetLocation(x, y);
		}
	}

	private void internalSetLocation(double x, double y) {
		if (location != null) {
			location.x = x;
			location.y = y;
		} else {
			location = new DisplayIndependentPoint(x, y);
		}
	}

	public void setSize(double width, double height) {
		ownerLayoutContext.checkChangesAllowed();
		internalSetSize(width, height);
	}

	private void internalSetSize(double width, double height) {
		if (size != null) {
			size.width = width;
			size.height = height;
		} else {
			size = new DisplayIndependentDimension(width, height);
		}
	}

	public void setMinimized(boolean minimized) {
		ownerLayoutContext.checkChangesAllowed();
		getSize();
		this.minimized = minimized;
	}

	public boolean isMinimized() {
		return minimized;
	}

	public NodeLayout[] getPredecessingNodes() {
		ConnectionLayout[] connections = getIncomingConnections();
		NodeLayout[] result = new NodeLayout[connections.length];
		for (int i = 0; i < connections.length; i++) {
			result[i] = connections[i].getSource();
			if (result[i] == this) {
				result[i] = connections[i].getTarget();
			}
		}
		return result;
	}

	public NodeLayout[] getSuccessingNodes() {
		ConnectionLayout[] connections = getOutgoingConnections();
		NodeLayout[] result = new NodeLayout[connections.length];
		for (int i = 0; i < connections.length; i++) {
			result[i] = connections[i].getTarget();
			if (result[i] == this) {
				result[i] = connections[i].getSource();
			}
		}
		return result;
	}

	public EntityLayout[] getSuccessingEntities() {
		if (isPruned()) {
			return new NodeLayout[0];
		}
		ArrayList result = new ArrayList();
		HashSet addedSubgraphs = new HashSet();
		NodeLayout[] successingNodes = getSuccessingNodes();
		for (int i = 0; i < successingNodes.length; i++) {
			if (!successingNodes[i].isPruned()) {
				result.add(successingNodes[i]);
			} else {
				SubgraphLayout successingSubgraph = successingNodes[i]
						.getSubgraph();
				if (successingSubgraph.isGraphEntity()
						&& !addedSubgraphs.contains(successingSubgraph)) {
					result.add(successingSubgraph);
					addedSubgraphs.add(successingSubgraph);
				}
			}
		}
		return (EntityLayout[]) result.toArray(new EntityLayout[result.size()]);
	}

	public EntityLayout[] getPredecessingEntities() {
		if (isPruned()) {
			return new NodeLayout[0];
		}
		ArrayList result = new ArrayList();
		HashSet addedSubgraphs = new HashSet();
		NodeLayout[] predecessingNodes = getPredecessingNodes();
		for (int i = 0; i < predecessingNodes.length; i++) {
			if (!predecessingNodes[i].isPruned()) {
				result.add(predecessingNodes[i]);
			} else {
				SubgraphLayout predecessingSubgraph = predecessingNodes[i]
						.getSubgraph();
				if (predecessingSubgraph.isGraphEntity()
						&& !addedSubgraphs.contains(predecessingSubgraph)) {
					result.add(predecessingSubgraph);
					addedSubgraphs.add(predecessingSubgraph);
				}
			}
		}
		return (EntityLayout[]) result.toArray(new EntityLayout[result.size()]);
	}

	public ConnectionLayout[] getIncomingConnections() {
		ArrayList result = new ArrayList();
		for (Iterator iterator = node.getTargetConnections().iterator(); iterator
				.hasNext();) {
			GraphConnection connection = (GraphConnection) iterator.next();
			if (!ownerLayoutContext.isLayoutItemFiltered(connection)) {
				result.add(connection.getLayout());
			}
		}
		for (Iterator iterator = node.getSourceConnections().iterator(); iterator
				.hasNext();) {
			GraphConnection connection = (GraphConnection) iterator.next();
			if (!connection.isDirected()
					&& !ownerLayoutContext.isLayoutItemFiltered(connection)) {
				result.add(connection.getLayout());
			}
		}
		return (ConnectionLayout[]) result.toArray(new ConnectionLayout[result
				.size()]);
	}

	public ConnectionLayout[] getOutgoingConnections() {
		ArrayList result = new ArrayList();
		for (Iterator iterator = node.getSourceConnections().iterator(); iterator
				.hasNext();) {
			GraphConnection connection = (GraphConnection) iterator.next();
			if (!ownerLayoutContext.isLayoutItemFiltered(connection)) {
				result.add(connection.getLayout());
			}
		}
		for (Iterator iterator = node.getTargetConnections().iterator(); iterator
				.hasNext();) {
			GraphConnection connection = (GraphConnection) iterator.next();
			if (!connection.isDirected()
					&& !ownerLayoutContext.isLayoutItemFiltered(connection)) {
				result.add(connection.getLayout());
			}
		}
		return (ConnectionLayout[]) result.toArray(new ConnectionLayout[result
				.size()]);
	}

	public double getPreferredAspectRatio() {
		return 0;
	}

	GraphNode getNode() {
		return node;
	}

	public Item[] getItems() {
		return new GraphNode[] { node };
	}

	void applyLayout() {
		if (minimized) {
			node.setSize(0, 0);
			if (location != null) {
				node.setLocation(location.x, location.y);
			}
		} else {
			node.setSize(-1, -1);
			if (location != null) {
				node.setLocation(location.x - getSize().width / 2, location.y
						- size.height / 2);
			}
			if (size != null) {
				Dimension currentSize = node.getSize();
				if (size.width != currentSize.width
						|| size.height != currentSize.height) {
					node.setSize(size.width, size.height);
				}
			}
		}
	}

	InternalLayoutContext getOwnerLayoutContext() {
		return ownerLayoutContext;
	}

	void refreshSize() {
		Dimension size2 = node.getSize();
		internalSetSize(size2.width, size2.height);
	}

	void refreshLocation() {
		Point location2 = node.getLocation();
		internalSetLocation(location2.x + getSize().width / 2, location2.y
				+ size.height / 2);
	}

	public String toString() {
		return node.toString() + "(layout)";
	}

	void dispose() {
		isDisposed = true;
		if (subgraph != null) {
			subgraph.removeDisposedNodes();
		}
		ownerLayoutContext.fireNodeRemovedEvent(node.getLayout());
		figureToNode.remove(node.nodeFigure);
	}

	boolean isDisposed() {
		return isDisposed;
	}
}