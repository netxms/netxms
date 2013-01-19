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

import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Set;

import org.eclipse.draw2d.ColorConstants;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.widgets.Item;
import org.eclipse.zest.core.widgets.custom.LabelSubgraph;
import org.eclipse.zest.core.widgets.custom.TriangleSubgraph;
import org.eclipse.zest.core.widgets.custom.TriangleSubgraph.TriangleParameters;
import org.eclipse.zest.layouts.dataStructures.DisplayIndependentDimension;
import org.eclipse.zest.layouts.dataStructures.DisplayIndependentPoint;
import org.eclipse.zest.layouts.dataStructures.DisplayIndependentRectangle;
import org.eclipse.zest.layouts.interfaces.ConnectionLayout;
import org.eclipse.zest.layouts.interfaces.EntityLayout;
import org.eclipse.zest.layouts.interfaces.LayoutContext;
import org.eclipse.zest.layouts.interfaces.NodeLayout;
import org.eclipse.zest.layouts.interfaces.SubgraphLayout;

/**
 * Default implementation of {@link SubgraphLayout}. Every subgraph added to
 * Zest {@link Graph} should inherit from this class. The default implementation
 * is very simple. A node pruned to this subgraph is minimized and all
 * connections adjacent to it are made invisible. No additional graphic elements
 * are added to the graph, but subclasses may add them.
 * 
 * @since 2.0
 */
public class DefaultSubgraph implements SubgraphLayout {

	/**
	 * Default factory for {@link DefaultSubgraph}. It creates one subgraph for
	 * a whole graph and throws every node intimageo it.
	 */
	public static class DefaultSubgraphFactory implements SubgraphFactory {
		private HashMap contextToSubgraph = new HashMap();

		public SubgraphLayout createSubgraph(NodeLayout[] nodes,
				LayoutContext context) {
			DefaultSubgraph subgraph = (DefaultSubgraph) contextToSubgraph
					.get(context);
			if (subgraph == null) {
				subgraph = new DefaultSubgraph(context);
				contextToSubgraph.put(context, subgraph);
			}
			subgraph.addNodes(nodes);
			return subgraph;
		}
	};

	public static class LabelSubgraphFactory implements SubgraphFactory {
		private Color defaultForegroundColor = ColorConstants.black;
		private Color defaultBackgroundColor = ColorConstants.yellow;

		/**
		 * Changes the default foreground color for newly created subgraphs.
		 * 
		 * @param c
		 *            color to use
		 */
		public void setDefualtForegroundColor(Color c) {
			defaultForegroundColor = c;
		}

		/**
		 * Changes the default background color for newly created subgraphs.
		 * 
		 * @param c
		 *            color to use
		 */
		public void setDefaultBackgroundColor(Color c) {
			defaultBackgroundColor = c;
		}

		public SubgraphLayout createSubgraph(NodeLayout[] nodes,
				LayoutContext context) {
			return new LabelSubgraph(nodes, context, defaultForegroundColor,
					defaultBackgroundColor);
		}
	};

	public static class TriangleSubgraphFactory implements SubgraphFactory {
		private TriangleParameters parameters = new TriangleParameters();

		public SubgraphLayout createSubgraph(NodeLayout[] nodes,
				LayoutContext context) {
			return new TriangleSubgraph(nodes, context,
					(TriangleParameters) parameters.clone());
		}

		/**
		 * 
		 * @return initial color of triangles created with this factory
		 */
		public Color getColor() {
			return parameters.color;
		}

		/**
		 * Changes the default color for newly created subgraphs.
		 * 
		 * @param color
		 *            color to use
		 */
		public void setColor(Color color) {
			parameters.color = color;
		}

		/**
		 * 
		 * @return initial direction of triangles created with this factory
		 */
		public int getDirection() {
			return parameters.direction;
		}

		/**
		 * Changes the default direction for newly cretaed subgraphs.
		 * 
		 * @param direction
		 *            direction to use, can be {@link SubgraphLayout#TOP_DOWN},
		 *            {@link SubgraphLayout#BOTTOM_UP},
		 *            {@link SubgraphLayout#LEFT_RIGHT}, or
		 *            {@link SubgraphLayout#RIGHT_LEFT}
		 */
		public void setDirection(int direction) {
			parameters.direction = direction;
		}

		/**
		 * 
		 * @return maximum height of triangles created with this factory
		 */
		public double getReferenceHeight() {
			return parameters.referenceHeight;
		}

		/**
		 * Sets the maximum height for the triangle visualizing this subgraph.
		 * 
		 * @param referenceHeight
		 *            height to use
		 */
		public void setReferenceHeight(double referenceHeight) {
			parameters.referenceHeight = referenceHeight;
		}

		/**
		 * 
		 * @return maximum base length of triangles created with this factory
		 */
		public double getReferenceBase() {
			return parameters.referenceBase;
		}

		/**
		 * Sets the maximum base length for the triangle visualizing this
		 * subgraph.
		 * 
		 * @param referenceBase
		 *            base length to use
		 */

		public void setReferenceBase(double referenceBase) {
			parameters.referenceBase = referenceBase;
		}
	};

