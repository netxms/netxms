/*******************************************************************************
 * Copyright (c) 2012 Rene Kuhlemann.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *    Rene Kuhlemann - provided first version of code based on the initial paper
 *    		of Sugiyama et al. (http://dx.doi.org/10.1109/TSMC.1981.4308636),
 *          associated to bugzilla entry #384730  
 *******************************************************************************/
package org.eclipse.zest.layouts.algorithms;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.IdentityHashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;

import org.eclipse.draw2d.geometry.Dimension;
import org.eclipse.zest.layouts.LayoutAlgorithm;
import org.eclipse.zest.layouts.dataStructures.DisplayIndependentRectangle;
import org.eclipse.zest.layouts.interfaces.LayoutContext;
import org.eclipse.zest.layouts.interfaces.NodeLayout;

/**
 * The SugiyamaLayoutAlgorithm class implements an algorithm to arrange a
 * directed graph in a layered tree-like layout. The final presentation follows
 * five design principles for enhanced readability:
 * 
 * - Hierarchical layout of vertices - Least crossings of lines (edges) -
 * Straightness of lines when ever possible - Close layout of vertices connected
 * to each other, i.e. short paths - Balanced layout of lines coming into or
 * going from a vertex
 * 
 * For further information see http://dx.doi.org/10.1109/TSMC.1981.4308636
 * 
 * This layout algorithm works only with - directed graphs (
 * {@link ZestStyles.CONNECTIONS_DIRECTED}) - graphs without cycles (otherwise
 * an appropriate RuntimeException is thrown)
 * 
 * @version 1.2
 * @author Rene Kuhlemann
 */
public class SugiyamaLayoutAlgorithm implements LayoutAlgorithm {

	// Tree direction constants
	public final static int HORIZONTAL = 1;
	public final static int VERTICAL = 2;

	// Internal constants
	private static final int MAX_LAYERS = 10;
	private static final int MAX_SWEEPS = 35;
	private static final int PADDING = -1;

	private class NodeWrapper {
		int index;
		final int layer;
		final NodeLayout node;
		final List<NodeWrapper> pred = new LinkedList<NodeWrapper>();
		final List<NodeWrapper> succ = new LinkedList<NodeWrapper>();

		NodeWrapper(NodeLayout n, int l) {
			node = n;
			layer = l;
		} // NodeLayout wrapper

		NodeWrapper(int l) {
			this(null, l);
		} // Dummy to connect two NodeLayout objects

		NodeWrapper() {
			this(null, PADDING);
		} // Padding for final refinement phase

		void addPredecessor(NodeWrapper node) {
			pred.add(node);
		}

		void addSuccessor(NodeWrapper node) {
			succ.add(node);
		}

		boolean isDummy() {
			return ((node == null) && (layer != PADDING));
		}

		boolean isPadding() {
			return ((node == null) && (layer == PADDING));
		}

		int getBaryCenter(List<NodeWrapper> list) {
			if (list.isEmpty())
				return (this.index);
			if (list.size() == 1)
				return (list.get(0).index);
			double barycenter = 0;
			for (NodeWrapper node : list)
				barycenter += node.index;
			return ((int) (barycenter / list.size())); // always rounding off to
														// avoid wrap around in
														// position refining!!!
		}

		int getPriorityDown() {
			if (isPadding())
				return (0);
			if (isDummy()) {
				if (succ.get(0).isDummy())
					return (Integer.MAX_VALUE); // part of a straight line
				else
					return (Integer.MAX_VALUE >> 1); // start of a straight line
			}
			return (pred.size());
		}

		int getPriorityUp() {
			if (isPadding())
				return (0);
			if (isDummy()) {
				if (pred.get(0).isDummy())
					return (Integer.MAX_VALUE); // part of a straight line
				else
					return (Integer.MAX_VALUE >> 1); // start of a straight line
			}
			return (succ.size());
		}

	}

	private final List<ArrayList<NodeWrapper>> layers = new ArrayList<ArrayList<NodeWrapper>>(
			MAX_LAYERS);
	private final Map<NodeLayout, NodeWrapper> map = new IdentityHashMap<NodeLayout, NodeWrapper>();
	private final int direction;
	private final Dimension dimension;

	private LayoutContext context;
	private int last; // index of the last element in a layer after padding
						// process

	/**
	 * Constructs a tree-like, layered layout of a directed graph.
	 * 
	 * @param dir
	 *            - {@link SugiyamaLayoutAlgorithm#HORIZONTAL}: left to right -
	 *            {@link SugiyamaLayoutAlgorithm#VERTCAL}: top to bottom
	 * 
	 * @param dim
	 *            - desired size of the layout area. Uses
	 *            {@link LayoutContext#getBounds()} if not set
	 */
	public SugiyamaLayoutAlgorithm(int dir, Dimension dim) {
		if (dir == HORIZONTAL)
			direction = HORIZONTAL;
		else
			direction = VERTICAL;
		dimension = dim;
	}

