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
 * This movement goes from 0 to 0 with intermediate values between -amplitude
 * and amplitude an decreasing with time.
 * 
 * This is not an easing equation.
 * 
 * @author Nicolas Richeton
 * 
 */
public class SinusDecreaseVariation extends AbstractMovement {

	int variations = 1;
	double amplitude;

	public SinusDecreaseVariation(int nb, double amplitude) {
		super();
		variations = nb;
		this.amplitude = amplitude;
	}

	public double getValue(double step) {
		return amplitude * (1 - step / duration)
				* Math.sin(step / duration * Math.PI * (double) variations);
	}

}
