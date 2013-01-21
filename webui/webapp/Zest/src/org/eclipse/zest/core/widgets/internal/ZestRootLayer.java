/*******************************************************************************
 * Copyright (c) 2005-2010 IBM Corporation and others. All rights reserved.
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors: IBM Corporation - initial API and implementation Chisel Group,
 * University of Victoria - Adapted for XY Scaled Graphics
 ******************************************************************************/
package org.eclipse.zest.core.widgets.internal;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.Iterator;

import org.eclipse.draw2d.FreeformLayer;
import org.eclipse.draw2d.IFigure;

/**
 * The root figure for Zest. The figure is broken up into following segments:
 * <ol>
 * <li>The Connections</li>
 * <li>The Subgraphs</li>
 * <li>The Nodes</li>
 * <li>The Highlighted Connections</li>
 * <li>The Highlighted Nodes</li>
 * </ol>
 * 
 */
@SuppressWarnings({ "rawtypes", "unchecked" })
public class ZestRootLayer extends FreeformLayer {

	public static final int CONNECTIONS_LAYER = 0;

	public static final int SUBGRAPHS_LAYER = 1;

	public static final int NODES_LAYER = 2;

	public static final int CONNECTIONS_HIGHLIGHTED_LAYER = 3;

	public static final int NODES_HIGHLIGHTED_LAYER = 4;

	public static final int TOP_LAYER = 5;

	public static final int NUMBER_OF_LAYERS = 6;

	private final int[] itemsInLayer = new int[NUMBER_OF_LAYERS];

	/**
	 * Set of all figures that are decorations for other figures. A decoration
	 * figure is always put one position (or more if there's more than one
	 * decoration for the same figure) after the decorated figure in children
	 * list.
	 */
	private HashSet decoratingFigures = new HashSet();

	/**
	 * If true, it indicates that a figure is added using a proper method and
	 * its layer is known. Otherwise, Figure#add() method was used and layer
	 * must be guessed
	 */
	private boolean isLayerKnown = false;

	/**
	 * Adds a node to the ZestRootLayer
	 * 
	 * @param nodeFigure
	 *            The figure representing the node
	 */
	public void addNode(IFigure nodeFigure) {
		addFigure(nodeFigure, NODES_LAYER);
	}

	public void addConnection(IFigure connectionFigure) {
		addFigure(connectionFigure, CONNECTIONS_LAYER);
	}

	public void addSubgraph(IFigure subgraphFigrue) {
		addFigure(subgraphFigrue, SUBGRAPHS_LAYER);
	}

	public void highlightNode(IFigure nodeFigure) {
		changeFigureLayer(nodeFigure, NODES_HIGHLIGHTED_LAYER);
	}

	public void highlightConnection(IFigure connectionFigure) {
		changeFigureLayer(connectionFigure, CONNECTIONS_HIGHLIGHTED_LAYER);
	}

	public void unHighlightNode(IFigure nodeFigure) {
		changeFigureLayer(nodeFigure, NODES_LAYER);
	}

	public void unHighlightConnection(IFigure connectionFigure) {
		changeFigureLayer(connectionFigure, CONNECTIONS_LAYER);
	}

	private void changeFigureLayer(IFigure figure, int newLayer) {
		ArrayList decorations = getDecorations(figure);
		remove(figure);

		addFigure(figure, newLayer);
		for (Iterator iterator = decorations.iterator(); iterator.hasNext();) {
			addDecoration(figure, (IFigure) iterator.next());
		}

		this.invalidate();
		this.repaint();
	}

	private ArrayList getDecorations(IFigure figure) {
		ArrayList result = new ArrayList();
		int index = getChildren().indexOf(figure);
		if (index == -1) {
			return result;
		}
		for (index++; index < getChildren().size(); index++) {
			Object nextFigure = getChildren().get(index);
			if (decoratingFigures.contains(nextFigure)) {
				result.add(nextFigure);
			} else {
				break;
			}
		}
		return result;
	}

	/**
	 * 
	 * @param layer
	 * @return position after the last element in given layer
	 */
	private int getPosition(int layer) {
		int result = 0;
		for (int i = 0; i <= layer; i++) {
			result += itemsInLayer[i];
		}
		return result;
	}

	/**
	 * 
	 * @param position
	 * @return number of layer containing element at given position
	 */
	private int getLayer(int position) {
		int layer = 0;
		int positionInLayer = itemsInLayer[0];
		while (layer < NUMBER_OF_LAYERS - 1 && positionInLayer <= position) {
			layer++;
			positionInLayer += itemsInLayer[layer];
		}
		return layer;
	}

	public void addFigure(IFigure figure, int layer) {
		int position = getPosition(layer);
		itemsInLayer[layer]++;
		isLayerKnown = true;
		add(figure, position);
	}

	public void add(IFigure child, Object constraint, int index) {
		super.add(child, constraint, index);
		if (!isLayerKnown) {
			int layer = 0, positionInLayer = itemsInLayer[0];
			while (positionInLayer < index) {
				layer++;
				positionInLayer += itemsInLayer[layer];
			}
			if (index == -1) {
				layer = NUMBER_OF_LAYERS - 1;
			}
			itemsInLayer[layer]++;
		}
		isLayerKnown = false;
	}

	public void remove(IFigure child) {
		int position = this.getChildren().indexOf(child);
		if (position == -1) {
			return;
		}
		itemsInLayer[getLayer(position)]--;
		if (decoratingFigures.contains(child)) {
			decoratingFigures.remove(child);
			super.remove(child);
		} else {
			ArrayList decorations = getDecorations(child);
			super.remove(child);
			for (Iterator iterator = decorations.iterator(); iterator.hasNext();) {
				remove((IFigure) iterator.next());
			}
		}
	}

	public void addDecoration(IFigure decorated, IFigure decorating) {
		int position = this.getChildren().indexOf(decorated);
		if (position == -1) {
			throw new RuntimeException(
					"Can't add decoration for a figuer that is not on this ZestRootLayer");
		}
		itemsInLayer[getLayer(position)]++;
		isLayerKnown = true;
		do {
			position++;
		} while (position < getChildren().size()
				&& decoratingFigures.contains(getChildren().get(position)));
		decoratingFigures.add(decorating);
		add(decorating, position);
	}
}
