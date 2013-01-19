/*******************************************************************************
 * Copyright (c) 2005-2010 The Chisel Group and others. All rights reserved. This
 * program and the accompanying materials are made available under the terms of
 * the Eclipse Public License v1.0 which accompanies this distribution, and is
 * available at http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors: The Chisel Group - initial API and implementation
 *               Mateusz Matela 
 *               Ian Bull
 ******************************************************************************/
package org.eclipse.zest.layouts;

import org.eclipse.zest.layouts.interfaces.LayoutContext;

/**
 * An interface for all layout algorithms.
 * 
 * 
 */
public interface LayoutAlgorithm {

	/**
	 * Sets the layout context for this algorithm. The receiver will unregister
	 * from its previous layout context and register to the new one
	 * (registration means for example adding listeners). After a call to this
	 * method, the receiving algorithm can compute and cache internal data
	 * related to given context and perform an initial layout.
	 * 
	 * @param context
	 *            a new layout context or null if this algorithm should not
	 *            perform any layout
	 */
	public void setLayoutContext(LayoutContext context);

	/**
	 * Makes this algorithm perform layout computation and apply it to its
	 * context.
	 * 
	 * @param clean
	 *            if true the receiver should assume that the layout context has
	 *            changed significantly and recompute the whole layout even if
	 *            it keeps track of changes with listeners. False can be used
	 *            after dynamic layout in a context is turned back on so that
	 *            layout algorithm working in background can apply accumulated
	 *            changes. Static layout algorithm can ignore this call entirely
	 *            if clean is false.
	 */
	public void applyLayout(boolean clean);

}
