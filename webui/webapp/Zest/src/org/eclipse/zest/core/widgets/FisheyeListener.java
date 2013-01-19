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

import org.eclipse.draw2d.IFigure;

/**
 * Interface for listener that can be added to {@link Graph} and receive
 * notifications when fisheye figures are added, removed or replaced in it.
 * 
 * @since 2.0
 */
public interface FisheyeListener {

	/**
	 * Called when a fisheye figure is added to an observed graph.
	 * 
	 * @param graph
	 *            observed graph
	 * @param originalFigure
	 *            figure to be fisheyed
	 * @param fisheyeFigure
	 *            the added fisheye figure
	 */
	public void fisheyeAdded(Graph graph, IFigure originalFigure,
			IFigure fisheyeFigure);

	/**
	 * Called when a fisheye figure is removed form an observed graph.
	 * 
	 * @param graph
	 *            observed graph
	 * @param originalFigure
	 *            figure that was fisheyed
	 * @param fisheyeFigure
	 *            the removed fisheye figure
	 */
	public void fisheyeRemoved(Graph graph, IFigure originalFigure,
			IFigure fisheyeFigure);

	/**
	 * Called when one fisheye figure is replaced by another in an observed
	 * graph.
	 * 
	 * @param graph
	 *            observed graph
	 * @param oldFisheyeFigure
	 *            fisheye figure that is replaced
	 * @param newFisheyeFigure
	 *            fisheye figure that replaces the old figure
	 */
	public void fisheyeReplaced(Graph graph, IFigure oldFisheyeFigure,
			IFigure newFisheyeFigure);
}
