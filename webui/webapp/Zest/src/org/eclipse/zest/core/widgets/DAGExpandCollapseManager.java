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

import java.util.HashSet;
import java.util.Iterator;

import org.eclipse.zest.layouts.interfaces.ConnectionLayout;
import org.eclipse.zest.layouts.interfaces.ContextListener;
import org.eclipse.zest.layouts.interfaces.ExpandCollapseManager;
import org.eclipse.zest.layouts.interfaces.GraphStructureListener;
import org.eclipse.zest.layouts.interfaces.LayoutContext;
import org.eclipse.zest.layouts.interfaces.NodeLayout;

/**
 * <p>
 * An {@link ExpandCollapseManager} specialized for Directed Acyclic Graphs. It
 * works correctly only when all connections are directed (and of course nodes
 * form an acyclic graph). It's supposed to be used with
 * {@link InternalLayoutContext}.
 * </p>
 * <p>
 * When a node is collapsed, all its outgoing connections are hidden and these
 * successors that have no visible incoming nodes are pruned. When a node is
 * expanded, all its successors are unpruned and connections pointing to them
 * are shown.
 * </p>
 * </p>
 * <p>
 * <b>NOTE:</b> A <code>Graph</code> using this manager should use
 * {@link DefaultSubgraph}, which doesn't show any information about subgraphs
 * in the graph. That's because for this manager it doesn't matter which
 * subgraph a node belongs to (each pruning creates a new subgraph). Also, this
 * manager adds a label to each collapsed node showing number of its successors.
 * </p>
 * One instance of this class can serve only one instance of <code>Graph</code>.
 * 
 * @since 2.0
 */
@SuppressWarnings({ "rawtypes", "unchecked" })
public class DAGExpandCollapseManager implements ExpandCollapseManager {

	private InternalLayoutContext context;

	private HashSet expandedNodes = new HashSet();

	private HashSet nodesToPrune = new HashSet();

	private HashSet nodesToUnprune = new HashSet();

	private HashSet nodesToUpdate = new HashSet();

	private boolean cleanLayoutScheduled = false;

	public void initExpansion(final LayoutContext context2) {
		if (!(context2 instanceof InternalLayoutContext)) {
			throw new RuntimeException(
					"This manager works only with org.eclipse.zest.core.widgets.InternalLayoutContext");
		}
		context = (InternalLayoutContext) context2;

		context.addGraphStructureListener(new GraphStructureListener() {
			public boolean nodeRemoved(LayoutContext context, NodeLayout node) {
				if (isExpanded(node)) {
					collapse(node);
				}
				flushChanges(false, true);
				return false;
			}

			public boolean nodeAdded(LayoutContext context, NodeLayout node) {
				resetState(node);
				flushChanges(false, true);
				return false;
			}

			public boolean connectionRemoved(LayoutContext context,
					ConnectionLayout connection) {
				NodeLayout target = connection.getTarget();
				if (!isExpanded(target)
						&& target.getIncomingConnections().length == 0) {
					expand(target);
				}
				flushChanges(false, true);
				return false;
			}

			public boolean connectionAdded(LayoutContext context,
					ConnectionLayout connection) {
				resetState(connection.getTarget());
				updateNodeLabel(connection.getSource());
				flushChanges(false, true);
				return false;
			}

		});

		context.addContextListener(new ContextListener.Stub() {
			public void backgroundEnableChanged(LayoutContext context) {
				flushChanges(false, false);
			}
		});
	}

	public boolean canCollapse(LayoutContext context, NodeLayout node) {
		return isExpanded(node) && !node.isPruned()
				&& node.getOutgoingConnections().length > 0;
	}

	public boolean canExpand(LayoutContext context, NodeLayout node) {
		return !isExpanded(node) && !node.isPruned()
				&& node.getOutgoingConnections().length > 0;
	}

	private void collapseAllConnections(NodeLayout node) {
		ConnectionLayout[] outgoingConnections = node.getOutgoingConnections();
		for (int i = 0; i < outgoingConnections.length; i++) {
			outgoingConnections[i].setVisible(false);
		}
		flushChanges(true, true);
	}

	private void expandAllConnections(NodeLayout node) {
		ConnectionLayout[] outgoingConnections = node.getOutgoingConnections();
		for (int i = 0; i < outgoingConnections.length; i++) {
			outgoingConnections[i].setVisible(true);
		}
		flushChanges(true, true);
	}

