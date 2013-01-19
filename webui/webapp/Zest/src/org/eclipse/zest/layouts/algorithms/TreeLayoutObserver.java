/*******************************************************************************
 * Copyright (c) 2005-2010 The Chisel Group and others. All rights reserved. This
 * program and the accompanying materials are made available under the terms of
 * the Eclipse Public License v1.0 which accompanies this distribution, and is
 * available at http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors: The Chisel Group - initial API and implementation
 *               Mateusz Matela 
 *               Ian Bull
 ******************************************************************************/
package org.eclipse.zest.layouts.algorithms;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.ListIterator;
import java.util.Set;

import org.eclipse.zest.layouts.interfaces.ConnectionLayout;
import org.eclipse.zest.layouts.interfaces.GraphStructureListener;
import org.eclipse.zest.layouts.interfaces.LayoutContext;
import org.eclipse.zest.layouts.interfaces.NodeLayout;

/**
 * A helper class for layout algorithms that are based on tree structure. It
 * keeps track of changes in observed layout context and stores current
 * information about the tree structure - children of each node and several
 * other parameters.
 */
public class TreeLayoutObserver {

	/**
	 * <code>TreeLayoutObserver</code> uses instance of this class to create
	 * instances of {@link TreeNode}. It may be extended and passed to
	 * <code>TreeLayoutObserver</code>'s constructor in order to build a tree
	 * structure made of <code>TreeNode</code>'s subclasses.
	 */
	public static class TreeNodeFactory {
		public TreeNode createTreeNode(NodeLayout nodeLayout,
				TreeLayoutObserver observer) {
			return new TreeNode(nodeLayout, observer);
		}
	}

	/**
	 * Represents a node in a tree structure and stores all information related
	 * to it. May be subclassed if additional data and behavior is necessary.
	 */
	public static class TreeNode {
		final protected NodeLayout node;
		final protected TreeLayoutObserver owner;
		protected int height = 0;
		protected int depth = -1;
		protected int numOfLeaves = 0;
		protected int numOfDescendants = 0;
		protected int order = 0;
		protected final List children = new ArrayList();
		protected TreeNode parent;
		protected boolean firstChild = false, lastChild = false;

		/**
		 * 
		 * @return node layout related to this tree node (null for
		 *         {@link TreeLayoutObserver#getSuperRoot() Super Root})
		 */
		public NodeLayout getNode() {
			return node;
		}

		/**
		 * 
		 * @return <code>TreeLayoutObserver</code> owning this tree node
		 */
		public TreeLayoutObserver getOwner() {
			return owner;
		}

		/**
		 * 
		 * @return height of this node in the tree (the longest distance to a
		 *         leaf, 0 for a leaf itself)
		 */
		public int getHeight() {
			return height;
		}

		/**
		 * 
		 * @return depth of this node in the tree (distance from root, 0 for a
		 *         root and -1 for {@link TreeLayoutObserver#getSuperRoot()
		 *         Super Root}
		 */
		public int getDepth() {
			return depth;
		}

		/**
		 * 
		 * @return number of all leaves descending from this node (1 for a leaf
		 *         itself)
		 */
		public int getNumOfLeaves() {
			return numOfLeaves;
		}

		/**
		 * 
		 * @return total number of descendants of this node (0 for leafs)
		 */
		public int getNumOfDescendants() {
			return numOfDescendants;
		}

		/**
		 * Returns order in which nodes are visited during Deep First Search.
		 * Children are visited in the same order as they were added to their
		 * layout context, unless {@link TreeLayoutObserver#recomputeTree()} was
		 * called after the nodes were added. In that case the order is
		 * determined by order of nodes returned by
		 * {@link NodeLayout#getSuccessingNodes()}. Leaves are assigned
		 * successive numbers starting from 0, other nodes have order equal to
		 * the smallest order of their children.
		 * 
		 * @return order of this node
		 */
		public int getOrder() {
			return order;
		}

		/**
		 * 
		 * @return an unmodifiable list of this node's children
		 */
		public List getChildren() {
			return Collections.unmodifiableList(children);
		}

		/**
		 * 
		 * @return this node's parent
		 */
		public TreeNode getParent() {
			return parent;
		}

