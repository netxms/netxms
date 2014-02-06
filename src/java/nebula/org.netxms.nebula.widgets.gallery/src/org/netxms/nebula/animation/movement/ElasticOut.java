/*******************************************************************************
 * Port of Robert Penner's easing equations for Nebula animation package.
 * Copyright (c) 2008-2009 Nicolas Richeton.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 *******************************************************************************/

/*********************************************************************************
 * TERMS OF USE - EASING EQUATIONS 
 * 
 * Open source under the BSD License.
 * 
 * Copyright (c) 2001 Robert Penner
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the author nor the names of contributors may be 
 *       used to endorse or promote products derived from this software without
 *       specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
 ********************************************************************************/

package org.netxms.nebula.animation.movement;

/**
 * 
 * Behave like an elastic at the end.
 * 
 * <p>
 * Warning : this equation will temporary go further the target value.
 * </p>
 * 
 * @author Nicolas Richeton
 * 
 */
public class ElasticOut extends AbstractMovement {

	Double a = null, p = null;

	public ElasticOut(double a, double p) {
		super();
		this.a = new Double(a);
		this.p = new Double(p);
	}

	public ElasticOut() {
		super();
	}

	public double getValue(double step) {
		// Conversion from Robert Penner's action scripts
		//
		// t : time -> step
		// b: min -> min
		// c : total increment -> max - min
		// d: duration -> duration

		double c = max - min;
		double s;

		if (step == 0) {
			return min;
		}

		step = step / duration;
		if (Double.compare(step, 1.0) == 0)
			return min + c;

		if (p == null)
			p = new Double(duration * .3);

		if (a == null || a.doubleValue() < Math.abs(c)) {
			a = new Double(c);
			s = p.doubleValue() / 4d;
		}

		else
			s = p.doubleValue() / (2d * Math.PI)
					* Math.asin(c / a.doubleValue());

		return (a.doubleValue()
				* Math.pow(2d, -10d * step)
				* Math.sin((step * duration - s) * (2d * Math.PI)
						/ p.doubleValue()) + c + min);
	}

}
