/*******************************************************************************
 * Copyright (c) 2006-2007 Nicolas Richeton.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors :
 *    Nicolas Richeton (nicolas.richeton@gmail.com) - initial API and implementation
 *    Richard Michalsky - bug 197959
 *******************************************************************************/
package org.netxms.nebula.widgets.gallery;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Item;

/**
 * <p>
 * Abstract class which provides low-level support for a grid-based group.
 * renderer.
 * </p>
 * <p>
 * NOTE: THIS WIDGET AND ITS API ARE STILL UNDER DEVELOPMENT.
 * </p>
 * 
 * @author Nicolas Richeton (nicolas.richeton@gmail.com)
 * @contributor Richard Michalsky (bug 197959)
 * @contributor Robert Handschmann (bug 215817)
 */

public abstract class AbstractGridGroupRenderer extends
		AbstractGalleryGroupRenderer {

	static final int DEFAULT_SIZE = 96;

	protected int minMargin;

	protected int margin;

	protected boolean autoMargin;

	protected int itemWidth = DEFAULT_SIZE;

	protected int itemHeight = DEFAULT_SIZE;

	public static final String H_COUNT = "g.h"; //$NON-NLS-1$

	public static final String V_COUNT = "g.v"; //$NON-NLS-1$

	protected static final String EMPTY_STRING = ""; //$NON-NLS-1$

	private static final int END = 0;

	private static final int START = 1;

	/**
	 * If true, groups are always expanded and toggle button is not displayed
	 */
	private boolean alwaysExpanded = false;

	public void draw(GC gc, GalleryItem group, int x, int y, int clipX,
			int clipY, int clipWidth, int clipHeight) {
	}

	public GalleryItem getItem(GalleryItem group, Point coords) {
		return null;
	}

	public Rectangle getSize(GalleryItem item) {
		return null;
	}

	public void layout(GC gc, GalleryItem group) {
	}

	/**
	 * If true, groups are always expanded and toggle button is not displayed
	 * 
	 * @return true if groups are always expanded
	 */
	public boolean isAlwaysExpanded() {
		return alwaysExpanded;
	}

	/**
	 * Return item expand state (item.isExpanded()) Returns always true is
	 * alwaysExpanded is set to true.
	 * 
	 * @param item
	 * @return
	 */
	protected boolean isGroupExpanded(GalleryItem item) {
		if (alwaysExpanded)
			return true;

		if (item == null)
			return false;

		return item.isExpanded();
	}

	/**
	 * If true, groups are always expanded and toggle button is not displayed if
	 * false, expand status depends on each item.
	 * 
	 * @param alwaysExpanded
	 */
	public void setAlwaysExpanded(boolean alwaysExpanded) {
		this.alwaysExpanded = alwaysExpanded;
	}

	public int getMinMargin() {
		return minMargin;
	}

	public int getItemWidth() {
		return itemWidth;
	}

	public void setItemWidth(int itemWidth) {
		this.itemWidth = itemWidth;

		updateGallery();
	}

	public int getItemHeight() {
		return itemHeight;
	}

	public void setItemHeight(int itemHeight) {
		this.itemHeight = itemHeight;

		updateGallery();
	}

	private void updateGallery() {
		// Update gallery
		if (gallery != null) {
			gallery.updateStructuralValues(null, true);
			gallery.updateScrollBarsProperties();
			gallery.redraw();
		}
	}

	public void setItemSize(int width, int height) {
		this.itemHeight = height;
		this.itemWidth = width;

		updateGallery();
	}

	public void setMinMargin(int minMargin) {
		this.minMargin = minMargin;

		updateGallery();
	}

	public boolean isAutoMargin() {
		return autoMargin;
	}

	public void setAutoMargin(boolean autoMargin) {
		this.autoMargin = autoMargin;

		updateGallery();
	}

	protected int calculateMargins(int size, int count, int itemSize) {
		int margin = this.minMargin;
		margin += Math
				.round((float) (size - this.minMargin - (count * (itemSize + this.minMargin)))
						/ (count + 1));
		return margin;
	}

	protected Point getSize(int nbx, int nby, int itemSizeX, int itemSizeY,
			int minMargin, int autoMargin) {
		int x = 0, y = 0;

		if (gallery.isVertical()) {
			x = nbx * itemSizeX + (nbx - 1) * margin + 2 * minMargin;
			y = nby * itemSizeY + nby * minMargin;
		} else {
			x = nbx * itemSizeX + nbx * minMargin;
			y = nby * itemSizeY + (nby - 1) * margin + 2 * minMargin;
		}
		return new Point(x, y);
	}

	/**
	 * Draw a child item. Only used when useGroup is true.
	 * 
	 * @param gc
	 * @param index
	 * @param selected
	 * @param parent
	 */
	protected void drawItem(GC gc, int index, boolean selected,
			GalleryItem parent, int offsetY) {

		if (Gallery.DEBUG)
			System.out.println("Draw item ? " + index); //$NON-NLS-1$

		if ((parent != null) && (index < parent.getItemCount())) 
		{
			int hCount = ((Integer) parent.getData(H_COUNT)).intValue();
			int vCount = ((Integer) parent.getData(V_COUNT)).intValue();

			if (Gallery.DEBUG)
				System.out.println("hCount :  " + hCount + " vCount : " //$NON-NLS-1$//$NON-NLS-2$
						+ vCount);

			int posX, posY;
			if (gallery.isVertical()) {
				posX = index % hCount;
				posY = (index - posX) / hCount;
			} else {
				posY = index % vCount;
				posX = (index - posY) / vCount;
			}

			Item item = parent.getItem(index);

			// No item ? return
			if (item == null)
				return;

			GalleryItem gItem = (GalleryItem) item;

			int xPixelPos, yPixelPos;
			if (gallery.isVertical()) {
				xPixelPos = posX * (itemWidth + margin) + margin;
				yPixelPos = posY * (itemHeight + minMargin) - gallery.translate
				/* + minMargin */
				+ parent.y + offsetY;
				gItem.x = xPixelPos;
				gItem.y = yPixelPos + gallery.translate;
			} else {
				xPixelPos = posX * (itemWidth + minMargin) - gallery.translate
				/* + minMargin */
				+ parent.x + offsetY;
				yPixelPos = posY * (itemHeight + margin) + margin;
				gItem.x = xPixelPos + gallery.translate;
				gItem.y = yPixelPos;
			}

			gItem.height = itemHeight;
			gItem.width = itemWidth;

			gallery.sendPaintItemEvent(item, index, gc, xPixelPos, yPixelPos,
					this.itemWidth, this.itemHeight);

			if (gallery.getItemRenderer() != null) {
				// gc.setClipping(xPixelPos, yPixelPos, itemWidth, itemHeight);
				gallery.getItemRenderer().setSelected(selected);
				if (Gallery.DEBUG)
					System.out.println("itemRender.draw"); //$NON-NLS-1$
				//Rectangle oldClipping = gc.getClipping();

				//gc.setClipping(oldClipping.intersection(new Rectangle(xPixelPos, yPixelPos, itemWidth, itemHeight)));
				gallery.getItemRenderer().draw(gc, gItem, index, xPixelPos,
						yPixelPos, itemWidth, itemHeight);
				//gc.setClipping(oldClipping);
				if (Gallery.DEBUG)
					System.out.println("itemRender done"); //$NON-NLS-1$
			}

		}
	}

	protected int[] getVisibleItems(GalleryItem group, int x, int y, int clipX,
			int clipY, int clipWidth, int clipHeight, int offset) {
		int[] indexes;

		if (gallery.isVertical()) {
			int count = ((Integer) group.getData(H_COUNT)).intValue();
			// TODO: Not used ATM
			// int vCount = ((Integer) group.getData(V_COUNT)).intValue();

			int firstLine = (clipY - y - offset - minMargin)
					/ (itemHeight + minMargin);
			if (firstLine < 0)
				firstLine = 0;

			int firstItem = firstLine * count;
			if (Gallery.DEBUG)
				System.out.println("First line : " + firstLine); //$NON-NLS-1$

			int lastLine = (clipY - y - offset + clipHeight - minMargin)
					/ (itemHeight + minMargin);

			if (lastLine < firstLine)
				lastLine = firstLine;

			if (Gallery.DEBUG)
				System.out.println("Last line : " + lastLine); //$NON-NLS-1$

			int lastItem = (lastLine + 1) * count;

			// exit if no item selected
			if (lastItem - firstItem == 0)
				return null;

			indexes = new int[lastItem - firstItem];
			for (int i = 0; i < (lastItem - firstItem); i++) {
				indexes[i] = firstItem + i;
			}

		} else {
			int count = ((Integer) group.getData(V_COUNT)).intValue();

			int firstLine = (clipX - x - offset - minMargin)
					/ (itemWidth + minMargin);
			if (firstLine < 0)
				firstLine = 0;

			int firstItem = firstLine * count;
			if (Gallery.DEBUG)
				System.out.println("First line : " + firstLine); //$NON-NLS-1$

			int lastLine = (clipX - x - offset + clipWidth - minMargin)
					/ (itemWidth + minMargin);

			if (lastLine < firstLine)
				lastLine = firstLine;

			if (Gallery.DEBUG)
				System.out.println("Last line : " + lastLine); //$NON-NLS-1$

			int lastItem = (lastLine + 1) * count;

			// exit if no item selected
			if (lastItem - firstItem == 0)
				return null;

			indexes = new int[lastItem - firstItem];
			for (int i = 0; i < (lastItem - firstItem); i++) {
				indexes[i] = firstItem + i;
			}
		}

		return indexes;
	}

	/**
	 * Calculate how many items are displayed horizontally and vertically.
	 * 
	 * @param size
	 * @param nbItems
	 * @param itemSize
	 * @return
	 */
	protected Point gridLayout(int size, int nbItems, int itemSize) {
		int x = 0, y = 0;

		if (nbItems == 0)
			return new Point(x, y);

		x = (size - minMargin) / (itemSize + minMargin);
		if (x > 0) {
			y = (int) Math.ceil((double) nbItems / (double) x);
		} else {
			// Show at least one item;
			y = nbItems;
			x = 1;
		}

		return new Point(x, y);
	}

	public void dispose() {
		// Nothing required here. This method can be overridden when needed.
	}

	public boolean mouseDown(GalleryItem group, MouseEvent e, Point coords) {
		return false;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.netxms.nebula.widgets.gallery.AbstractGalleryGroupRenderer#preLayout
	 * (org.eclipse.swt.graphics.GC)
	 */
	public void preLayout(GC gc) {
		// Reset margin to minimal value before "best fit" calculation
		this.margin = this.minMargin;
		super.preLayout(gc);
	}

	protected Point getLayoutData(GalleryItem item) {
		Integer hCount = ((Integer) item.getData(H_COUNT));
		Integer vCount = ((Integer) item.getData(V_COUNT));

		if (hCount == null || vCount == null)
			return null;

		return new Point(hCount.intValue(), vCount.intValue());
	}

	protected Rectangle getSize(GalleryItem item, int offsetY) {

		GalleryItem parent = item.getParentItem();
		if (parent != null) {
			int index = parent.indexOf(item);

			Point layoutData = getLayoutData(parent);
			if (layoutData == null)
				return null;

			int hCount = layoutData.x;
			int vCount = layoutData.y;

			if (Gallery.DEBUG)
				System.out.println("hCount :  " + hCount + " vCount : " //$NON-NLS-1$ //$NON-NLS-2$
						+ vCount);

			if (gallery.isVertical()) {
				int posX = index % hCount;
				int posY = (index - posX) / hCount;

				int xPixelPos = posX * (itemWidth + margin) + margin;
				int yPixelPos = posY * (itemHeight + minMargin)
						+ ((parent == null) ? 0 : (parent.y) + offsetY);

				return new Rectangle(xPixelPos, yPixelPos, this.itemWidth,
						this.itemHeight);
			}

			// gallery is horizontal
			int posY = index % vCount;
			int posX = (index - posY) / vCount;

			int yPixelPos = posY * (itemHeight + margin) + margin;
			int xPixelPos = posX * (itemWidth + minMargin)
					+ ((parent == null) ? 0 : (parent.x) + offsetY);

			return new Rectangle(xPixelPos, yPixelPos, this.itemWidth,
					this.itemHeight);
		}

		return null;
	}

	/**
	 * Get item at pixel position
	 * 
	 * @param coords
	 * @return
	 */
	protected GalleryItem getItem(GalleryItem group, Point coords, int offsetY) {
		if (Gallery.DEBUG) {
			System.out.println("getitem " + coords.x + " " + coords.y); //$NON-NLS-1$//$NON-NLS-2$
		}

		int itemNb;
		if (gallery.isVertical()) {
			Integer tmp = (Integer) group.getData(H_COUNT);
			if (tmp == null)
				return null;
			int hCount = tmp.intValue();

			// Calculate where the item should be if it exists
			int posX = (coords.x - margin) / (itemWidth + margin);

			// Check if the users clicked on the X margin.
			int posOnItem = (coords.x - margin) % (itemWidth + margin);
			if (posOnItem > itemWidth || posOnItem < 0) {
				return null;
			}

			if (posX >= hCount) // Nothing there
				return null;

			if (coords.y - group.y < offsetY)
				return null;

			int posY = (coords.y - group.y - offsetY)
					/ (itemHeight + minMargin);

			// Check if the users clicked on the Y margin.
			if (((coords.y - group.y - offsetY) % (itemHeight + minMargin)) > itemHeight) {
				return null;
			}
			itemNb = posX + posY * hCount;
		} else {
			Integer tmp = (Integer) group.getData(V_COUNT);
			if (tmp == null)
				return null;
			int vCount = tmp.intValue();

			// Calculate where the item should be if it exists
			int posY = (coords.y - margin) / (itemHeight + margin);

			// Check if the users clicked on the X margin.
			int posOnItem = (coords.y - margin) % (itemHeight + margin);
			if (posOnItem > itemHeight || posOnItem < 0) {
				return null;
			}

			if (posY >= vCount) // Nothing there
				return null;

			if (coords.x - group.x < offsetY)
				return null;

			int posX = (coords.x - group.x - offsetY) / (itemWidth + minMargin);

			// Check if the users clicked on the X margin.
			if (((coords.x - group.x - offsetY) % (itemWidth + minMargin)) > itemWidth) {
				return null;
			}
			itemNb = posY + posX * vCount;
		}

		if (Gallery.DEBUG) {
			System.out.println("Item found : " + itemNb); //$NON-NLS-1$
		}

		if (itemNb < group.getItemCount()) {
			return group.getItem(itemNb);
		}

		return null;
	}

	private GalleryItem goLeft(GalleryItem group, int posParam) {
		int pos = posParam - 1;

		if (pos < 0) {
			// Look for next non-empty group and get the last item
			GalleryItem item = null;
			GalleryItem currentGroup = group;
			while (item == null && currentGroup != null) {
				currentGroup = this.getPreviousGroup(currentGroup);
				item = this.getFirstItem(currentGroup, END);
			}
			return item;
		}

		// else
		return group.getItem(pos);
	}

	private GalleryItem goRight(GalleryItem group, int posParam) {
		int pos = posParam + 1;

		if (pos >= group.getItemCount()) {
			// Look for next non-empty group and get the first item
			GalleryItem item = null;
			GalleryItem currentGroup = group;
			while (item == null && currentGroup != null) {
				currentGroup = this.getNextGroup(currentGroup);
				item = this.getFirstItem(currentGroup, START);
			}
			return item;
		}

		// else
		return group.getItem(pos);
	}

	private GalleryItem goUp(GalleryItem group, int posParam, int hCount,
			int lineCount) {

		if (lineCount == 0) {
			return null;
		}

		// Optimization when only one group involved
		if (posParam - hCount * lineCount >= 0) {
			return group.getItem(posParam - hCount * lineCount);
		}

		// Get next item.
		GalleryItem next = goUp(group, posParam, hCount);
		if (next == null) {
			return null;
		}

		GalleryItem newItem = null;
		for (int i = 1; i < lineCount; i++) {
			newItem = goUp(next.getParentItem(), next.getParentItem().indexOf(
					next), hCount);
			if (newItem == next || newItem == null) {
				break;
			}

			next = newItem;
		}

		return next;
	}

	private GalleryItem goDown(GalleryItem group, int posParam, int hCount,
			int lineCount) {

		if (lineCount == 0) {
			return null;
		}

		// Optimization when only one group involved
		if (posParam + hCount * lineCount < group.getItemCount()) {
			return group.getItem(posParam + hCount * lineCount);
		}

		// Get next item.
		GalleryItem next = goDown(group, posParam, hCount);
		if (next == null) {
			return null;
		}

		GalleryItem newItem = null;
		for (int i = 1; i < lineCount; i++) {
			newItem = goDown(next.getParentItem(), next.getParentItem()
					.indexOf(next), hCount);
			if (newItem == next || newItem == null) {
				break;
			}

			next = newItem;
		}

		return next;
	}

	/**
	 * Get the next item, when going up.
	 * 
	 * @param group
	 *            current group
	 * @param posParam
	 *            index of currently selected item
	 * @param hCount
	 *            size of a line
	 * @return
	 */
	private GalleryItem goUp(GalleryItem group, int posParam, int hCount) {
		int colPos = posParam % hCount;
		int pos = posParam - hCount;

		if (pos < 0) {
			// Look for next non-empty group and get the last item
			GalleryItem item = null;
			GalleryItem currentGroup = group;
			while (item == null && currentGroup != null) {
				currentGroup = this.getPreviousGroup(currentGroup);
				item = this.getItemAt(currentGroup, colPos, END);
			}
			return item;
		}

		// else
		return group.getItem(pos);
	}

	/**
	 * Get the next item, when going down.
	 * 
	 * @param group
	 *            current group
	 * @param posParam
	 *            index of currently selected item
	 * @param hCount
	 *            size of a line
	 * @return
	 */
	private GalleryItem goDown(GalleryItem group, int posParam, int hCount) {
		int colPos = posParam % hCount;
		int pos = posParam + hCount;

		if (pos >= group.getItemCount()) {
			// Look for next non-empty group and get the first item
			GalleryItem item = null;
			GalleryItem currentGroup = group;
			while (item == null && currentGroup != null) {
				currentGroup = this.getNextGroup(currentGroup);
				item = this.getItemAt(currentGroup, colPos, START);
			}
			return item;

		}

		// else
		return group.getItem(pos);
	}

	/**
	 * Get maximum visible lines.
	 * 
	 * @return
	 */
	private int getMaxVisibleLines() {

		// TODO: support group titles (fewer lines are visible if one or more
		// group titles are displayed). This method should probably be
		// implemented in the group renderer and not in the abstract class.

		// Gallery is vertical
		if (gallery.isVertical()) {
			return gallery.getClientArea().height / itemHeight;
		}

		// Gallery is horizontal
		return gallery.getClientArea().width / itemWidth;
	}

	public GalleryItem getNextItem(GalleryItem item, int key) {
		// Key navigation is useless with an empty gallery
		if (gallery.getItemCount() == 0) {
			return null;
		}

		// Check for current selection
		if (item == null) {
			// No current selection, select the first item
			if (gallery.getItemCount() > 0) {
				GalleryItem firstGroup = gallery.getItem(0);
				if (firstGroup != null && firstGroup.getItemCount() > 0) {
					return firstGroup.getItem(0);
				}

			}
			return null;
		}

		// Check for groups
		if (item.getParentItem() == null) {
			// Key navigation is only available for child items ATM
			return null;
		}

		GalleryItem group = item.getParentItem();

		// Handle HOME and END
		switch (key) {
		case SWT.HOME:
			gallery.getItem(0).setExpanded(true);
			return getFirstItem(gallery.getItem(0), START);

		case SWT.END:
			gallery.getItem(gallery.getItemCount() - 1).setExpanded(true);
			return getFirstItem(gallery.getItem(gallery.getItemCount() - 1),
					END);
		}

		int pos = group.indexOf(item);
		GalleryItem next = null;

		// Handle arrows and page up / down
		if (gallery.isVertical()) {
			int hCount = ((Integer) group.getData(H_COUNT)).intValue();
			int maxVisibleRows = getMaxVisibleLines();
			switch (key) {
			case SWT.ARROW_LEFT:
				next = goLeft(group, pos);
				break;

			case SWT.ARROW_RIGHT:
				next = goRight(group, pos);
				break;

			case SWT.ARROW_UP:
				next = goUp(group, pos, hCount, 1);
				break;

			case SWT.ARROW_DOWN:
				next = goDown(group, pos, hCount, 1);
				break;

			case SWT.PAGE_UP:
				next = goUp(group, pos, hCount, Math.max(maxVisibleRows - 1, 1));
				break;

			case SWT.PAGE_DOWN:
				next = goDown(group, pos, hCount, Math.max(maxVisibleRows - 1,
						1));
				break;
			}
		} else {
			int vCount = ((Integer) group.getData(V_COUNT)).intValue();
			int maxVisibleColumns = getMaxVisibleLines();
			switch (key) {
			case SWT.ARROW_LEFT:
				next = goUp(group, pos, vCount);
				break;

			case SWT.ARROW_RIGHT:
				next = goDown(group, pos, vCount);
				break;

			case SWT.ARROW_UP:
				next = goLeft(group, pos);
				break;

			case SWT.ARROW_DOWN:
				next = goRight(group, pos);
				break;

			case SWT.PAGE_UP:
				next = goUp(group, pos, vCount
						* Math.max(maxVisibleColumns - 1, 1));
				break;

			case SWT.PAGE_DOWN:
				next = goDown(group, pos, vCount
						* Math.max(maxVisibleColumns - 1, 1));
				break;

			}
		}

		return next;
	}

	private GalleryItem getPreviousGroup(GalleryItem group) {
		int gPos = gallery.indexOf(group);
		while (gPos > 0) {
			GalleryItem newGroup = gallery.getItem(gPos - 1);
			if (isGroupExpanded(newGroup))
				return newGroup;
			gPos--;
		}

		return null;
	}

	private GalleryItem getNextGroup(GalleryItem group) {
		int gPos = gallery.indexOf(group);
		while (gPos < gallery.getItemCount() - 1) {
			GalleryItem newGroup = gallery.getItem(gPos + 1);
			if (isGroupExpanded(newGroup))
				return newGroup;
			gPos++;
		}

		return null;
	}

	private GalleryItem getFirstItem(GalleryItem group, int from) {
		if (group == null)
			return null;

		switch (from) {
		case END:
			return group.getItem(group.getItemCount() - 1);

		case START:
		default:
			return group.getItem(0);
		}

	}

	/**
	 * Return the child item of group which is at column 'pos' starting from
	 * direction. If this item doesn't exists, returns the nearest item.
	 * 
	 * @param group
	 * @param pos
	 * @param from
	 *            START or END
	 * @return
	 */
	private GalleryItem getItemAt(GalleryItem group, int pos, int from) {
		if (group == null)
			return null;

		int hCount = ((Integer) group.getData(H_COUNT)).intValue();
		int offset = 0;
		switch (from) {
		case END:
			if (group.getItemCount() == 0)
				return null;

			// Last item column
			int endPos = group.getItemCount() % hCount;

			// If last item column is 0, the line is full
			if (endPos == 0) {
				endPos = hCount - 1;
				offset--;
			}

			// If there is an item at column 'pos'
			if (pos < endPos) {
				int nbLines = (group.getItemCount() / hCount) + offset;
				return group.getItem(nbLines * hCount + pos);
			}

			// Get the last item.
			return group.getItem((group.getItemCount() / hCount + offset)
					* hCount + endPos - 1);

		case START:
		default:
			if (pos >= group.getItemCount())
				return group.getItem(group.getItemCount() - 1);

			return group.getItem(pos);

		}

	}

}