	public SugiyamaLayoutAlgorithm(int dir) {
		this(dir, null);
	}

	public SugiyamaLayoutAlgorithm() {
		this(VERTICAL, null);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.eclipse.zest.layouts.LayoutAlgorithm#setLayoutContext(org.eclipse
	 * .zest.layouts.interfaces.LayoutContext)
	 */
	public void setLayoutContext(LayoutContext context) {
		this.context = context;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.zest.layouts.LayoutAlgorithm#applyLayout(boolean)
	 */
	public void applyLayout(boolean clean) {
		if (!clean)
			return;
		layers.clear();
		map.clear();
		createLayers();
		padLayers();
		for (int i = 0; i < layers.size(); i++) { // reduce and refine
													// iteratively, depending on
													// the depth of the graph
			reduceCrossings();
			refineLayers();
		}
		reduceCrossings();
		calculatePositions();
	}

	private void createLayers() {
		List<NodeLayout> nodes = new LinkedList<NodeLayout>(
				Arrays.asList(context.getNodes()));
		List<NodeLayout> predecessors = findRoots(nodes);
		nodes.removeAll(predecessors); // nodes now contains only nodes that are
										// no roots
		addLayer(predecessors);
		for (int level = 1; nodes.isEmpty() == false; level++) {
			if (level > MAX_LAYERS)
				throw new RuntimeException(
						"Graphical tree exceeds maximum depth of " + MAX_LAYERS
								+ "! (Graph not directed? Cycles?)");
			List<NodeLayout> layer = new ArrayList<NodeLayout>();
			for (NodeLayout item : nodes) {
				if (predecessors.containsAll(Arrays.asList(item
						.getPredecessingNodes())))
					layer.add(item);
			}
			nodes.removeAll(layer);
			predecessors.addAll(layer);
			addLayer(layer);
		}
	}

	/**
	 * Wraps all {@link NodeLayout} objects into an internal presentation
	 * {@link NodeWrapper} and inserts dummy wrappers into the layers between an
	 * object and their predecessing nodes if necessary. Finally, all nodes are
	 * chained over immediate adjacent layers down to their predecessors. This
	 * is necessary to apply the final step of the Sugiyama algorithm to refine
	 * the node position within a layer.
	 * 
	 * @param list
	 *            : List of all {@link NodeLayout} objects within the current
	 *            layer
	 */
	private void addLayer(List<NodeLayout> list) {
		ArrayList<NodeWrapper> layer = new ArrayList<NodeWrapper>(list.size());
		for (NodeLayout node : list) {
			// wrap each NodeLayout with the internal data object and provide a
			// corresponding mapping
			NodeWrapper nw = new NodeWrapper(node, layers.size());
			map.put(node, nw);
			layer.add(nw);
			// insert dummy nodes if the adjacent layer does not contain the
			// predecessor
			for (NodeLayout node_predecessor : node.getPredecessingNodes()) { // for
																				// all
																				// predecessors
				NodeWrapper nw_predecessor = map.get(node_predecessor);
				for (int level = nw_predecessor.layer + 1; level < nw.layer; level++) {
					// add "virtual" wrappers (dummies) to the layers in between
					// virtual wrappers are in fact parts of a double linked
					// list
					NodeWrapper nw_dummy = new NodeWrapper(level);
					nw_dummy.addPredecessor(nw_predecessor);
					nw_predecessor.addSuccessor(nw_dummy);
					nw_predecessor = nw_dummy;
					layers.get(level).add(nw_dummy);
				}
				nw.addPredecessor(nw_predecessor);
				nw_predecessor.addSuccessor(nw);
			}
		}
		layers.add(layer);
		updateIndex(layer);
	}

	/**
	 * Reduces connection crossings between two adjacent layers by a combined
	 * top-down and bottom-up approach. It uses a heuristic approach based on
	 * the predecessor's barycenter.
	 */
	private void reduceCrossings() {
		for (int round = 0; round < MAX_SWEEPS; round++) {
			if ((round & 1) == 0) { // if round is even then do a bottom-up scan
				for (int index = 1; index < layers.size(); index++)
					reduceCrossingsDown(layers.get(index));
			} else { // else top-down
				for (int index = layers.size() - 2; index >= 0; index--)
					reduceCrossingsUp(layers.get(index));
			}
		}
	}

	private static void reduceCrossingsDown(ArrayList<NodeWrapper> layer) {
		// DOWN: scan PREDECESSORS
		for (NodeWrapper node : layer)
			node.index = node.getBaryCenter(node.pred);
		Collections.sort(layer, new Comparator<NodeWrapper>() {
			public int compare(NodeWrapper node1, NodeWrapper node2) {
				return (node1.index - node2.index);
			}
		});
		updateIndex(layer);
	}

	private static void reduceCrossingsUp(ArrayList<NodeWrapper> layer) {
		// UP: scan SUCCESSORS
		for (NodeWrapper node : layer)
			node.index = node.getBaryCenter(node.succ);
		Collections.sort(layer, new Comparator<NodeWrapper>() {
			public int compare(NodeWrapper node1, NodeWrapper node2) {
				return (node1.index - node2.index);
			}
		});
		updateIndex(layer);
	}

	/**
	 * Fills in virtual nodes, so the layer system finally becomes an
	 * equidistant grid
	 */
	private void padLayers() {
		last = 0;
		for (List<NodeWrapper> iter : layers)
			if (iter.size() > last)
				last = iter.size();
		last--; // index of the last element of any layer
		for (List<NodeWrapper> iter : layers) { // padding is always added at
												// the END of each layer!
			for (int i = iter.size(); i <= last; i++)
				iter.add(new NodeWrapper());
			updateIndex(iter);
		}
	}

	private void refineLayers() { // from Sugiyama paper: down, up and down
									// again yields best results, wonder why...
		for (int index = 1; index < layers.size(); index++)
			refineLayersDown(layers.get(index));
		for (int index = layers.size() - 2; index >= 0; index--)
			refineLayersUp(layers.get(index));
		for (int index = 1; index < layers.size(); index++)
			refineLayersDown(layers.get(index));
	}

	private void refineLayersDown(List<NodeWrapper> layer) {
		// first, get a priority list
		List<NodeWrapper> list = new ArrayList<NodeWrapper>(layer);
		Collections.sort(list, new Comparator<NodeWrapper>() {
			public int compare(NodeWrapper node1, NodeWrapper node2) {
				return (node2.getPriorityDown() - node1.getPriorityDown()); // descending
																			// ordering!!!
			}
		});
		// second, remove padding from the layer's end and place them in front
		// of the current node to improve its position
		for (NodeWrapper iter : list) {
			if (iter.isPadding())
				break; // break, if there are no more "real" nodes
			int delta = iter.getBaryCenter(iter.pred) - iter.index; // distance
																	// to new
																	// position
			for (int i = 0; i < delta; i++)
				layer.add(iter.index, layer.remove(last));
		}
		updateIndex(layer);
	}

	private void refineLayersUp(List<NodeWrapper> layer) {
		// first, get a priority list
		List<NodeWrapper> list = new ArrayList<NodeWrapper>(layer);
		Collections.sort(list, new Comparator<NodeWrapper>() {
			public int compare(NodeWrapper node1, NodeWrapper node2) {
				return (node2.getPriorityUp() - node1.getPriorityUp()); // descending
																		// ordering!!!
			}
		});
		// second, remove padding from the layer's end and place them in front
		// of the current node to improve its position
		for (NodeWrapper iter : list) {
			if (iter.isPadding())
				break; // break, if there are no more "real" nodes
			int delta = iter.getBaryCenter(iter.succ) - iter.index; // distance
																	// to new
																	// position
			for (int i = 0; i < delta; i++)
				layer.add(iter.index, layer.remove(last));
		}
		updateIndex(layer);
	}

	private void calculatePositions() {
		DisplayIndependentRectangle boundary = context.getBounds();
		if (dimension != null)
			boundary = new DisplayIndependentRectangle(0, 0,
					dimension.preciseWidth(), dimension.preciseHeight());
		double dx = boundary.width / layers.size();
		double dy = boundary.height / (last + 1);
		if (direction == HORIZONTAL)
			for (NodeLayout node : context.getNodes()) {
				NodeWrapper nw = map.get(node);
				node.setLocation((nw.layer + 0.5d) * dx, (nw.index + 0.5d) * dy);
			}
		else
			for (NodeLayout node : context.getNodes()) {
				NodeWrapper nw = map.get(node);
				node.setLocation((nw.index + 0.5d) * dx, (nw.layer + 0.5d) * dy);
			}
	}

	private static List<NodeLayout> findRoots(List<NodeLayout> list) {
		List<NodeLayout> roots = new ArrayList<NodeLayout>();
		for (NodeLayout iter : list) { // no predecessors means: this is a root,
										// add it to list
			if (iter.getPredecessingNodes().length == 0)
				roots.add(iter);
		}
		return (roots);
	}

	private static void updateIndex(List<NodeWrapper> list) {
		for (int index = 0; index < list.size(); index++)
			list.get(index).index = index;
	}

}