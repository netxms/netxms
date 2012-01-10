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

import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.ScrollBar;
import org.netxms.nebula.animation.movement.IMovement;

/**
 * Scrolls a control.
 * 
 * @author Nicolas Richeton
 * 
 */
public class MoveScrollBarEffect extends AbstractEffect {

	int start, end, step, current;

	ScrollBar scrollBar = null;

	public MoveScrollBarEffect(ScrollBar scrollBar, int start, int end,
			long lengthMilli, IMovement movement, Runnable onStop,
			Runnable onCancel) {
		super(lengthMilli, movement, onStop, onCancel);

		this.start = start;
		this.end = end;
		step = end - start;

		easingFunction.init(0, 1, (int) lengthMilli);

		this.scrollBar = scrollBar;
		current = start;
	}

	public void applyEffect(final long currentTime) {
		current = (int) (start + step
				* easingFunction.getValue((int) currentTime));

		if (!scrollBar.isDisposed()) {
			scrollBar.setSelection(current);
			Event event = new Event();
			event.detail = step < 0 ? SWT.PAGE_UP : SWT.PAGE_DOWN;
			event.data = this;
			event.display = scrollBar.getDisplay();
			event.widget = scrollBar;
			event.doit = true;

			scrollBar.notifyListeners(SWT.Selection, event);
		}
	}

	public int getStart() {
		return start;
	}

	public int getEnd() {
		return end;
	}

	public int getCurrent() {
		return current;
	}

}