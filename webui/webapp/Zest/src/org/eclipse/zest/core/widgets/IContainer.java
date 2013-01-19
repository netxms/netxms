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

import java.util.List;

import org.eclipse.draw2d.IFigure;
import org.eclipse.swt.widgets.Widget;
import org.eclipse.zest.layouts.LayoutAlgorithm;
import org.eclipse.zest.layouts.dataStructures.DisplayIndependentRectangle;

/**
 * @noimplement This interface is not intended to be implemented by clients.
 * 
 */
public interface IContainer {

	public abstract Graph getGraph();

	/**
	 * @since 2.0
	 */
	public Widget getItem();

	public abstract List getNodes();

	/**
	 * Returns list of connections laying inside this container. Only
	 * connections which both source and target nodes lay directly in this
	 * container are returned.
	 * 
	 * @return
	 * @since 2.0
	 */
	public abstract List getConnections();

	/**
	 * @noreference This method is not intended to be referenced by clients.
	 * @param graphNode
	 */
	public abstract void addNode(GraphNode graphNode);

	/**
	 * @noreference This method is not intended to be referenced by clients.
	 * @param figure
	 */
	public abstract void addSubgraphFigure(IFigure figure);

	public abstract int getItemType();

	/**
	 * @return
	 * @since 2.0
	 */
	public abstract DisplayIndependentRectangle getLayoutBounds();

	/**
	 * @noreference This method is not intended to be referenced by clients.
	 * @return
	 */
	public abstract InternalLayoutContext getLayoutContext();

	public void applyLayout();

	public void setLayoutAlgorithm(LayoutAlgorithm algorithm, boolean apply);

	/**
	 * Takes a list of connections and returns only those which source and
	 * target nodes lay directly in this container.
	 * 
	 * @param connections
	 *            list of GraphConnection to filter
	 * @return filtered list
	 */
	// protected List filterConnections(List connections) {
	// List result = new ArrayList();
	// for (Iterator iterator = connections.iterator(); iterator.hasNext();) {
	// GraphConnection connection = (GraphConnection) iterator.next();
	// if (connection.getSource().getParent() == this &&
	// connection.getDestination().getParent() == this)
	// result.add(connection);
	// }
	// return result;
	// }
}
