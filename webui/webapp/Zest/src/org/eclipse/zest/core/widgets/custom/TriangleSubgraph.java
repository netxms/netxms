/*******************************************************************************
 * Copyright (c) 2009-2010 Mateusz Matela and others. All rights reserved. This
 * program and the accompanying materials are made available under the terms of
 * the Eclipse Public License v1.0 which accompanies this distribution, and is
 * available at http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors: Mateusz Matela - initial API and implementation Ian Bull
 ******************************************************************************/
package org.eclipse.zest.core.widgets.custom;

import java.util.HashMap;

import org.eclipse.draw2d.ColorConstants;
import org.eclipse.draw2d.Graphics;
import org.eclipse.draw2d.Shape;
import org.eclipse.draw2d.geometry.PointList;
import org.eclipse.draw2d.geometry.Rectangle;
import org.eclipse.swt.graphics.Color;
import org.eclipse.zest.core.widgets.FigureSubgraph;
import org.eclipse.zest.layouts.algorithms.TreeLayoutObserver;
import org.eclipse.zest.layouts.algorithms.TreeLayoutObserver.TreeListener;
import org.eclipse.zest.layouts.algorithms.TreeLayoutObserver.TreeNode;
import org.eclipse.zest.layouts.interfaces.LayoutContext;
import org.eclipse.zest.layouts.interfaces.NodeLayout;
import org.eclipse.zest.layouts.interfaces.SubgraphLayout;

/**
 * A subgraph that is visualized in a graph as a triangle. It assumes that nodes
 * in context that uses them are arranged in a tree structure and the nodes
 * added to the subgraph are a subtree (except for the subtree's root, which
 * should not be added).
 * 
 * The triangle has three features that show the properties of a subtree
 * contained within it:
 * <ul>
 * <li><b>Height of the triangle</b> is proportional to the height of the
 * subtree. If the subtree contains the whole tree, the triangle's height will
 * be equal to value provided with
 * {@link TriangleSubgraph#setReferenceHeight(double)} (default is 50).</li>
 * <li><b>Length of the triangle's base</b> depends on the number of leaves in
 * the subtree. More precisely, it is proportional to the logarithm of the
 * percent that the subtree's leaves make of the whole context's leaves. The
 * proportion factor is adjusted so that for a subtree containing all the leaves
 * the base has length provided with {@link TriangleSubgraph}
 * {@link #setReferenceBase(double)} (default is 50) and for a subtree
 * containing only one leaf the base has length 1.</li>
 * <li><b>Background color of the triangle</b> depends on average number of
 * children for nodes in the subtree. The less is this value, the more bright is
 * the color (up to white for a subtree with average number of children equal to
 * 1). The average value is calculated only for nodes that have at least one
 * child. The root of the subtree (which is not directly added to this subgraph)
 * is also accounted.</li>
 * </ul>
 * 
 * When the first subgraph of this class is created for a layout context, a
 * {@link TreeLayoutObserver} is created for the context. It must keep track of
 * changes in the graph structure, so events related to it should not be
 * intercepted by other listeners before they reach the subgraph's observer.
 * 
 * @since 2.0
 */
public class TriangleSubgraph extends FigureSubgraph {

	public static class TriangleParameters implements Cloneable {
		public Color color = ColorConstants.black;

		public int direction = TOP_DOWN;

		public double referenceHeight = 50;

		public double referenceBase = 50;

		public Object clone() {
			TriangleParameters result = new TriangleParameters();
			result.color = color;
			result.direction = direction;
			result.referenceHeight = referenceHeight;
			result.referenceBase = referenceBase;
			return result;
		}
	}

	private class IsoscelesTriangle extends Shape {

		private PointList points = new PointList(3);

		protected void fillShape(Graphics graphics) {
			graphics.fillPolygon(points);
		}

		protected void outlineShape(Graphics graphics) {
			graphics.drawPolygon(points);
		}

		protected void primTranslate(int dx, int dy) {
			super.primTranslate(dx, dy);
			points.translate(dx, dy);
		}

		public void validate() {
			super.validate();
			Rectangle r = new Rectangle();
			r.setBounds(getBounds());
			r.shrink(getInsets());
			points.removeAllPoints();
			switch (parameters.direction) {
			case TOP_DOWN:
				points.addPoint(r.x + r.width / 2, r.y);
				points.addPoint(r.x, r.y + r.height);
				points.addPoint(r.x + r.width, r.y + r.height);
				break;
			case BOTTOM_UP:
				points.addPoint(r.x + r.width / 2, r.y + r.height);
				points.addPoint(r.x, r.y);
				points.addPoint(r.x + r.width, r.y);
				break;
			case LEFT_RIGHT:
				points.addPoint(r.x, r.y + r.height / 2);
				points.addPoint(r.x + r.width, r.y);
				points.addPoint(r.x + r.width, r.y + r.height);
				break;
			case RIGHT_LEFT:
				points.addPoint(r.x + r.width, r.y + r.height / 2);
				points.addPoint(r.x, r.y);
				points.addPoint(r.x, r.y + r.height);
				break;
			}
		}
	}

