/*******************************************************************************
 * Copyright (c) 2006-2010 Nicolas Richeton.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors :
 *    Nicolas Richeton (nicolas.richeton@gmail.com) - initial API and implementation
 *******************************************************************************/
package org.netxms.nebula.animation.effects;

/**
 * Run several effects one after another.
 * 
 * @author Nicolas Richeton
 * 
 */
public class SequenceEffect implements IEffect {
	int currentEffect = 0;
	IEffect[] effects = null;
	long length = 0;
	Runnable onCancel;
	Runnable onStop;
	long start = 0;

	/**
	 * Run several effects one after another.
	 * <p>
	 * Note :
	 * <ul>
	 * <li>onStop and onCancel runnables applies to the whole sequence effect.</li>
	 * <li>Each effect can have its own onStop and onCancel.</li>
	 * </ul>
	 * </p>
	 * 
	 * @param effects
	 * @param onStop
	 * @param onCancel
	 */
	public SequenceEffect(IEffect[] effects, Runnable onStop, Runnable onCancel) {
		this.effects = effects;
		this.onCancel = onCancel;
		this.onStop = onStop;

		if (effects != null) {
			IEffect e = null;
			for (int i = effects.length - 1; i >= 0; i--) {
				e = effects[i];
				length += e.getLength();
			}
		}
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.netxms.nebula.animation.effects.IEffect#cancel()
	 */
	public void cancel() {
		for (int i = currentEffect; i < effects.length; i++) {
			effects[currentEffect].cancel();
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
		if (currentEffect >= effects.length) {
			return;
		}

		effects[currentEffect].doEffect(time - start);
		if (effects[currentEffect].isDone()) {
			start += effects[currentEffect].getLength();
			currentEffect++;
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
		if (effects == null) {
			return true;
		}
		return effects[effects.length - 1].isDone();
	}

}
