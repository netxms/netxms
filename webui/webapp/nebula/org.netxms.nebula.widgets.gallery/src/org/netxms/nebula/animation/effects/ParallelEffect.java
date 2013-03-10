/*******************************************************************************
 * Copyright (c) 2009 Nicolas Richeton.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors :
 *    Nicolas Richeton (nicolas.richeton@gmail.com) - initial API and implementation
 *******************************************************************************/
package org.netxms.nebula.animation.effects;

import java.util.List;

/**
 * A wrapper for running effects in parallel.
 * 
 * @author Nicolas Richeton
 * 
 */
public class ParallelEffect implements IEffect {

	IEffect[] effects = null;
	long length = 0;
	Runnable onCancel;
	Runnable onStop;

	/**
	 * Wrap several effects and start them in parallel.
	 * 
	 * @param effects
	 */
	public ParallelEffect(IEffect[] effects) {
		this(effects, null, null);
	}

	/**
	 * Wrap several effects and start them in parallel.
	 * 
	 * @param effects
	 * @param onStop
	 * @param onCancel
	 */
	public ParallelEffect(IEffect[] effects, Runnable onStop, Runnable onCancel) {
		this.effects = effects;
		this.onCancel = onCancel;
		this.onStop = onStop;

		// Get total length
		if (effects != null) {
			IEffect e = null;
			for (int i = effects.length - 1; i >= 0; i--) {
				e = (IEffect) effects[i];
				if (e.getLength() > length) {
					length = e.getLength();
				}
			}
		}
	}

	/**
	 * Wrap several effects and start them in parallel.
	 * 
	 * @param effects
	 */
	public ParallelEffect(List<IEffect> effects) {
		this(effects, null, null);
	}

	/**
	 * Wrap several effects and start them in parallel.
	 * 
	 * @param effects
	 * @param onStop
	 * @param onCancel
	 */
	public ParallelEffect(List<IEffect> effects, Runnable onStop, Runnable onCancel) {
		this(effects.toArray(new IEffect[effects.size()]), onStop, onCancel);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.netxms.nebula.animation.effects.IEffect#cancel()
	 */
	public void cancel() {
		if (effects != null) {
			for (int i = effects.length - 1; i >= 0; i--) {
				effects[i].cancel();
			}
		}

		// Call cancel runnable
		if (onCancel != null) {
			onCancel.run();
		}
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.netxms.nebula.animation.effects.IEffect#doEffect(long)
	 */
	public void doEffect(long time) {
		if (effects != null) {
			for (int i = effects.length - 1; i >= 0; i--) {
				effects[i].doEffect(time);
			}
		}

		// Call stop runnable
		if (onStop != null && isDone()) {
			onStop.run();
		}
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.netxms.nebula.animation.effects.IEffect#getLength()
	 */
	public long getLength() {
		return length;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.netxms.nebula.animation.effects.IEffect#isDone()
	 */
	public boolean isDone() {

		if (effects != null) {
			// Ensure all effects are done.
			boolean done = true;
			for (int i = effects.length - 1; i >= 0; i--) {
				if (!effects[i].isDone()) {
					done = false;
				}
			}
			return done;
		}

		// No effects ? always done.
		return true;
	}

}
