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

package org.netxms.nebula.animation;

import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Listener;
import org.eclipse.swt.widgets.ScrollBar;
import org.eclipse.swt.widgets.Scrollable;
import org.netxms.nebula.animation.effects.IEffect;
import org.netxms.nebula.animation.effects.MoveScrollBarEffect;
import org.netxms.nebula.animation.movement.IMovement;

/**
 * <p>
 * Allows to replace the default scrolling behavior by an animation effect.
 * </p>
 * 
 * <p>
 * Compatible with :
 * </p>
 * <ul>
 * <li>Shell</li>
 * <li>StyledText</li>
 * <li>Canvas</li>
 * <li>Gallery</li>
 * </ul>
 * 
 * 
 * @author Nicolas Richeton
 * 
 */
public class ScrollingSmoother {

	Scrollable component;

	ScrollBar verticalScrollBar;

	ScrollBar horizontalScrollBar;

	IMovement movement = null;

	int duration = 2000;

	AnimationRunner animationRunner = new AnimationRunner();

	/**
	 * Create a Scrolling Smoother instance over a scrollable widget. This
	 * effect can then be activated using
	 * {@link ScrollingSmoother#smoothControl(boolean)}.
	 * 
	 * @see ScrollingSmoother#smoothControl(boolean)
	 * 
	 * @param c2
	 * @param movement
	 */
	public ScrollingSmoother(final Scrollable c2, IMovement movement) {
		this.component = c2;
		verticalScrollBar = c2.getVerticalBar();
		horizontalScrollBar = c2.getHorizontalBar();
		this.movement = movement;
	}

	/**
	 * Create a Scrolling Smoother instance over a scrollable widget. This
	 * effect can then be activated using
	 * {@link ScrollingSmoother#smoothControl(boolean)}.
	 * 
	 * @see ScrollingSmoother#smoothControl(boolean)
	 * 
	 * @param c2
	 * @param movement
	 * @param duration
	 */
	public ScrollingSmoother(final Scrollable c2, IMovement movement,
			int duration) {
		this(c2, movement);
		this.duration = duration;
	}

	/**
	 * Get current effect duration (ms).
	 * 
	 * @return
	 */
	public int getDuration() {
		return duration;
	}

	/**
	 * Set effect duration (ms).
	 * 
	 * @param duration
	 */
	public void setDuration(int duration) {
		this.duration = duration;
	}

	/**
	 * Set the FPS (frame per second) to use with the animator.
	 * 
	 * @param fps
	 */
	public void setFPS(int fps) {
		animationRunner = new AnimationRunner(fps);
	}

	protected ScrollBar getScrollbar(Event event) {
		ScrollBar result = verticalScrollBar;

		if (result == null) {
			result = horizontalScrollBar;
		}

		return result;
	}

	Listener mouseWheelListener = new Listener() {

		public void handleEvent(Event event) {
			// Remove standard behavior
			event.doit = false;

			// Get scrollbar on which the event occurred.
			ScrollBar currentScrollBar = getScrollbar(event);

			int start = currentScrollBar.getSelection();
			int end = start;

			// If an effect is currently running, get the current and target
			// values.
			IEffect current = animationRunner.getEffect();
			if (current instanceof MoveScrollBarEffect) {
				MoveScrollBarEffect mseffect = (MoveScrollBarEffect) current;
				start = mseffect.getCurrent();
				end = mseffect.getEnd();
			}

			//end -= event.count * currentScrollBar.getIncrement();
			end -= event.count;

			if (end > currentScrollBar.getMaximum()
					- currentScrollBar.getThumb()) {
				end = currentScrollBar.getMaximum()
						- currentScrollBar.getThumb();
			}

			if (end < currentScrollBar.getMinimum()) {
				end = currentScrollBar.getMinimum();
			}

			animationRunner.runEffect(new MoveScrollBarEffect(currentScrollBar,
					start, end, duration, movement, null, null));

		}
	};

	/**
	 * Enable or disable scrolling effect.
	 * 
	 * @param enable
	 *            true or false.
	 */
	public void smoothControl(boolean enable) {
		/*
		if (enable) {
			component.addListener(SWT.MouseWheel, mouseWheelListener);

			if (verticalScrollBar != null)
				verticalScrollBar
						.addSelectionListener(cancelEffectIfUserSelection);

			if (horizontalScrollBar != null)
				horizontalScrollBar
						.addSelectionListener(cancelEffectIfUserSelection);

		} else {
			component.removeListener(SWT.MouseWheel, mouseWheelListener);

			if (verticalScrollBar != null)
				verticalScrollBar
						.removeSelectionListener(cancelEffectIfUserSelection);

			if (horizontalScrollBar != null)
				horizontalScrollBar
						.removeSelectionListener(cancelEffectIfUserSelection);

		}
		*/
	}

	SelectionListener cancelEffectIfUserSelection = new SelectionListener() {

		public void widgetDefaultSelected(SelectionEvent e) {
			widgetSelected(e);
		}

		public void widgetSelected(SelectionEvent e) {
			// data contains an instance of MoveScrollBarEffect if the selection
			// was generated by this effect. Otherwise this is an user
			// selection.
			if (!(e.data instanceof MoveScrollBarEffect)) {
				animationRunner.cancel();
			}
		}
	};

}
