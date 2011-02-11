/*******************************************************************************
 * Copyright (c) 2006-2009 Nicolas Richeton.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors :
 *    Nicolas Richeton (nicolas.richeton@gmail.com) - initial API and implementation
 *******************************************************************************/

package org.netxms.nebula.animation.effects;

import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Control;
import org.netxms.nebula.animation.movement.IMovement;

/**
 * Changes all bounds of a control.
 * 
 * @author Nicolas Richeton
 * 
 */
public class SetBoundsEffect extends AbstractEffect {

	Rectangle src, dest, diff;

	Control control = null;

	public SetBoundsEffect(Control control, Rectangle src, Rectangle dest,
			long lengthMilli, IMovement movement, Runnable onStop,
			Runnable onCancel) {
		super(lengthMilli, movement, onStop, onCancel);
		this.src = src;
		this.dest = dest;
		this.control = control;
		this.diff = new Rectangle(dest.x - src.x, dest.y - src.y, dest.width
				- src.width, dest.height - src.height);

		easingFunction.init(0, 1, (int) lengthMilli);
	}

	public void applyEffect(final long currentTime) {
		if (!control.isDisposed()) {
			control.setBounds((int) (src.x + diff.x
					* easingFunction.getValue(currentTime)),
					(int) (src.y + diff.y
							* easingFunction.getValue(currentTime)),
					(int) (src.width + diff.width
							* easingFunction.getValue(currentTime)),
					(int) (src.height + diff.height
							* easingFunction.getValue(currentTime)));
		}
	}
}