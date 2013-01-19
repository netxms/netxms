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
 * A manager that controls expanding and collapsing nodes in a Graph.
 */
public interface ExpandCollapseManager {

	/**
	 * Initializes the expansion state of all nodes in given layout context. The
	 * receiver can initialize its internal state related to the layout context
	 * and add its listeners if necessary.
	 * 
	 * @param context
	 *            the context to initialize
	 */
	public void initExpansion(LayoutContext context);

	/**
	 * Changes the expanded state of given node. It prunes/unprunes nodes and
	 * hides/shows connections in the graph according to its policy. If
	 * requested operation cannot be currently performed on the node, it does
	 * nothing.
	 * 
	 * @param context
	 *            context in which to perform the operation
	 * @param node
	 *            node to expand or collapse
	 * @param expanded
	 *            true to expand, false to collapse
	 */
	public void setExpanded(LayoutContext context, NodeLayout node,
			boolean expanded);

	/**
	 * Checks if given node can be expanded.
	 * 
	 * @param context
	 *            context containing the node
	 * @param node
	 *            node to check
	 * @return
	 */
	public boolean canExpand(LayoutContext context, NodeLayout node);

	/**
	 * Checks if given node can be collapsed.
	 * 
	 * @param context
	 *            context containing the node
	 * @param node
	 *            node to check
	 * @return
	 */
	public boolean canCollapse(LayoutContext context, NodeLayout node);
}