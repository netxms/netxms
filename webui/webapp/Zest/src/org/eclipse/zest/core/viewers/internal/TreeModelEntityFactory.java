/*******************************************************************************
 * Copyright (c) 2011 Sushant Sirsikar and others. All rights reserved. This 
 * program and the accompanying materials are made available under the terms of 
 * the Eclipse Public License v1.0 which accompanies this distribution, and is 
 * available at http://www.eclipse.org/legal/epl-v10.html
 * <p/>
 * Contributors:  Sushant Sirsikar - initial implementation
 *                Zoltan Ujhelyi - adaptation to Zest 2.0
 *******************************************************************************/
package org.eclipse.zest.core.viewers.internal;

import org.eclipse.jface.viewers.ITreeContentProvider;
import org.eclipse.zest.core.viewers.AbstractStructuredGraphViewer;
import org.eclipse.zest.core.viewers.EntityConnectionData;
import org.eclipse.zest.core.widgets.Graph;
import org.eclipse.zest.core.widgets.GraphConnection;
import org.eclipse.zest.core.widgets.GraphNode;

/**
 * A model factory for creating graph content based on an ITreeContentProvider
 * 
 * @author sushant sirsikar
 */
public class TreeModelEntityFactory extends AbstractStylingModelFactory {

	AbstractStructuredGraphViewer viewer = null;

	public TreeModelEntityFactory(AbstractStructuredGraphViewer viewer) {
		super(viewer);
		this.viewer = viewer;
	}

	@Override
	public Graph createGraphModel(Graph model) {
		doBuildGraph(model);
		return model;
	}

	@Override
	protected void doBuildGraph(Graph model) {
		super.doBuildGraph(model);
		Object inputElement = getViewer().getInput();
		ITreeContentProvider provider = (ITreeContentProvider) getContentProvider();
		Object[] entities = provider.getElements(inputElement);
		if (entities == null)
			return;
		for (Object data : entities) {
			createGraphNodes(model, data, provider);
		}
	}

	private GraphNode createGraphNodes(Graph model, Object data,
			ITreeContentProvider provider) {
		GraphNode node = createNode(model, data);
		if (provider.hasChildren(data)) {
			for (Object child : provider.getChildren(data)) {
				GraphNode childNode = createGraphNodes(model, child, provider);
				EntityConnectionData connectionData = new EntityConnectionData(
						node, childNode);
				createConnection(model, connectionData, data, child);
			}
		}
		return node;
	}

	@Override
	public void refresh(Graph graph, Object element, boolean updateLabels) {
		if (element == null) {
			return;
		}
		GraphNode node = viewer.getGraphModelNode(element);
		if (node == null) {
			GraphConnection conn = viewer.getGraphModelConnection(element);
			if (conn != null) {
				refresh(graph, conn.getSource().getData(), updateLabels);
				refresh(graph, conn.getDestination().getData(), updateLabels);
				return;
			}
		}
		if (node == null)
			return;

		reconnect(graph, element, updateLabels);

		if (updateLabels)
			update(node);
	}

	/**
	 * Reconnects all the connections.
	 * 
	 * @param graph
	 * @param element
	 * @param updateLabels
	 */
	private void reconnect(Graph graph, Object element, boolean updateLabels) {
		GraphNode sourceNode = viewer.getGraphModelNode(element);
		Object[] children = ((ITreeContentProvider) getContentProvider())
				.getChildren(element);
		EntityConnectionData data = null;
		GraphConnection oldConnection = null;
		GraphConnection newConnection = null;
		for (Object child : children) {
			GraphNode destNode = viewer.getGraphModelNode(child);
			data = new EntityConnectionData(sourceNode, destNode);
			oldConnection = viewer.getGraphModelConnection(data);
			if (oldConnection != null) {
				viewer.removeGraphModelConnection(oldConnection);
				newConnection = createConnection(graph, data,
						sourceNode.getData(), destNode.getData());
				if (updateLabels) {
					styleItem(newConnection);
				}
			}
			refresh(graph, child, updateLabels);
		}
	}
}