	private static HashMap contextToTree = new HashMap();

	private TriangleParameters parameters;

	public TriangleSubgraph(NodeLayout[] nodes, LayoutContext context,
			TriangleParameters triangleParameters) {
		super(nodes, context);
		this.parameters = triangleParameters;
		if (contextToTree.get(context) == null) {
			TreeLayoutObserver treeLayoutObserver = new TreeLayoutObserver(
					context, null);
			treeLayoutObserver.addTreeListener(new TreeListener() {
				protected void defaultHandle(TreeNode changedNode) {
					SubgraphLayout subgraph = changedNode.getNode()
							.getSubgraph();
					if (subgraph instanceof TriangleSubgraph) {
						((TriangleSubgraph) subgraph).updateFigure();
					}
				}
			});
			contextToTree.put(context, treeLayoutObserver);
		}
	}

	protected void createFigure() {
		figure = new IsoscelesTriangle();
		figure.setBackgroundColor(parameters.color);
		figure.setForegroundColor(parameters.color);
	}

	private double log(double value, double base) {
		return Math.log(value) / Math.log(base);
	}

	protected void updateFigure() {
		TreeLayoutObserver tree = (TreeLayoutObserver) contextToTree
				.get(context);
		TreeNode subgraphRoot = tree.getTreeNode((NodeLayout) nodes.iterator()
				.next());
		if (subgraphRoot == null) {
			return;
		}
		while (nodes.contains(subgraphRoot.getNode())) {
			subgraphRoot = subgraphRoot.getParent();
		}

		TreeNode superRoot = tree.getSuperRoot();
		double triangleHeight = parameters.referenceHeight
				* subgraphRoot.getHeight() / superRoot.getHeight();

		int numOfNodes = superRoot.getNumOfDescendants();
		int numOfNodesWithChildren = numOfNodes - superRoot.getNumOfLeaves()
				+ 1;
		double logBase = (numOfNodesWithChildren > 0) ? (double) numOfNodes
				/ numOfNodesWithChildren : 1;
		// logarithm base is the average number of children for whole context
		double triangleBaseModifier = (parameters.referenceBase - 1)
				/ log(superRoot.getNumOfLeaves(), logBase);
		double triangleBase = parameters.referenceBase
				+ triangleBaseModifier
				* log((double) subgraphRoot.getNumOfLeaves()
						/ superRoot.getNumOfLeaves(), logBase);

		if (parameters.direction == 0) {
			parameters.direction = parameters.direction;
		}
		if (parameters.direction == TOP_DOWN
				|| parameters.direction == BOTTOM_UP) {
			figure.setSize((int) (triangleBase + 0.5),
					(int) (triangleHeight + 0.5));
		} else {
			figure.setSize((int) (triangleHeight + 0.5),
					(int) (triangleBase + 0.5));
		}

		int numOfNodesWithChildrenInSubgraph = nodes.size()
				- subgraphRoot.getNumOfLeaves() + 1;
		double avgNumOfChildrenInSugbraph = (numOfNodesWithChildrenInSubgraph > 0) ? (double) nodes
				.size() / numOfNodesWithChildrenInSubgraph
				: 1;
		int r = (int) (parameters.color.getRed() + ((double) 255 - parameters.color
				.getRed()) / avgNumOfChildrenInSugbraph);
		int g = (int) (parameters.color.getGreen() + ((double) 255 - parameters.color
				.getGreen()) / avgNumOfChildrenInSugbraph);
		int b = (int) (parameters.color.getBlue() + ((double) 255 - parameters.color
				.getBlue()) / avgNumOfChildrenInSugbraph);
		figure.setBackgroundColor(new Color(parameters.color.getDevice(), r, g,
				b));
		figure.setForegroundColor(parameters.color);
	}

	public boolean isDirectionDependant() {
		return true;
	}

	public void setDirection(int direction) {
		super.setDirection(direction);
		if (parameters.direction == direction) {
			return;
		}
		if (direction == TOP_DOWN || direction == BOTTOM_UP
				|| direction == LEFT_RIGHT || direction == RIGHT_LEFT) {
			parameters.direction = direction;
			updateFigure();
		} else {
			throw new IllegalArgumentException("invalid direction");
		}
	}

	/**
	 * Changes the color of the triangle visualizing this subgraph.
	 * 
	 * @param color
	 *            color to use
	 */
	public void setColor(Color color) {
		parameters.color = color;
		updateFigure();
	}
}
