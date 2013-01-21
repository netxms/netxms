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

import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;

import org.eclipse.draw2d.AncestorListener;
import org.eclipse.draw2d.ColorConstants;
import org.eclipse.draw2d.FigureListener;
import org.eclipse.draw2d.IFigure;
import org.eclipse.draw2d.geometry.Dimension;
import org.eclipse.draw2d.geometry.Point;
import org.eclipse.draw2d.geometry.Rectangle;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.FontData;
import org.eclipse.swt.widgets.Display;
import org.eclipse.zest.core.widgets.internal.GraphLabel;
import org.eclipse.zest.core.widgets.internal.ZestRootLayer;
import org.eclipse.zest.layouts.interfaces.LayoutContext;
import org.eclipse.zest.layouts.interfaces.NodeLayout;

/**
 * A subgraph that for each unexpanded node in a graph adds a label showing
 * number of pruned successors (as unexpanded node is considered a node for
 * which {@link Graph#canExpand(GraphNode)} returns true AND
 * {@link Graph#canCollapse(GraphNode)} returns false). It doesn't matter which
 * subgraph a node is pruned to, so the factory for this subgraph uses one
 * instance for whole layout context.
 */
@SuppressWarnings({ "rawtypes", "unchecked" })
class PrunedSuccessorsSubgraph extends DefaultSubgraph {

	private class LabelAncestorListener extends AncestorListener.Stub {
		private final IFigure originalFigure;
		private IFigure fisheyeFigure;

		public LabelAncestorListener(IFigure originalFigure,
				IFigure fisheyeFigure) {
			this.originalFigure = originalFigure;
			this.fisheyeFigure = fisheyeFigure;
		}

		public void ancestorRemoved(IFigure ancestor) {
			if (fisheyeFigure != null) {
				final GraphLabel label = (GraphLabel) nodeFigureToLabel
						.get(fisheyeFigure);
				if (label == null) {
					return;
				}
				nodeFigureToLabel.remove(fisheyeFigure);
				Display.getDefault().asyncExec(new Runnable() {
					public void run() {
						label.removeAncestorListener(LabelAncestorListener.this);
					}
				});
				fisheyeFigure.removeFigureListener(nodeFigureListener);
				originalFigure.addFigureListener(nodeFigureListener);
				labelToAncestorListener.remove(label);
				fisheyeFigure = null;
				addLabelForFigure(originalFigure, label);
				refreshLabelBounds(originalFigure, label);
			}
		}
	}

	private final FigureListener nodeFigureListener = new FigureListener() {
		public void figureMoved(IFigure source) {
			GraphLabel label = (GraphLabel) nodeFigureToLabel.get(source);
			if (label != null) {
				refreshLabelBounds(source, label);
			}
		}
	};

	private final FisheyeListener fisheyeListener = new FisheyeListener() {

		public void fisheyeReplaced(Graph graph, IFigure oldFisheyeFigure,
				IFigure newFisheyeFigure) {
			oldFisheyeFigure.removeFigureListener(nodeFigureListener);
			newFisheyeFigure.addFigureListener(nodeFigureListener);
			GraphLabel label = (GraphLabel) nodeFigureToLabel
					.remove(oldFisheyeFigure);
			nodeFigureToLabel.put(newFisheyeFigure, label);

			LabelAncestorListener ancestorListener = (LabelAncestorListener) labelToAncestorListener
					.get(label);
			ancestorListener.fisheyeFigure = null;
			addLabelForFigure(newFisheyeFigure, label);
			ancestorListener.fisheyeFigure = newFisheyeFigure;
			refreshLabelBounds(newFisheyeFigure, label);
		}

		public void fisheyeRemoved(Graph graph, IFigure originalFigure,
				IFigure fisheyeFigure) {
			// do nothing - labelAncestorListener will take care of cleaning
			// up
		}

		public void fisheyeAdded(Graph graph, IFigure originalFigure,
				IFigure fisheyeFigure) {
			originalFigure.removeFigureListener(nodeFigureListener);
			fisheyeFigure.addFigureListener(nodeFigureListener);
			GraphLabel label = (GraphLabel) nodeFigureToLabel
					.get(originalFigure);
			if (label == null) {
				return;
			}
			nodeFigureToLabel.put(fisheyeFigure, label);
			refreshLabelBounds(fisheyeFigure, label);
			addLabelForFigure(fisheyeFigure, label);
			LabelAncestorListener labelAncestorListener = new LabelAncestorListener(
					originalFigure, fisheyeFigure);
			label.addAncestorListener(labelAncestorListener);
			labelToAncestorListener.put(label, labelAncestorListener);
		}
	};

	/**
	 * Maps from figures of nodes to labels showing number of nodes hidden
	 * successors
	 */
	private HashMap nodeFigureToLabel = new HashMap();

	private HashMap labelToAncestorListener = new HashMap();

	protected PrunedSuccessorsSubgraph(LayoutContext context2) {
		super(context2);
		context.container.getGraph().addFisheyeListener(fisheyeListener);
	}