		/**
		 * 
		 * @return true if this node is the first child of its parent (has the
		 *         smallest order)
		 */
		public boolean isFirstChild() {
			return firstChild;
		}

		/**
		 * 
		 * @return
		 */
		public boolean isLastChild() {
			return lastChild;
		}

		/**
		 * Creates a tree node related to given layout node
		 * 
		 * @param node
		 *            the layout node
		 * @param owner
		 *            <code>TreeLayoutObserver</code> owning created node
		 */
		protected TreeNode(NodeLayout node, TreeLayoutObserver owner) {
			this.node = node;
			this.owner = owner;
		}

		/**
		 * Adds given node to the list of this node's children and set its
		 * parent to this node.
		 * 
		 * @param child
		 *            node to add
		 */
		protected void addChild(TreeNode child) {
			children.add(child);
			child.parent = this;
		}

		/**
		 * Performs a DFS on the tree structure and calculates all parameters of
		 * its nodes. Should be called on
		 * {@link TreeLayoutObserver#getSuperRoot() Super Root}. Uses recurrence
		 * to go through all the nodes.
		 */
		protected void precomputeTree() {
			if (children.isEmpty()) {
				height = 0;
				numOfLeaves = 1;
				numOfDescendants = 0;
			} else {
				height = 0;
				numOfLeaves = 0;
				numOfDescendants = 0;
				for (ListIterator iterator = children.listIterator(); iterator
						.hasNext();) {
					TreeNode child = (TreeNode) iterator.next();
					child.depth = this.depth + 1;
					child.order = this.order + this.numOfLeaves;
					child.precomputeTree();
					child.firstChild = (this.numOfLeaves == 0);
					child.lastChild = !iterator.hasNext();

					this.height = Math.max(this.height, child.height + 1);
					this.numOfLeaves += child.numOfLeaves;
					this.numOfDescendants += child.numOfDescendants + 1;
				}
			}
		}

		/**
		 * Finds a node that is the best parent for this node. Add this node as
		 * a child of the found node.
		 */
		protected void findNewParent() {
			if (parent != null)
				parent.children.remove(this);
			NodeLayout[] predecessingNodes = node.getPredecessingNodes();
			parent = null;
			for (int i = 0; i < predecessingNodes.length; i++) {
				TreeNode potentialParent = (TreeNode) owner.layoutToTree
						.get(predecessingNodes[i]);
				if (!children.contains(potentialParent)
						&& isBetterParent(potentialParent))
					parent = potentialParent;
			}
			if (parent == null)
				parent = owner.superRoot;

			parent.addChild(this);
		}

		/**
		 * Checks if a potential parent would be better for this node than its
		 * current parent. A better parent has smaller depth (with exception to
		 * {@link TreeLayoutObserver#getSuperRoot() Super Root}, which has depth
		 * equal to -1 but is never a better parent than any other node).
		 * 
		 * @param potentialParent
		 *            potential parent to check
		 * @return true if potentialParent can be a parent of this node and is
		 *         better than its current parent
		 */
		protected boolean isBetterParent(TreeNode potentialParent) {
			if (potentialParent == null)
				return false;
			if (this.parent == null && !this.isAncestorOf(potentialParent))
				return true;
			if (potentialParent.depth <= this.depth
					&& potentialParent.depth != -1)
				return true;
			if (this.parent != null && this.parent.depth == -1
					&& potentialParent.depth >= 0
					&& !this.isAncestorOf(potentialParent))
				return true;
			return false;
		}

		/**
		 * 
		 * @param descendant
		 * @return true if this node is an ancestor if given descendant node
		 */
		public boolean isAncestorOf(TreeNode descendant) {
			while (descendant.depth > this.depth)
				descendant = descendant.parent;
			return descendant == this;
		}
	}

	/**
	 * A superclass for listeners that can be added to this observer to get
	 * notification whenever the tree structure changes.
	 */
	public static class TreeListener {
		/**
		 * Called when new node is added to the tree structure. The new node
		 * will not have any connections, so it will be a child of
		 * {@link TreeLayoutObserver#getSuperRoot() Super Root}
		 * 
		 * @param newNode
		 *            the added node
		 */
		public void nodeAdded(TreeNode newNode) {
			defaultHandle(newNode);
		}

