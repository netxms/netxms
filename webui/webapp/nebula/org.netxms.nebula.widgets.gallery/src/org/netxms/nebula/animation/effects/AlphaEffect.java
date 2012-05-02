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

import org.eclipse.swt.events.ShellEvent;
import org.eclipse.swt.events.ShellListener;
import org.eclipse.swt.widgets.Shell;
import org.netxms.nebula.animation.AnimationRunner;
import org.netxms.nebula.animation.movement.IMovement;

/**
 * Set the alpha value of a Shell
 * 
 * @author Nicolas Richeton
 * 
 */
public class AlphaEffect extends AbstractEffect {

	/**
	 * @deprecated
	 * @param w
	 * @param alpha
	 * @param duration
	 * @param movement
	 * @param onStop
	 * @param onCancel
	 */
	public static void setAlpha(AnimationRunner runner, Shell w, int alpha,
			int duration, IMovement movement, Runnable onStop, Runnable onCancel) {
		AlphaEffect effect = new AlphaEffect(w, w.getAlpha(), alpha, duration,
				movement, onStop, onCancel);
		runner.runEffect(effect);
	}

	/**
	 * Add a listener that will fade the window when it get closed.
	 * 
	 * @param shell
	 * @param duration
	 * @param easing
	 * @deprecated Use {@link #fadeOnClose(Shell,int,IMovement,AnimationRunner)}
	 *             instead
	 */
	public static void fadeOnClose(final Shell shell, final int duration,
			final IMovement easing) {
		fadeOnClose(shell, duration, easing, null);
	}

	/**
	 * Add a listener that will fade the window when it get closed.
	 * 
	 * @param shell
	 * @param duration
	 * @param easing
	 * @param runner
	 *            : The AnimationRunner to use, or null
	 * 
	 */
	public static void fadeOnClose(final Shell shell, final int duration,
			final IMovement easing, AnimationRunner runner) {

		final AnimationRunner useRunner;
		if (runner != null) {
			useRunner = runner;
		} else {
			useRunner = new AnimationRunner();
		}

		final Runnable closeListener = new Runnable() {
			public void run() {
				shell.dispose();
			}
		};

		shell.addShellListener(new ShellListener() {
			private static final long serialVersionUID = 1L;

			public void shellDeactivated(ShellEvent e) {
				// Do nothing
			}

			public void shellClosed(ShellEvent e) {
				e.doit = false;
				setAlpha(useRunner, shell, 0, duration, easing, closeListener,
						null);
			}

			public void shellActivated(ShellEvent e) {
				// Do nothing
			}

		});

	}

	int start, end, step;

	Shell shell = null;

	public AlphaEffect(Shell shell, int start, int end, long lengthMilli,
			IMovement movement, Runnable onStop, Runnable onCancel) {
		super(lengthMilli, movement, onStop, onCancel);

		this.start = start;
		this.end = end;
		step = end - start;
		this.shell = shell;
		easingFunction.init(0, 1, (int) lengthMilli);

	}

	public void applyEffect(final long currentTime) {
		if (shell.isDisposed())
			return;

		shell.setAlpha((int) (start + step
				* easingFunction.getValue((int) currentTime)));
	}

}