	public void addNodes(NodeLayout[] nodes) {
		super.addNodes(nodes);
		HashSet nodesToUpdate = new HashSet();
		for (int i = 0; i < nodes.length; i++) {
			nodesToUpdate
					.addAll(Arrays.asList(nodes[i].getPredecessingNodes()));
		}
		for (Iterator iterator = nodesToUpdate.iterator(); iterator.hasNext();) {
			InternalNodeLayout nodeToUpdate = (InternalNodeLayout) iterator
					.next();
			updateNodeLabel(nodeToUpdate);
		}

	}

	public void removeNodes(NodeLayout[] nodes) {
		super.removeNodes(nodes);
		HashSet nodesToUpdate = new HashSet();
		for (int i = 0; i < nodes.length; i++) {
			nodesToUpdate
					.addAll(Arrays.asList(nodes[i].getPredecessingNodes()));
			if (((InternalNodeLayout) nodes[i]).isDisposed()) {
				removeFigureForNode((InternalNodeLayout) nodes[i]);
			} else {
				nodesToUpdate.add(nodes[i]);
			}
		}
		for (Iterator iterator = nodesToUpdate.iterator(); iterator.hasNext();) {
			InternalNodeLayout predecessor = (InternalNodeLayout) iterator
					.next();
			updateNodeLabel(predecessor);
		}
	}

	private void addLabelForFigure(IFigure figure, GraphLabel label) {
		IFigure parent = figure.getParent();
		if (parent instanceof ZestRootLayer) {
			((ZestRootLayer) parent).addDecoration(figure, label);
		} else {
			if (parent.getChildren().contains(label)) {
				parent.remove(label);
			}
			int index = parent.getChildren().indexOf(figure);
			parent.add(label, index + 1);
		}
	}

	private void refreshLabelBounds(IFigure figure, GraphLabel label) {
		Rectangle figureBounds = figure.getBounds();
		if (figureBounds.width * figureBounds.height > 0) {
			label.setText(label.getText()); // hack: resets label's size
			Dimension labelSize = label.getSize();
			labelSize.expand(-6, -4);
			Point anchorPoint = figure.getBounds().getBottomRight();
			anchorPoint.x -= labelSize.width / 2;
			anchorPoint.y -= labelSize.height / 2;
			Rectangle bounds = new Rectangle(anchorPoint, labelSize);
			label.setBounds(bounds);
			label.getParent().setConstraint(label, bounds);
		} else {
			label.getParent().setConstraint(label,
					new Rectangle(figureBounds.x, figureBounds.y, 0, 0));
			label.setBounds(new Rectangle(figureBounds.x, figureBounds.y, 0, 0));
		}
	}

	void updateNodeLabel(InternalNodeLayout internalNode) {
		if (internalNode.isDisposed()) {
			return;
		}
		IFigure figure = internalNode.getNode().getFigure();
		GraphLabel label = (GraphLabel) nodeFigureToLabel.get(figure);
		IFigure fisheye = getFisheyeFigure(figure);
		if (fisheye != null) {
			figure = fisheye;
		}
		if (label == null) {
			label = new GraphLabel(false);
			label.setForegroundColor(ColorConstants.white);
			label.setBackgroundColor(ColorConstants.red);
			FontData fontData = Display.getDefault().getSystemFont()
					.getFontData()[0];
			fontData.setHeight(6);
			label.setFont(new Font(Display.getCurrent(), fontData));
			figure.addFigureListener(nodeFigureListener);
			addLabelForFigure(figure, label);
			nodeFigureToLabel.put(figure, label);
		}

		GraphNode graphNode = internalNode.getNode();
		if (!graphNode.getGraphModel().canExpand(graphNode)
				|| graphNode.getGraphModel().canCollapse(graphNode)
				|| internalNode.isPruned()) {
			label.setVisible(false);
		} else {
			NodeLayout[] successors = internalNode.getSuccessingNodes();
			int numberOfHiddenSuccessors = 0;
			for (int i = 0; i < successors.length; i++) {
				if (successors[i].isPruned()) {
					numberOfHiddenSuccessors++;
				}
			}
			String labelText = numberOfHiddenSuccessors > 0 ? ""
					+ numberOfHiddenSuccessors : "";
			if (!labelText.equals(label.getText())) {
				label.setText(labelText);
			}
			label.setVisible(true);
		}

		refreshLabelBounds(figure, label);
	}

	private IFigure getFisheyeFigure(IFigure originalFigure) {
		// a node has a fisheye if and only if its label has an AncestorListener
		GraphLabel label = (GraphLabel) nodeFigureToLabel.get(originalFigure);
		LabelAncestorListener ancestorListener = (LabelAncestorListener) labelToAncestorListener
				.get(label);
		if (ancestorListener != null) {
			return ancestorListener.fisheyeFigure;
		}
		return null;
	}

	private void removeFigureForNode(InternalNodeLayout internalNode) {
		IFigure figure = internalNode.getNode().getFigure();
		GraphLabel label = (GraphLabel) nodeFigureToLabel.get(figure);
		if (label != null && label.getParent() != null) {
			label.getParent().remove(label);
		}
		nodeFigureToLabel.remove(figure);
	}
}