		/**
		 * Called when a node is removed from the tree structure. The given node
		 * no longer exists in the tree at the moment of call.
		 * 
		 * @param removedNode
		 *            the removed node
		 */
		public void nodeRemoved(TreeNode removedNode) {
			defaultHandle(removedNode);
		}

		/**
		 * Called when a node changes its parent.
		 * 
		 * @param node
		 *            node that changes its parent
		 * @param previousParent
		 *            previous parent of the node
		 */
		public void parentChanged(TreeNode node, TreeNode previousParent) {
			defaultHandle(node);
		}

		/**
		 * A convenience method that can be overridden if a listener reacts the
		 * same way to all events. By default it's called in every event handler
		 * and does nothing.
		 * 
		 * @param changedNode
		 *            the node that has changed
		 */
		protected void defaultHandle(TreeNode changedNode) {
		}
	}

	private GraphStructureListener structureListener = new GraphStructureListener() {

		public boolean nodeRemoved(LayoutContext context, NodeLayout node) {
			TreeNode treeNode = (TreeNode) layoutToTree.get(node);
			treeNode.parent.children.remove(treeNode);
			superRoot.precomputeTree();
			for (Iterator iterator = treeListeners.iterator(); iterator
					.hasNext();) {
				TreeListener listener = (TreeListener) iterator.next();
				listener.nodeRemoved(treeNode);
			}
			return false;
		}

		public boolean nodeAdded(LayoutContext context, NodeLayout node) {
			TreeNode treeNode = getTreeNode(node);
			superRoot.addChild(treeNode);
			superRoot.precomputeTree();
			for (Iterator iterator = treeListeners.iterator(); iterator
					.hasNext();) {
				TreeListener listener = (TreeListener) iterator.next();
				listener.nodeAdded(treeNode);
			}
			return false;
		}

		public boolean connectionRemoved(LayoutContext context,
				ConnectionLayout connection) {
			TreeNode node1 = (TreeNode) layoutToTree
					.get(connection.getSource());
			TreeNode node2 = (TreeNode) layoutToTree
					.get(connection.getTarget());
			if (node1.parent == node2) {
				node1.findNewParent();
				if (node1.parent != node2) {
					superRoot.precomputeTree();
					fireParentChanged(node1, node2);
				}
			}
			if (node2.parent == node1) {
				node2.findNewParent();
				if (node2.parent != node1) {
					superRoot.precomputeTree();
					fireParentChanged(node2, node1);
				}
			}
			return false;
		}

		public boolean connectionAdded(LayoutContext context,
				ConnectionLayout connection) {
			TreeNode source = (TreeNode) layoutToTree.get(connection
					.getSource());
			TreeNode target = (TreeNode) layoutToTree.get(connection
					.getTarget());
			if (source == target)
				return false;
			if (target.isBetterParent(source)) {
				TreeNode previousParent = target.parent;
				previousParent.children.remove(target);
				source.addChild(target);
				superRoot.precomputeTree();
				fireParentChanged(target, previousParent);
			}
			if (!connection.isDirected() && source.isBetterParent(target)) {
				TreeNode previousParent = source.parent;
				previousParent.children.remove(source);
				target.addChild(source);
				superRoot.precomputeTree();
				fireParentChanged(source, previousParent);
			}
			return false;
		}

		private void fireParentChanged(TreeNode node, TreeNode previousParent) {
			for (Iterator iterator = treeListeners.iterator(); iterator
					.hasNext();) {
				TreeListener listener = (TreeListener) iterator.next();
				listener.parentChanged(node, previousParent);
			}
		}
	};

	private final HashMap layoutToTree = new HashMap();
	private final TreeNodeFactory factory;
	private final LayoutContext context;
	private TreeNode superRoot;
	private ArrayList treeListeners = new ArrayList();

	/**
	 * Creates a
	 * 
	 * @param context
	 * @param nodeFactory
	 */
	public TreeLayoutObserver(LayoutContext context, TreeNodeFactory nodeFactory) {
		if (nodeFactory == null)
			this.factory = new TreeNodeFactory();
		else
			this.factory = nodeFactory;
		this.context = context;
		context.addGraphStructureListener(structureListener);
		recomputeTree();
	}

