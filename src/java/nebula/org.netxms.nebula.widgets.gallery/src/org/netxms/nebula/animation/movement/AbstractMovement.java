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

package org.netxms.nebula.animation.movement;

/**
 * Abstract implementation of IMovement.
 * 
 * @author Nicolas Richeton
 */
public abstract class AbstractMovement implements IMovement {

	protected double min;
	protected double max;
	protected double duration;

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.netxms.nebula.animation.movement.IMovement#getValue(double)
	 */
	public abstract double getValue(double step);

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.netxms.nebula.animation.movement.IMovement#init(double, double,
	 * int)
	 */
	public void init(double minValue, double maxValue, int steps) {
		this.min = minValue;
		this.max = maxValue;
		this.duration = steps;
	}

}
