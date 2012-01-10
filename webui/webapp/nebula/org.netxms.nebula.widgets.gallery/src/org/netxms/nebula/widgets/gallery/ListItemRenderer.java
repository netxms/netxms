/*******************************************************************************
 * Copyright (c) 2006-2007 Nicolas Richeton.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors :
 *    Nicolas Richeton (nicolas.richeton@gmail.com) - initial API and implementation
 *******************************************************************************/
package org.netxms.nebula.widgets.gallery;

import java.util.ArrayList;
import java.util.Iterator;

import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Display;

/**
 * <p>
 * Alternate item renderer for the Gallery widget using a list style. Supports
 * multi-line text, image, drop shadows and decorators.
 * </p>
 * <p>
 * Best with bigger width than height.
 * </p>
 * <p>
 * Decorator images can be set with {@link GalleryItem#setData(String, Object)}
 * by using the following keys :
 * </p>
 * <ul>
 * <li>org.netxms.nebula.widget.gallery.bottomLeftOverlay</li>
 * <li>org.netxms.nebula.widget.gallery.bottomRightOverlay</li>
 * <li>org.netxms.nebula.widget.gallery.topLeftOverlay</li>
 * <li>org.netxms.nebula.widget.gallery.topRightOverlay</li>
 *</ul>
 *<p>
 * Supported types are org.eclipse.swt.Image for one single decorator and
 * org.eclipse.swt.Image[] for multiple decorators.
 * </p>
 * <p>
 * NOTE: THIS WIDGET AND ITS API ARE STILL UNDER DEVELOPMENT.
 * </p>
 * 
 * @author Nicolas Richeton (nicolas.richeton@gmail.com)
 * 
 */

public class ListItemRenderer extends AbstractGalleryItemRenderer {

	protected ArrayList dropShadowsColors = new ArrayList();

	boolean dropShadows = false;

	int dropShadowsSize = 5;

	int dropShadowsAlphaStep = 20;

	Color selectionBackgroundColor;

	Color selectionForegroundColor;

	Color foregroundColor, backgroundColor;

	Color descriptionColor;

	Font textFont = null;

	Font descriptionFont = null;

	boolean showLabels = true;

	boolean showRoundedSelectionCorners = true;

	int selectionRadius = 15;

	public boolean isShowLabels() {
		return showLabels;
	}

	public void setShowLabels(boolean showLabels) {
		this.showLabels = showLabels;
	}