	/**
	 * Recomputes all the information about the tree structure (the same effect
	 * as creating new <code>TreeLayoutObserver</code>).
	 */
	public void recomputeTree() {
		superRoot = factory.createTreeNode(null, this);
		layoutToTree.put(null, superRoot);
		createTrees(context.getNodes());
	}

	/**
	 * Stops this observer from listening to changes in observed layout context.
	 * After calling this method the information about tree structure can be
	 * updated only when {@link #recomputeTree()} is called.
	 */
	public void stop() {
		context.removeGraphStructureListener(structureListener);
	}

	/**
	 * Returns Super Root, that is an artificial node being a common parent for
	 * all nodes in observed tree structure.
	 * 
	 * @return Super Root
	 */
	public TreeNode getSuperRoot() {
		return superRoot;
	}

	/**
	 * Returns a {@link TreeNode} related to given node layout. If such a
	 * <code>TreeNode</code> doesn't exist, it's created.
	 * 
	 * @param node
	 * @return
	 */
	public TreeNode getTreeNode(NodeLayout node) {
		TreeNode treeNode = (TreeNode) layoutToTree.get(node);
		if (treeNode == null) {
			treeNode = factory.createTreeNode(node, this);
			layoutToTree.put(node, treeNode);
		}
		return treeNode;
	}

	/**
	 * Adds a listener that will be informed about changes in tree structure.
	 * 
	 * @param listener
	 *            listener to add
	 */
	public void addTreeListener(TreeListener listener) {
		treeListeners.add(listener);
	}

	/**
	 * Removes a listener from list of listener to be informed about changes in
	 * tree structure.
	 * 
	 * @param listener
	 *            listener to remove
	 */
	public void removeTreeListener(TreeListener listener) {
		treeListeners.add(listener);
	}

	/**
	 * Builds a tree structure using BFS method. Created trees are children of
	 * {@link #superRoot}.
	 * 
	 * @param nodes
	 */
	private void createTrees(NodeLayout[] nodes) {
		HashSet alreadyVisited = new HashSet();
		LinkedList nodesToAdd = new LinkedList();
		for (int i = 0; i < nodes.length; i++) {
			NodeLayout root = findRoot(nodes[i], alreadyVisited);
			if (root != null) {
				alreadyVisited.add(root);
				nodesToAdd.addLast(new Object[] { root, superRoot });
			}
		}
		while (!nodesToAdd.isEmpty()) {
			Object[] dequeued = (Object[]) nodesToAdd.removeFirst();
			TreeNode currentNode = factory.createTreeNode(
					(NodeLayout) dequeued[0], this);
			layoutToTree.put(dequeued[0], currentNode);
			TreeNode currentRoot = (TreeNode) dequeued[1];

			currentRoot.addChild(currentNode);
			NodeLayout[] children = currentNode.node.getSuccessingNodes();
			for (int i = 0; i < children.length; i++) {
				if (!alreadyVisited.contains(children[i])) {
					alreadyVisited.add(children[i]);
					nodesToAdd
							.addLast(new Object[] { children[i], currentNode });
				}
			}
		}
		superRoot.precomputeTree();
	}

	/**
	 * Searches for a root of a tree containing given node by continuously
	 * grabbing a predecessor of current node. If it reaches an node that exists
	 * in alreadyVisited set, it returns null. If it detects a cycle, it returns
	 * the first found node of that cycle. If it reaches a node that has no
	 * predecessors, it returns that node.
	 * 
	 * @param nodeLayout
	 *            starting node
	 * @param alreadyVisited
	 *            set of nodes that can't lay on path to the root (if one does,
	 *            method stops and returns null).
	 * @return
	 */
	private NodeLayout findRoot(NodeLayout nodeLayout, Set alreadyVisited) {
		HashSet alreadyVisitedRoot = new HashSet();
		while (true) {
			if (alreadyVisited.contains(nodeLayout))
				return null;
			if (alreadyVisitedRoot.contains(nodeLayout))
				return nodeLayout;
			alreadyVisitedRoot.add(nodeLayout);
			NodeLayout[] predecessingNodes = nodeLayout.getPredecessingNodes();
			if (predecessingNodes.length > 0) {
				nodeLayout = predecessingNodes[0];
			} else {
				return nodeLayout;
			}
		}
	}
}
