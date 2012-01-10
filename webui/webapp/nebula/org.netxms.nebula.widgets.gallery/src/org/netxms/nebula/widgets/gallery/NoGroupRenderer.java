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

import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;

/**
 * <p>
 * Alternate group renderer for the Gallery widget. This group renderer does not
 * draw group titles. Only items are displayed. All groups are considered as
 * expanded.
 * </p>
 * <p>
 * The visual aspect is the same as the first version of the gallery widget.
 * 
 * </p>
 * <p>
 * NOTE: THIS WIDGET AND ITS API ARE STILL UNDER DEVELOPMENT.
 * </p>
 * 
 * 
 * @author Nicolas Richeton (nicolas.richeton@gmail.com)
 */
public class NoGroupRenderer extends AbstractGridGroupRenderer {

	protected static int OFFSET = 0;

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.netxms.nebula.widgets.gallery.AbstractGridGroupRenderer#draw(org
	 * .eclipse.swt.graphics.GC, org.netxms.nebula.widgets.gallery.GalleryItem,
	 * int, int, int, int, int, int)
	 */
	public void draw(GC gc, GalleryItem group, int x, int y, int clipX,
			int clipY, int clipWidth, int clipHeight) {

		// Get items in the clipping area
		int[] indexes = getVisibleItems(group, x, y, clipX, clipY, clipWidth,
				clipHeight, OFFSET);

		if (indexes != null && indexes.length > 0) {
			for (int i = indexes.length - 1; i >= 0; i--) {
				// Draw item
				boolean selected = group.isSelected(group.getItem(indexes[i]));

				if (Gallery.DEBUG) {
					System.out
							.println("Selected : " + selected + " index : " + indexes[i] + "item : " + group.getItem(indexes[i])); //$NON-NLS-1$//$NON-NLS-2$ //$NON-NLS-3$
				}

				drawItem(gc, indexes[i], selected, group, OFFSET);

			}
		}
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.netxms.nebula.widgets.gallery.AbstractGridGroupRenderer#layout(org
	 * .eclipse.swt.graphics.GC, org.netxms.nebula.widgets.gallery.GalleryItem)
	 */
	public void layout(GC gc, GalleryItem group) {

		int countLocal = group.getItemCount();

		if (gallery.isVertical()) {
			int sizeX = group.width;
			group.height = OFFSET;

			Point l = gridLayout(sizeX, countLocal, itemWidth);
			int hCount = l.x;
			int vCount = l.y;
			if (autoMargin) {
				// Calculate best margins
				margin = calculateMargins(sizeX, hCount, itemWidth);
			}

			Point s = this.getSize(hCount, vCount, itemWidth, itemHeight,
					minMargin, margin);
			group.height += s.y;

			group.setData(H_COUNT, new Integer(hCount));
			group.setData(V_COUNT, new Integer(vCount));
		} else {
			int sizeY = group.height;
			group.width = OFFSET;

			Point l = gridLayout(sizeY, countLocal, itemHeight);
			int vCount = l.x;
			int hCount = l.y;
			if (autoMargin) {
				// Calculate best margins
				margin = calculateMargins(sizeY, vCount, itemHeight);
			}

			Point s = this.getSize(hCount, vCount, itemWidth, itemHeight,
					minMargin, margin);
			group.width += s.x;

			group.setData(H_COUNT, new Integer(hCount));
			group.setData(V_COUNT, new Integer(vCount));
		}

	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.netxms.nebula.widgets.gallery.AbstractGridGroupRenderer#getItem(
	 * org.netxms.nebula.widgets.gallery.GalleryItem,
	 * org.eclipse.swt.graphics.Point)
	 */
	public GalleryItem getItem(GalleryItem group, Point coords) {
		return super.getItem(group, coords, OFFSET);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.netxms.nebula.widgets.gallery.AbstractGridGroupRenderer#mouseDown
	 * (org.netxms.nebula.widgets.gallery.GalleryItem,
	 * org.eclipse.swt.events.MouseEvent, org.eclipse.swt.graphics.Point)
	 */
	public boolean mouseDown(GalleryItem group, MouseEvent e, Point coords) {
		// Do nothing
		return true;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.netxms.nebula.widgets.gallery.AbstractGridGroupRenderer#getSize(
	 * org.netxms.nebula.widgets.gallery.GalleryItem)
	 */
	public Rectangle getSize(GalleryItem item) {
		return super.getSize(item, OFFSET);
	}
}
