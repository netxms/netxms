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

import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.widgets.Control;
import org.netxms.nebula.animation.AnimationRunner;
import org.netxms.nebula.animation.movement.IMovement;

/**
 * Changes width and height of a control.
 * 
 * @author Nicolas Richeton
 */
public class ResizeEffect extends AbstractEffect {

	/**
	 * @deprecated
	 * @param w
	 * @param x
	 * @param y
	 * @param duration
	 * @param movement
	 * @param onStop
	 * @param onCancel
	 */
	public static void resize(AnimationRunner runner, Control w, int x, int y,
			int duration, IMovement movement, Runnable onStop, Runnable onCancel) {
		Point oldSize = w.getSize();
		IEffect effect = new ResizeEffect(w, oldSize, new Point(x, y),
				duration, movement, onStop, onCancel);
		runner.runEffect(effect);
	}

	Point src, dest, diff;
	Control control = null;

	public ResizeEffect(Control control, Point src, Point dest,
			long lengthMilli, IMovement movement, Runnable onStop,
			Runnable onCancel) {
		super(lengthMilli, movement, onStop, onCancel);

		this.src = src;
		this.dest = dest;
		this.diff = new Point(dest.x - src.x, dest.y - src.y);

		easingFunction.init(0, 1, (int) lengthMilli);

		this.control = control;
	}

	public void applyEffect(final long currentTime) {
		if (!control.isDisposed()) {
			control.setSize((int) (src.x + diff.x
					* easingFunction.getValue((int) currentTime)),
					(int) (src.y + diff.y
							* easingFunction.getValue((int) currentTime)));
		}
	}

}