	public ListItemRenderer() {
		super();

		// Set defaults
		foregroundColor = Display.getDefault().getSystemColor(
				SWT.COLOR_LIST_FOREGROUND);
		backgroundColor = Display.getDefault().getSystemColor(
				SWT.COLOR_LIST_BACKGROUND);
		selectionBackgroundColor = Display.getDefault().getSystemColor(
				SWT.COLOR_LIST_SELECTION);
		selectionForegroundColor = Display.getDefault().getSystemColor(
				SWT.COLOR_LIST_FOREGROUND);
		descriptionColor = Display.getDefault().getSystemColor(
				SWT.COLOR_DARK_GRAY);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.netxms.nebula.widgets.gallery.AbstractGalleryItemRenderer#preDraw
	 * (org.eclipse.swt.graphics.GC)
	 */
	public void preDraw(GC gc) {
		super.preDraw(gc);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.netxms.nebula.widgets.gallery.AbstractGalleryItemRenderer#draw(org
	 * .eclipse.swt.graphics.GC, org.netxms.nebula.widgets.gallery.GalleryItem,
	 * int, int, int, int, int)
	 */
	public void draw(GC gc, GalleryItem item, int index, int x, int y,
			int width, int height) {

		Image itemImage = item.getImage();
		Color itemBackgroundColor = item.getBackground();
		Color itemForegroundColor = item.getForeground();

		int useableHeight = height;

		int imageWidth = 0;
		int imageHeight = 0;
		int xShift = 0;
		int yShift = 0;
		Point size = null;

		if (itemImage != null) {
			Rectangle itemImageBounds = itemImage.getBounds();
			imageWidth = itemImageBounds.width;
			imageHeight = itemImageBounds.height;

			size = RendererHelper.getBestSize(imageWidth, imageHeight,
					useableHeight - 4 - this.dropShadowsSize, useableHeight - 4
							- this.dropShadowsSize);

			xShift = ((useableHeight - size.x) >> 1) + 2;
			yShift = (useableHeight - size.y) >> 1;

			if (dropShadows) {
				Color c = null;
				for (int i = this.dropShadowsSize - 1; i >= 0; i--) {
					c = (Color) dropShadowsColors.get(i);
					gc.setForeground(c);

					gc.drawLine(x + useableHeight + i - xShift - 1, y
							+ dropShadowsSize + yShift, x + useableHeight + i
							- xShift - 1, y + useableHeight + i - yShift);
					gc.drawLine(x + xShift + dropShadowsSize, y + useableHeight
							+ i - yShift - 1, x + useableHeight + i - xShift, y
							- 1 + useableHeight + i - yShift);
				}
			}
		}

		// Draw selection background (rounded rectangles)
		if (selected
				|| RendererHelper.isColorsEquals(itemBackgroundColor, gallery
						.getBackground())) {
			if (selected) {
				gc.setBackground(selectionBackgroundColor);
				gc.setForeground(selectionBackgroundColor);
			} else if (itemBackgroundColor != null) {
				gc.setBackground(itemBackgroundColor);
			}

			if (showRoundedSelectionCorners)
				gc.fillRoundRectangle(x, y, width, useableHeight,
						selectionRadius, selectionRadius);
			else
				gc.fillRectangle(x, y, width, useableHeight);
		}

		if (itemImage != null && size != null) {
			if (size.x > 0 && size.y > 0) {
				gc.drawImage(itemImage, 0, 0, imageWidth, imageHeight, x
						+ xShift, y + yShift, size.x, size.y);
				drawAllOverlays(gc, item, x, y, size, xShift, yShift);
			}
		}

		if (item.getText() != null && !EMPTY_STRING.equals(item.getText())
				&& showLabels) {

			// Calculate font height (text and description)
			gc.setFont(textFont);
			String text = RendererHelper.createLabel(item.getText(), gc, width
					- useableHeight - 10);
			int textFontHeight = gc.getFontMetrics().getHeight();

			String description = null;
			int descriptionFontHeight = 0;
			if (item.getText(1) != null
					&& !EMPTY_STRING.equals(item.getText(1))) {
				gc.setFont(descriptionFont);
				description = RendererHelper.createLabel(item.getText(1), gc,
						width - useableHeight - 10);
				descriptionFontHeight = gc.getFontMetrics().getHeight();
			}

			boolean displayText = false;
			boolean displayDescription = false;
			int remainingHeight = height - 2 - textFontHeight;
			if (remainingHeight > 0)
				displayText = true;
			remainingHeight -= descriptionFontHeight;
			if (remainingHeight > 0)
				displayDescription = true;

			// Background color
			gc.setBackground(selected ? selectionBackgroundColor
					: backgroundColor);

			// Draw text
			if (displayText) {
				int transY = (height - textFontHeight - 2);
				if (displayDescription)
					transY -= descriptionFontHeight;
				transY = transY >> 1;

				if (selected) {
					gc.setForeground(this.selectionForegroundColor);
				} else if (itemForegroundColor != null) {
					gc.setForeground(itemForegroundColor);
				} else {
					gc.setForeground(this.foregroundColor);
				}

				gc.setFont(textFont);
				gc.drawText(text, x + useableHeight + 5, y + transY, true);
			}
			// Draw description
			if (description != null && displayDescription) {
				gc.setForeground(this.descriptionColor);
				gc.setFont(descriptionFont);
				gc.drawText(description, x + useableHeight + 5,
						y
								+ ((height - descriptionFontHeight
										- textFontHeight - 2) >> 1)
								+ textFontHeight + 1, true);
			}
		}
	}

	public void setDropShadowsSize(int dropShadowsSize) {
		this.dropShadowsSize = dropShadowsSize;
		this.dropShadowsAlphaStep = (dropShadowsSize == 0) ? 0
				: (200 / dropShadowsSize);

		freeDropShadowsColors();
		createColors();
		// TODO: force redraw

	}

	private void createColors() {
		if (dropShadowsSize > 0) {
			int step = 125 / dropShadowsSize;
			// Create new colors
			for (int i = dropShadowsSize - 1; i >= 0; i--) {
				int value = 255 - i * step;
				Color c = new Color(Display.getDefault(), value, value, value);
				dropShadowsColors.add(c);
			}
		}
	}

	private void freeDropShadowsColors() {
		// Free colors :
		{
			Iterator i = this.dropShadowsColors.iterator();
			while (i.hasNext()) {
				Color c = (Color) i.next();
				if (c != null && !c.isDisposed())
					c.dispose();
			}
		}
	}

	public boolean isDropShadows() {
		return dropShadows;
	}

	public void setDropShadows(boolean dropShadows) {
		this.dropShadows = dropShadows;
	}

	public int getDropShadowsSize() {
		return dropShadowsSize;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.netxms.nebula.widgets.gallery.AbstractGalleryItemRenderer#dispose()
	 */
	public void dispose() {
		freeDropShadowsColors();
	}

	public Color getSelectionBackgroundColor() {
		return selectionBackgroundColor;
	}

	public void setSelectionBackgroundColor(Color selectionColor) {
		this.selectionBackgroundColor = selectionColor;
	}

	public Color getForegroundColor() {
		return foregroundColor;
	}

	public void setForegroundColor(Color foregroundColor) {
		this.foregroundColor = foregroundColor;
	}

	public Color getBackgroundColor() {
		return backgroundColor;
	}

	public void setBackgroundColor(Color backgroundColor) {
		this.backgroundColor = backgroundColor;
	}

	public Color getDescriptionColor() {
		return descriptionColor;
	}

	public void setDescriptionColor(Color descriptionColor) {
		this.descriptionColor = descriptionColor;
	}

	public Color getSelectionForegroundColor() {
		return selectionForegroundColor;
	}

	public void setSelectionForegroundColor(Color selectionForegroundColor) {
		this.selectionForegroundColor = selectionForegroundColor;
	}

	/**
	 * Returns the font used for drawing item label or <tt>null</tt> if system
	 * font is used.
	 * 
	 * @deprecated Use {@link Gallery#setFont(Font)} or
	 *             {@link GalleryItem#setFont(Font)} instead.
	 * @return the font
	 */
	public Font getTextFont() {
		return textFont;
	}

	/**
	 * Set the font for drawing item label or <tt>null</tt> to use system font.
	 * 
	 * @deprecated Use {@link Gallery#setFont(Font)} or
	 *             {@link GalleryItem#setFont(Font)} instead.
	 * @param font
	 *            the font to set
	 */
	public void setTextFont(Font textFont) {
		this.textFont = textFont;
	}

	/**
	 * Returns the font used for drawing item description or <tt>null</tt> if
	 * system font is used.
	 * 
	 * @return the font
	 */
	public Font getDescriptionFont() {
		return descriptionFont;
	}

	/**
	 * Set the font for drawing item description or <tt>null</tt> to use system
	 * font.
	 * 
	 * @param font
	 *            the font to set
	 */
	public void setDescriptionFont(Font descriptionFont) {
		this.descriptionFont = descriptionFont;
	}

	public boolean isShowRoundedSelectionCorners() {
		return this.showRoundedSelectionCorners;
	}

	public void setShowRoundedSelectionCorners(
			boolean showRoundedSelectionCorners) {
		this.showRoundedSelectionCorners = showRoundedSelectionCorners;
	}
}
