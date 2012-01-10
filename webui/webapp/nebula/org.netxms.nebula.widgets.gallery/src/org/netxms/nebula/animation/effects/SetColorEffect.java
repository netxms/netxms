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

import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.widgets.Display;
import org.netxms.nebula.animation.movement.IMovement;

/**
 * Progressively changes the color of an object.
 * 
 * @author Nicolas Richeton (nicolas.richeton@gmail.com)
 * 
 */
public class SetColorEffect extends AbstractEffect {

	/**
	 * Objects on which the SetColorEffect is applied must implements this
	 * interface. This allows to use this effect on different widgets and select
	 * the right color to change (foreground, background).
	 * 
	 * @author Nicolas Richeton (nicolas.richeton@gmail.com)
	 * 
	 */
	public interface IColoredObject {
		/**
		 * Set the color of this object
		 * 
		 * @param c
		 */
		void setColor(Color c);

		/**
		 * Get the current color of this object
		 * 
		 * @return
		 */
		Color getColor();
	}

	Color src;
	Color dest;
	int diffR, diffG, diffB;

	IColoredObject control = null;

	/**
	 * <p>
	 * Create a new effect on object control.
	 * </p>
	 * 
	 * <p>
	 * Source and destination color will not be disposed during or after the
	 * animation. All other temporary colors created by this effect will be
	 * disposed automatically.
	 * </p>
	 * 
	 * @param control
	 * @param src
	 * @param dest
	 * @param lengthMilli
	 * @param movement
	 * @param onStop
	 *            can be a Runnable or null
	 * @param onCancel
	 *            can be a Runnable or null
	 */
	public SetColorEffect(IColoredObject control, Color src, Color dest,
			long lengthMilli, IMovement movement, Runnable onStop,
			Runnable onCancel) {
		super(lengthMilli, movement, onStop, onCancel);
		this.src = src;
		this.dest = dest;
		this.diffR = dest.getRed() - src.getRed();
		this.diffG = dest.getGreen() - src.getGreen();
		this.diffB = dest.getBlue() - src.getBlue();
		this.control = control;

		easingFunction.init(0, 1, (int) lengthMilli);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.netxms.nebula.animation.effects.AbstractEffect#applyEffect(long)
	 */
	public void applyEffect(final long currentTime) {

		Color currentColor = control.getColor();

		// Get the next color values
		int nextRed = (int) (src.getRed() + diffR
				* easingFunction.getValue(currentTime));
		int nextGreen = (int) (src.getGreen() + diffG
				* easingFunction.getValue(currentTime));
		int nextBlue = (int) (src.getBlue() + diffB
				* easingFunction.getValue(currentTime));

		RGB nextRGB = new RGB(nextRed, nextGreen, nextBlue);

		if (currentColor == null || !nextRGB.equals(currentColor.getRGB())) {

			Color nextColor = new Color(Display.getCurrent(), nextRed,
					nextGreen, nextBlue);

			// If this is the destination color, dispose the newly created color
			// and use the destination one instead.
			if (dest.getRGB().equals(nextColor.getRGB())) {
				nextColor.dispose();
				nextColor = dest;
			}

			control.setColor(nextColor);

			// If the previous color is not the source or destination one,
			// dispose it.
			if (currentColor != null && !currentColor.isDisposed()) {
				if (dest != currentColor && src != currentColor) {
					currentColor.dispose();
				}
			}

		}

	}
}
