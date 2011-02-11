/*******************************************************************************
 * Copyright (c) 2006-2008 Nicolas Richeton.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors :
 *    Nicolas Richeton (nicolas.richeton@gmail.com) - initial API and implementation
 *    Richard Michalsky - bug 195439
 *******************************************************************************/
package org.netxms.nebula.widgets.gallery;

import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Point;

/**
 * <p>
 * Utility methods for Gallery item and group renderers
 * </p>
 * 
 * <p>
 * NOTE: THIS WIDGET AND ITS API ARE STILL UNDER DEVELOPMENT.
 * </p>
 * 
 * @author Nicolas Richeton (nicolas.richeton@gmail.com)
 * @contributor Richard Michalsky
 * 
 */
public class RendererHelper {
	private static final String ELLIPSIS = "..."; //$NON-NLS-1$

	/**
	 * Shorten the given text <code>text</code> so that its length doesn't
	 * exceed the given width. The default implementation replaces characters in
	 * the center of the original string with an ellipsis ("..."). Override if
	 * you need a different strategy.
	 * 
	 * Note: Code originally from org.eclipse.cwt.CLabel
	 * 
	 * @param gc
	 *            the gc to use for text measurement
	 * @param t
	 *            the text to shorten
	 * @param width
	 *            the width to shorten the text to, in pixels
	 * @return the shortened text
	 */
	public static String createLabel(String text, GC gc, int width) {

		if (text == null)
			return null;

		final int extent = gc.textExtent(text).x;

		if (extent > width) {
			final int w = gc.textExtent(ELLIPSIS).x;
			if (width <= w) {
				return text;
			}
			final int l = text.length();
			int max = l / 2;
			int min = 0;
			int mid = (max + min) / 2 - 1;
			if (mid <= 0) {
				return text;
			}
			while (min < mid && mid < max) {
				final String s1 = text.substring(0, mid);
				final String s2 = text.substring(l - mid, l);
				final int l1 = gc.textExtent(s1).x;
				final int l2 = gc.textExtent(s2).x;
				if (l1 + w + l2 > width) {
					max = mid;
					mid = (max + min) / 2;
				} else if (l1 + w + l2 < width) {
					min = mid;
					mid = (max + min) / 2;
				} else {
					min = max;
				}
			}
			if (mid == 0) {

				return text;
			}
			String result = text.substring(0, mid) + ELLIPSIS
					+ text.substring(l - mid, l);

			return result;
		}

		return text;

	}

	/**
	 * Get best-fit size for an image drawn in an area of maxX, maxY
	 * 
	 * @param originalX
	 * @param originalY
	 * @param maxX
	 * @param maxY
	 * @return
	 */
	public static Point getBestSize(int originalX, int originalY, int maxX,
			int maxY) {
		double widthRatio = (double) originalX / (double) maxX;
		double heightRatio = (double) originalY / (double) maxY;

		double bestRatio = widthRatio > heightRatio ? widthRatio : heightRatio;

		int newWidth = (int) (originalX / bestRatio);
		int newHeight = (int) (originalY / bestRatio);

		return new Point(newWidth, newHeight);
	}

	/**
	 * Return both width and height offsets for an image to be centered in a
	 * given area.
	 * 
	 * @param imageWidth
	 * @param imageHeight
	 * @param areaWidth
	 * @param areaHeight
	 * @return
	 */
	public static Point getImageOffset(int imageWidth, int imageHeight,
			int areaWidth, int areaHeight) {
		return new Point(getShift(areaWidth, imageWidth), getShift(areaHeight,
				imageHeight));
	}

	/**
	 * Return the offset to use in order to center an object in a given area.
	 * 
	 * @param totalSize
	 * @param size
	 * @return
	 */
	public static int getShift(int totalSize, int size) {
		int xShift = totalSize - size;
		if (xShift < 0)
			xShift = 0;
		xShift = xShift >> 1;
		return xShift;
	}

	/**
	 * Checks if two colors are equals by comparing their RGB values.
	 * This method is null-proof.
	 * 
	 * @param galleryColor
	 *            First color
	 * @param itemColor
	 *            Second color.
	 * @return true if same object or RGB values are equals.
	 */
	public static boolean isColorsEquals(Color galleryColor, Color itemColor) {

		// Default case
		if (galleryColor == itemColor) {
			return true;
		}

		if (galleryColor == null || itemColor == null) {
			return false;
		}

		if (galleryColor.getRGB().equals(itemColor.getRGB())) {
			return true;
		}

		return false;
	}
}
