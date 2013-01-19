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
package org.eclipse.zest.core.widgets.custom;

import org.eclipse.draw2d.IFigure;
import org.eclipse.zest.core.widgets.Graph;
import org.eclipse.zest.core.widgets.GraphContainer;
import org.eclipse.zest.core.widgets.GraphNode;

/**
 * A Custom Graph Node
 * 
 * @since 2.0
 */
public class CGraphNode extends GraphNode {

	IFigure figure = null;

	/**
	 * @since 2.0
	 */
	public CGraphNode(Graph graphModel, int style, IFigure figure) {
		super(graphModel, style, figure);
	}

	/**
	 * @since 2.0
	 */
	public CGraphNode(GraphContainer graphModel, int style, IFigure figure) {
		super(graphModel, style, figure);
	}

	public IFigure getFigure() {
		return super.getFigure();
	}

	protected IFigure createFigureForModel() {
		this.figure = (IFigure) this.getData();
		return this.figure;
	}

}
