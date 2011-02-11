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
 * Moves at a constant speed.
 * 
 * @author Nicolas Richeton
 */
public class LinearInOut extends AbstractMovement {

	double increment;

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.sharemedia.gui.viewers.impl.gl.IMovement#getValue(int)
	 */
	public double getValue(double step) {
		return min + increment * step;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.sharemedia.gui.viewers.impl.gl.IMovement#init(float, float, int)
	 */
	public void init(double min, double max, int steps) {
		increment = (max - min) / steps;
		super.init(min, max, steps);
	}

}
