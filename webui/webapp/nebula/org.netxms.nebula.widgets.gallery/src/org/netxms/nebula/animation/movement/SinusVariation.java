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
 * and amplitude.
 * 
 * This is not an easing equation.
 * 
 * @author Nicolas Richeton
 * 
 */
public class SinusVariation extends AbstractMovement {

	int variations = 1;
	double amplitude;

	public SinusVariation(int nb, double amplitude) {
		super();
		variations = nb;
		this.amplitude = amplitude;
	}

	public double getValue(double step) {
		return amplitude
				* Math.sin(step / duration * Math.PI * (double) variations);
	}

}
