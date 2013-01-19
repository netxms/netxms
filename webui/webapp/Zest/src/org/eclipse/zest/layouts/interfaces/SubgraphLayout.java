/*******************************************************************************
 * Copyright (c) 2009-2010 Mateusz Matela and others. All rights reserved. This
 * program and the accompanying materials are made available under the terms of
 * the Eclipse Public License v1.0 which accompanies this distribution, and is
 * available at http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors: Mateusz Matela - initial API and implementation
 *               Ian Bull
 ******************************************************************************/
package org.eclipse.zest.layouts.interfaces;

/**
 * An interface for subgraphs in layout. A subgraph is a set of pruned nodes
 * that will be displayed as one element. A subgraph must contain at least one
 * node (empty subgraphs will be removed from its context). Every node can
 * belong to at most one subgraph.
 */
public interface SubgraphLayout extends EntityLayout {

	/**
	 * Constant for top-down direction (default).
	 */
	public final int TOP_DOWN = 1;

	/**
	 * Constant for bottom-up direction.
	 */
	public final int BOTTOM_UP = 2;

	/**
	 * Constant for direction from left to right.SubgraphLayout
	 */
	public final int LEFT_RIGHT = 3;

	/**
	 * Constant for direction from right to left.
	 */
	public final int RIGHT_LEFT = 4;

	/**
	 * Returns all the nodes belonging to this subgraph. Replacing elements in
	 * the returned array does not affect this subgraph.
	 * 
	 * @return array of nodes
	 */
	public NodeLayout[] getNodes();

	/**
	 * 
	 * @return number of nodes pruned into this subgraph
	 */
	public int countNodes();

	/**
	 * Adds nodes to this subgraph. If given nodes already belong to another
	 * subgraph, they are first removed from them.
	 * 
	 * @param nodes
	 *            array of nodes to add
	 */
	public void addNodes(NodeLayout[] nodes);

	/**
	 * Removes nodes from this subgraph.
	 * 
	 * @param nodes
	 *            array of nodes to remove
	 */
	public void removeNodes(NodeLayout[] nodes);

	/**
	 * Returns true if this subgraph is visualized as a particular object on the
	 * graph. If this method returns false, it means that this subgraph will not
	 * be visible so all methods related to location, size and direction should
	 * be ignored.
	 * 
	 * @return whether or not this subgraph is a graph entity that should be
	 *         laid out.
	 */
	public boolean isGraphEntity();

	/**
	 * @return true if this subgraph is visualized differently depending on
	 *         direction
	 */
	public boolean isDirectionDependant();

	/**
	 * Sets the direction of this subgraph (does nothing in case of subgraphs
	 * that don't depend on direction)
	 * 
	 * @param direction
	 *            one of constants: {@link #TOP_DOWN}, {@link #BOTTOM_UP},
	 *            {@link #LEFT_RIGHT}, {@link #RIGHT_LEFT}
	 */
	public void setDirection(int direction);

}
