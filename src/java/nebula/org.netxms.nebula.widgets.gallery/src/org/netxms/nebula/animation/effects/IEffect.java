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

/**
 * All animation effects must implement this interface.
 * <p>
 * Note : an effect should not do initialization in constructor, but at the
 * first call to doEffect(). For instance, a move effect should not get the
 * initial position of an object in the constructor, because the object may have
 * moved between creation and effect start.
 * </p>
 * 
 * @author Nicolas Richeton
 */
public interface IEffect {

	/**
	 * Set the effect as done and run the cancel runnable.
	 */
	public void cancel();

	/**
	 * Apply effect to the target according to the given time.
	 * 
	 * @param time
	 *            - Current time in ms. This value may be larger than the effect
	 *            length.
	 */
	public void doEffect(long time);

	/**
	 * Get effect length
	 * 
	 * @return length (ms)
	 */
	public long getLength();

	/**
	 * @return true if the effect as already reached its end.
	 */
	public boolean isDone();
}
