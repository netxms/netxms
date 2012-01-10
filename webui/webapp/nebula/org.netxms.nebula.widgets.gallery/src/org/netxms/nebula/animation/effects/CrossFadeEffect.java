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

import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.netxms.nebula.animation.movement.IMovement;

/**
 * Cross fade images in a target object. The target must implement
 * {@link IImageObject}.
 * 
 * @author Nicolas Richeton
 */
public class CrossFadeEffect extends AbstractEffect {
	public interface IImageObject {
		/**
		 * Get the current image of this object
		 * 
		 * @return
		 */
		Image getImage();

		/**
		 * Set the image of this object
		 * 
		 * @param c
		 */
		void setImage(Image i);
	}

	Image buffer = null;
	double easingValue;
	GC gc = null;
	Image image1 = null;
	Image image2 = null;
	IImageObject obj = null;

	/**
	 * Cross fade from image1 to image2 on obj.
	 * 
	 * @param obj
	 * @param image1
	 * @param image2
	 * @param lengthMilli
	 * @param movement
	 */
	public CrossFadeEffect(IImageObject obj, Image image1, Image image2,
			long lengthMilli, IMovement movement) {
		this(obj, image1, image2, lengthMilli, movement, null, null);
	}

	/**
	 * Cross fade from image1 to image2 on obj.
	 * 
	 * @param obj
	 * @param image1
	 * @param image2
	 * @param lengthMilli
	 * @param movement
	 * @param onStop
	 */
	public CrossFadeEffect(IImageObject obj, Image image1, Image image2,
			long lengthMilli, IMovement movement, Runnable onStop) {
		this(obj, image1, image2, lengthMilli, movement, onStop, null);
	}

	/**
	 * Cross fade from image1 to image2 on obj.
	 * 
	 * @param obj
	 * @param image1
	 * @param image2
	 * @param lengthMilli
	 * @param movement
	 * @param onStop
	 * @param onCancel
	 */
	public CrossFadeEffect(IImageObject obj, Image image1, Image image2,
			long lengthMilli, IMovement movement, Runnable onStop,
			Runnable onCancel) {
		super(lengthMilli, movement, onStop, onCancel);
		this.obj = obj;
		this.image1 = image1;
		this.image2 = image2;

		if (!image1.getBounds().equals(image2.getBounds())) {
			throw new IllegalArgumentException(
					"Both image must have the same dimensions");
		}

		easingFunction.init(0, 1, (int) lengthMilli);

		buffer = new Image(image1.getDevice(), image1.getBounds().width, image1
				.getBounds().height);
		gc = new GC(buffer);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.netxms.nebula.animation.effects.AbstractEffect#applyEffect(long)
	 */
	public void applyEffect(long currentTime) {
		easingValue = easingFunction.getValue(currentTime);

		if (easingValue == 0) {
			obj.setImage(image1);
		} else if (easingValue == 1) {
			obj.setImage(image2);
		} else {
			gc.setAlpha(255);
			gc.drawImage(image1, 0, 0);
			gc.setAlpha((int) (255 * easingValue));
			gc.drawImage(image2, 0, 0);
			obj.setImage(buffer);
		}
	}

	/**
	 * Clear resources.
	 */
	protected void cleanup() {
		gc.dispose();
		buffer.dispose();
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.netxms.nebula.animation.effects.AbstractEffect#doCancel()
	 */
	protected void doCancel() {
		obj.setImage(image2);
		super.doCancel();
		cleanup();
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.netxms.nebula.animation.effects.AbstractEffect#doStop()
	 */
	protected void doStop() {
		super.doStop();
		cleanup();
	}
}
