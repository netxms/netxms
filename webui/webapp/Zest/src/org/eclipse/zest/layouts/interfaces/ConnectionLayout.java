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

public interface ConnectionLayout {

	public NodeLayout getSource();

	public NodeLayout getTarget();

	/**
	 * 
	 * @return weight assigned to this connection
	 */
	public double getWeight();

	/**
	 * Checks if this connection is directed. For undirected connections, source
	 * and target nodes should be considered just adjacent nodes without
	 * dividing to source/target.
	 * 
	 * @return true if this connection is directed
	 */
	public boolean isDirected();

	/**
	 * Changes the visibility state of this connection.
	 * 
	 * @param visible
	 *            true if this connection should be visible, false otherwise
	 */
	public void setVisible(boolean visible);

	/**
	 * Checks the visibility state of this connection.
	 * 
	 * @return true if this connection is visible, false otherwise
	 */
	public boolean isVisible();
}