	/**
	 * Factory for {@link PrunedSuccessorsSubgraph}. It creates one subgraph for
	 * a whole graph and throws every node into it.
	 */
	public static class PrunedSuccessorsSubgraphFactory implements
			SubgraphFactory {
		private HashMap contextToSubgraph = new HashMap();

		public SubgraphLayout createSubgraph(NodeLayout[] nodes,
				LayoutContext context) {
			PrunedSuccessorsSubgraph subgraph = (PrunedSuccessorsSubgraph) contextToSubgraph
					.get(context);
			if (subgraph == null) {
				subgraph = new PrunedSuccessorsSubgraph(context);
				contextToSubgraph.put(context, subgraph);
			}
			subgraph.addNodes(nodes);
			return subgraph;
		}

		/**
		 * Updates a label for given node (creates the label if necessary).
		 * 
		 * @param node
		 *            node to update
		 */
		public void updateLabelForNode(InternalNodeLayout node) {
			InternalLayoutContext context = node.getOwnerLayoutContext();
			PrunedSuccessorsSubgraph subgraph = (PrunedSuccessorsSubgraph) contextToSubgraph
					.get(context);
			if (subgraph == null) {
				subgraph = new PrunedSuccessorsSubgraph(context);
				contextToSubgraph.put(context, subgraph);
			}
			subgraph.updateNodeLabel(node);
		}

	};

	protected final InternalLayoutContext context;

	protected final Set nodes = new HashSet();

	protected boolean disposed = false;

	protected DefaultSubgraph(LayoutContext context2) {
		if (context2 instanceof InternalLayoutContext) {
			this.context = (InternalLayoutContext) context2;
		} else {
			throw new RuntimeException(
					"This subgraph can be only created with LayoutContext provided by Zest Graph");
		}
	}

	public boolean isGraphEntity() {
		return false;
	}

	public void setSize(double width, double height) {
		// do nothing
		context.checkChangesAllowed();
	}

	public void setLocation(double x, double y) {
		// do nothing
		context.checkChangesAllowed();
	}

	public boolean isResizable() {
		return false;
	}

	public boolean isMovable() {
		return false;
	}

	public EntityLayout[] getSuccessingEntities() {
		return new EntityLayout[0];
	}

	public DisplayIndependentDimension getSize() {
		DisplayIndependentRectangle bounds = context.getBounds();
		return new DisplayIndependentDimension(bounds.width, bounds.height);
	}

	public double getPreferredAspectRatio() {
		return 0;
	}

	public EntityLayout[] getPredecessingEntities() {
		return new EntityLayout[0];
	}

	public DisplayIndependentPoint getLocation() {
		DisplayIndependentRectangle bounds = context.getBounds();
		return new DisplayIndependentPoint(bounds.x + bounds.width / 2,
				bounds.y + bounds.height / 2);
	}

	public boolean isDirectionDependant() {
		return false;
	}

	public void setDirection(int direction) {
		context.checkChangesAllowed();
		// do nothing
	}

	public void removeNodes(NodeLayout[] nodes) {
		context.checkChangesAllowed();
		for (int i = 0; i < nodes.length; i++) {
			if (this.nodes.remove(nodes[i])) {
				nodes[i].prune(null);
				nodes[i].setMinimized(false);
				refreshConnectionsVisibility(nodes[i].getIncomingConnections());
				refreshConnectionsVisibility(nodes[i].getOutgoingConnections());
			}
		}
		if (this.nodes.isEmpty()) {
			dispose();
		}
	}

	public void removeDisposedNodes() {
		for (Iterator iterator = nodes.iterator(); iterator.hasNext();) {
			InternalNodeLayout node = (InternalNodeLayout) iterator.next();
			if (node.isDisposed()) {
				iterator.remove();
			}
		}
	}

	public NodeLayout[] getNodes() {
		InternalNodeLayout[] result = new InternalNodeLayout[nodes.size()];
		int i = 0;
		for (Iterator iterator = nodes.iterator(); iterator.hasNext();) {
			result[i] = (InternalNodeLayout) iterator.next();
			if (!context.isLayoutItemFiltered(result[i].getNode())) {
				i++;
			}
		}
		if (i == nodes.size()) {
			return result;
		}

		NodeLayout[] result2 = new NodeLayout[i];
		System.arraycopy(result, 0, result2, 0, i);
		return result2;
	}

	public Item[] getItems() {
		GraphNode[] result = new GraphNode[nodes.size()];
		int i = 0;
		for (Iterator iterator = nodes.iterator(); iterator.hasNext();) {
			InternalNodeLayout node = (InternalNodeLayout) iterator.next();
			// getItems always returns an array of size 1 in case of
			// InternalNodeLayout
			result[i] = (GraphNode) node.getItems()[0];
			if (!context.isLayoutItemFiltered(node.getNode())) {
				i++;
			}
		}
		if (i == nodes.size()) {
			return result;
		}

		GraphNode[] result2 = new GraphNode[i];
		System.arraycopy(result, 0, result2, 0, i);
		return result2;
	}

	public int countNodes() {
		return nodes.size();
	}

	public void addNodes(NodeLayout[] nodes) {
		context.checkChangesAllowed();
		for (int i = 0; i < nodes.length; i++) {
			if (this.nodes.add(nodes[i])) {
				nodes[i].prune(this);
				nodes[i].setMinimized(true);
				refreshConnectionsVisibility(nodes[i].getIncomingConnections());
				refreshConnectionsVisibility(nodes[i].getOutgoingConnections());
			}
		}
	}

	protected void refreshConnectionsVisibility(ConnectionLayout[] connections) {
		for (int i = 0; i < connections.length; i++) {
			connections[i].setVisible(!connections[i].getSource().isPruned()
					&& !connections[i].getTarget().isPruned());
		}
	}

	/**
	 * Makes sure that value returned by {@link #getLocation()} will be equal to
	 * current location of this subgraph.
	 */
	protected void refreshLocation() {
		// do nothing, to be reimplemented in subclasses
	}

	/**
	 * Makes sure that value returned by {@link #getSize()} will be equal to
	 * current size of this subgraph.
	 */
	protected void refreshSize() {
		// do nothing, to be reimplemented in subclasses
	}

	protected void applyLayoutChanges() {
		// do nothing
	}

	protected void dispose() {
		if (!disposed) {
			context.removeSubgrah(this);
			disposed = true;
		}
	}

};