	public void setExpanded(LayoutContext context, NodeLayout node,
			boolean expanded) {

		// if (isExpanded(node) == expanded)
		// return;
		if (expanded) {
			if (canExpand(context, node)) {
				expand(node);
			}
			expandAllConnections(node);
		} else {
			if (canCollapse(context, node)) {
				collapse(node);
			}
			collapseAllConnections(node);
		}
		flushChanges(true, true);
	}

	private void expand(NodeLayout node) {
		setExpanded(node, true);
		NodeLayout[] successingNodes = node.getSuccessingNodes();
		for (int i = 0; i < successingNodes.length; i++) {
			unpruneNode(successingNodes[i]);
		}
		updateNodeLabel(node);
	}

	private void collapse(NodeLayout node) {
		if (isExpanded(node)) {
			setExpanded(node, false);
		} else {
			return;
		}
		NodeLayout[] successors = node.getSuccessingNodes();
		for (int i = 0; i < successors.length; i++) {
			checkPruning(successors[i]);
			if (isPruned(successors[i])) {
				collapse(successors[i]);
			}
		}
		updateNodeLabel(node);
	}

	private void checkPruning(NodeLayout node) {
		boolean prune = true;
		NodeLayout[] predecessors = node.getPredecessingNodes();
		for (int j = 0; j < predecessors.length; j++) {
			if (isExpanded(predecessors[j])) {
				prune = false;
				break;
			}
		}
		if (prune) {
			pruneNode(node);
		} else {
			unpruneNode(node);
		}
	}

	/**
	 * By default nodes at the top (having no predecessors) are expanded. The
	 * rest are collapsed and pruned if they don't have any expanded
	 * predecessors
	 * 
	 * @param target
	 */
	private void resetState(NodeLayout node) {
		NodeLayout[] predecessors = node.getPredecessingNodes();
		if (predecessors.length == 0) {
			expand(node);
		} else {
			collapse(node);
			checkPruning(node);
		}
	}

	/**
	 * If given node belongs to a layout context using
	 * {@link PrunedSuccessorsSubgraph}, update of the nodes's label is forced.
	 * 
	 * @param node
	 *            node to update
	 */
	private void updateNodeLabel(NodeLayout node) {
		nodesToUpdate.add(node);
	}

	private void updateNodeLabel2(InternalNodeLayout node) {
		SubgraphFactory subgraphFactory = node.getOwnerLayoutContext()
				.getSubgraphFactory();
		if (subgraphFactory instanceof DefaultSubgraph.PrunedSuccessorsSubgraphFactory) {
			((DefaultSubgraph.PrunedSuccessorsSubgraphFactory) subgraphFactory)
					.updateLabelForNode(node);
		}
	}

	private void pruneNode(NodeLayout node) {
		if (isPruned(node)) {
			return;
		}
		nodesToUnprune.remove(node);
		nodesToPrune.add(node);
	}

	private void unpruneNode(NodeLayout node) {
		if (!isPruned(node)) {
			return;
		}
		nodesToPrune.remove(node);
		nodesToUnprune.add(node);
	}

	private boolean isPruned(NodeLayout node) {
		if (nodesToUnprune.contains(node)) {
			return false;
		}
		if (nodesToPrune.contains(node)) {
			return true;
		}
		return node.isPruned();
	}

	private void flushChanges(boolean force, boolean clean) {
		cleanLayoutScheduled = cleanLayoutScheduled || clean;
		if (!force && !context.isBackgroundLayoutEnabled()) {
			return;
		}

		for (Iterator iterator = nodesToUnprune.iterator(); iterator.hasNext();) {
			NodeLayout node = (NodeLayout) iterator.next();
			node.prune(null);
		}
		nodesToUnprune.clear();

		if (!nodesToPrune.isEmpty()) {
			context.createSubgraph((NodeLayout[]) nodesToPrune
					.toArray(new NodeLayout[nodesToPrune.size()]));
			nodesToPrune.clear();
		}

		for (Iterator iterator = nodesToUpdate.iterator(); iterator.hasNext();) {
			InternalNodeLayout node = (InternalNodeLayout) iterator.next();
			updateNodeLabel2(node);
		}
		nodesToUpdate.clear();

		(context).applyLayout(cleanLayoutScheduled);
		cleanLayoutScheduled = false;
		context.flushChanges(true);
	}

	private boolean isExpanded(NodeLayout node) {
		return expandedNodes.contains(node);
	}

	private void setExpanded(NodeLayout node, boolean expanded) {
		if (expanded) {
			expandedNodes.add(node);
		} else {
			expandedNodes.remove(node);
		}
	}
}
