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

import org.eclipse.swt.SWT;
import org.eclipse.swt.SWTException;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Item;

/**
 * <p>
 * Instances of this class represent a selectable user interface object that
 * represents an item in a gallery.
 * </p>
 * 
 * <p>
 * NOTE: THIS WIDGET AND ITS API ARE STILL UNDER DEVELOPMENT.
 * </p>
 * 
 * @see Gallery
 * 
 * @author Nicolas Richeton (nicolas.richeton@gmail.com)
 * @contributor Peter Centgraf (bugs 212071, 212073)
 * 
 */

public class GalleryItem extends Item {

	private static final String EMPTY_STRING = ""; //$NON-NLS-1$

	private String[] text = { EMPTY_STRING, EMPTY_STRING, EMPTY_STRING };

	// This is managed by the Gallery
	/**
	 * Children of this item. Only used when groups are enabled.
	 */
	protected GalleryItem[] items = null;

	/**
	 * Bounds of this items in the current Gallery.
	 * 
	 * X and Y values are used for vertical or horizontal offset depending on
	 * the Gallery settings. Only used when groups are enabled.
	 * 
	 * Width and height
	 */
	// protected Rectangle bounds = new Rectangle(0, 0, 0, 0);
	protected int x = 0;

	protected int y = 0;

	/**
	 * Size of the group, including its title.
	 */
	protected int width = 0;

	protected int height = 0;

	protected int marginBottom = 0;

	protected int hCount = 0;

	protected int vCount = 0;

	/**
	 * Last result of indexOf( GalleryItem). Used for optimisation.
	 */
	protected int lastIndexOf = 0;

	/**
	 * True if the Gallery was created wih SWT.VIRTUAL
	 */
	private boolean virtualGallery;

	private Gallery parent;

	private GalleryItem parentItem;

	protected int[] selectionIndices = null;

	protected Font font;

	protected Color foreground, background;

	private boolean ultraLazyDummy = false;

	/**
	 * 
	 */
	private boolean expanded;

	protected boolean isUltraLazyDummy() {
		return ultraLazyDummy;
	}

	protected void setUltraLazyDummy(boolean ultraLazyDummy) {
		this.ultraLazyDummy = ultraLazyDummy;
	}

	public Gallery getParent() {
		return parent;
	}

	protected void setParent(Gallery parent) {
		this.parent = parent;
	}

	public GalleryItem getParentItem() {
		return parentItem;
	}

	protected void setParentItem(GalleryItem parentItem) {
		this.parentItem = parentItem;
	}

	public GalleryItem(Gallery parent, int style) {
		this(parent, style, -1, true);
	}

	public GalleryItem(Gallery parent, int style, int index) {
		this(parent, style, index, true);
	}

	public GalleryItem(GalleryItem parent, int style) {
		this(parent, style, -1, true);
	}

	public GalleryItem(GalleryItem parent, int style, int index) {
		this(parent, style, index, true);
	}

	protected GalleryItem(GalleryItem parent, int style, int index,
			boolean create) {
		super(parent, style);

		this.parent = parent.parent;
		this.parentItem = parent;
		if ((parent.getStyle() & SWT.VIRTUAL) > 0) {
			virtualGallery = true;
		}

		if (create)
			parent.addItem(this, index);

	}

	protected GalleryItem(Gallery parent, int style, int index, boolean create) {
		super(parent, style);
		this.parent = parent;
		this.parentItem = null;
		if ((parent.getStyle() & SWT.VIRTUAL) > 0) {
			virtualGallery = true;
		}

		if (create)
			parent.addItem(this, index);

	}

	protected void addItem(GalleryItem item, int position) {
		if (position != -1 && (position < 0 || position > getItemCount())) {
			throw new IllegalArgumentException("ERROR_INVALID_RANGE"); //$NON-NLS-1$
		}
		_addItem(item, position);
	}

	private void _addItem(GalleryItem item, int position) {
		// TODO: ensure that there was no item at this position before using
		// this item in virtual mode

		// Insert item
		items = (GalleryItem[]) parent._arrayAddItem(items, item, position);

		// Update Gallery
		parent.updateStructuralValues(null, false);
		parent.updateScrollBarsProperties();
	}

	/**
	 * Returns the number of items contained in the receiver that are direct
	 * item children of the receiver.
	 * 
	 * @return
	 */
	public int getItemCount() {

		if (items == null)
			return 0;

		return items.length;
	}

	/**
	 * Only work when the table was created with SWT.VIRTUAL
	 * 
	 * @param itemCount
	 */
	public void setItemCount(int count) {
		if (count == 0) {
			// No items
			items = null;
		} else {
			// At least one item, create a new array and copy data from the
			// old one.
			GalleryItem[] newItems = new GalleryItem[count];
			if (items != null) {
				System.arraycopy(items, 0, newItems, 0, Math.min(count,
						items.length));
			}
			items = newItems;
		}
	}

	/**
	 * Searches the receiver's list starting at the first item (index 0) until
	 * an item is found that is equal to the argument, and returns the index of
	 * that item. <br/>
	 * If SWT.VIRTUAL is used and the item has not been used yet, the item is
	 * created and a SWT.SetData event is fired.
	 * 
	 * @param index
	 *            : index of the item.
	 * @return : the GalleryItem or null if index is out of bounds
	 */
	public GalleryItem getItem(int index) {
		checkWidget();
		return parent._getItem(this, index);
	}

	public GalleryItem[] getItems() {
		checkWidget();
		if (items == null)
			return new GalleryItem[0];

		GalleryItem[] itemsLocal = new GalleryItem[this.items.length];
		System.arraycopy(items, 0, itemsLocal, 0, this.items.length);

		return itemsLocal;
	}

	/**
	 * Returns the index of childItem within this item or -1 if childItem is not
	 * found. The search is only one level deep.
	 * 
	 * @param childItem
	 * @return
	 */
	public int indexOf(GalleryItem childItem) {
		checkWidget();

		return parent._indexOf(this, childItem);
	}

	public void setImage(Image image) {
		super.setImage(image);
		parent.redraw(this);
	}

	/**
	 * Returns true if the receiver is expanded, and false otherwise.
	 * 
	 * @return
	 */
	public boolean isExpanded() {
		return expanded;
	}

	/**
	 * Sets the expanded state of the receiver.
	 * 
	 * @param expanded
	 */
	public void setExpanded(boolean expanded) {
		checkWidget();
		_setExpanded(expanded, true);
	}

	public void _setExpanded(boolean expanded, boolean redraw) {
		this.expanded = expanded;
		parent.updateStructuralValues(this, false);
		parent.updateScrollBarsProperties();

		if (redraw) {
			parent.redraw();
		}
	}

	/**
	 * @deprecated
	 * @return
	 */
	public String getDescription() {
		return getText(1);
	}

	/**
	 * @param description
	 * @deprecated
	 */
	public void setDescription(String description) {
		setText(1, description);
	}

	/**
	 * Deselect all children of this item
	 */
	public void deselectAll() {
		checkWidget();
		_deselectAll();
		parent.redraw();
	}

	protected void _deselectAll() {
		this.selectionIndices = null;
		if (items == null)
			return;
		for (int i = 0; i < items.length; i++) {
			if (items[i] != null)
				items[i]._deselectAll();
		}
	}

	protected void _addSelection(GalleryItem item) {
		// Deselect all items is multi selection is disabled
		if (!parent.multi) {
			_deselectAll();
		}

		if (item.getParentItem() == this) {
			if (selectionIndices == null) {
				selectionIndices = new int[1];
			} else {
				int[] oldSelection = selectionIndices;
				selectionIndices = new int[oldSelection.length + 1];
				System.arraycopy(oldSelection, 0, selectionIndices, 0,
						oldSelection.length);
			}
			selectionIndices[selectionIndices.length - 1] = indexOf(item);

		}
	}

	protected boolean isSelected(GalleryItem item) {
		if (item == null)
			return false;

		if (item.getParentItem() == this) {
			if (selectionIndices == null)
				return false;

			int index = indexOf(item);
			for (int i = 0; i < selectionIndices.length; i++) {
				if (selectionIndices[i] == index)
					return true;
			}
		}
		return false;
	}

	protected void select(int from, int to) {
		if (Gallery.DEBUG)
			System.out.println("GalleryItem.select(  " + from + "," + to + ")"); //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$

		for (int i = from; i <= to; i++) {
			GalleryItem item = getItem(i);
			parent._addSelection(item);
			item._selectAll();
		}
	}

	/**
	 * Return the current bounds of the item. This method may return negative
	 * values if it is not visible.
	 * 
	 * @return
	 */
	public Rectangle getBounds() {
		// The y coords is relative to the client area because it may return
		// wrong values
		// on win32 when using the scroll bars. Instead, I use the absolute
		// position and make it relative using the current translation.

		if (parent.isVertical()) {
			return new Rectangle(x, y - parent.translate, width, height);
		}

		return new Rectangle(x - parent.translate, y, width, height);
	}

	public Font getFont() {
		return getFont(false);
	}

	public Font getFont(boolean itemOnly) {
		checkWidget();

		if (itemOnly) {
			return font;
		}

		// Let the renderer decide the color.
		if (parent.getGroupRenderer() != null) {
			return parent.getGroupRenderer().getFont(this);
		}

		// Default SWT behavior if no renderer.
		return font != null ? font : parent.getFont();
	}

	public void setFont(Font font) {
		checkWidget();
		if (font != null && font.isDisposed()) {
			SWT.error(SWT.ERROR_INVALID_ARGUMENT);
		}
		this.font = font;
		this.parent.redraw(this);
	}

	/**
	 * Returns the receiver's foreground color.
	 * 
	 * @return The foreground color
	 * 
	 * @exception SWTException
	 *                <ul>
	 *                <li>ERROR_WIDGET_DISPOSED - if the receiver has been
	 *                disposed</li>
	 *                <li>ERROR_THREAD_INVALID_ACCESS - if not called from the
	 *                thread that created the receiver</li>
	 *                </ul>
	 * 
	 */
	public Color getForeground() {
		return getForeground(false);
	}

	/**
	 * Returns the receiver's foreground color.
	 * 
	 * @param itemOnly
	 *            If TRUE, does not try to use renderer or parent widget to
	 *            guess the real foreground color. Note : FALSE is the default
	 *            behavior.
	 * 
	 * @return The foreground color
	 * 
	 * @exception SWTException
	 *                <ul>
	 *                <li>ERROR_WIDGET_DISPOSED - if the receiver has been
	 *                disposed</li>
	 *                <li>ERROR_THREAD_INVALID_ACCESS - if not called from the
	 *                thread that created the receiver</li>
	 *                </ul>
	 * 
	 */
	public Color getForeground(boolean itemOnly) {
		checkWidget();

		if (itemOnly) {
			return this.foreground;
		}

		// Let the renderer decide the color.
		if (parent.getGroupRenderer() != null) {
			return parent.getGroupRenderer().getForeground(this);
		}

		// Default SWT behavior if no renderer.
		return foreground != null ? foreground : parent.getForeground();
	}

	/**
	 * Sets the receiver's foreground color to the color specified by the
	 * argument, or to the default system color for the item if the argument is
	 * null.
	 * 
	 * @param color
	 *            The new color (or null)
	 * 
	 * @exception IllegalArgumentException
	 *                <ul>
	 *                <li>ERROR_INVALID_ARGUMENT - if the argument has been
	 *                disposed</li>
	 *                </ul>
	 * @exception SWTException
	 *                <ul>
	 *                <li>ERROR_WIDGET_DISPOSED - if the receiver has been
	 *                disposed</li>
	 *                <li>ERROR_THREAD_INVALID_ACCESS - if not called from the
	 *                thread that created the receiver</li>
	 *                </ul>
	 * 
	 */
	public void setForeground(Color foreground) {
		checkWidget();
		if (foreground != null && foreground.isDisposed()) {
			SWT.error(SWT.ERROR_INVALID_ARGUMENT);
		}
		this.foreground = foreground;
		this.parent.redraw(this);
	}

	/**
	 * Returns the receiver's background color.
	 * 
	 * @return The background color
	 * 
	 * @exception SWTException
	 *                <ul>
	 *                <li>ERROR_WIDGET_DISPOSED - if the receiver has been
	 *                disposed</li>
	 *                <li>ERROR_THREAD_INVALID_ACCESS - if not called from the
	 *                thread that created the receiver</li>
	 *                </ul>
	 * 
	 */
	public Color getBackground() {
		return getBackground(false);
	}

	/**
	 * Returns the receiver's background color.
	 * 
	 * @param itemOnly
	 *            If TRUE, does not try to use renderer or parent widget to
	 *            guess the real background color. Note : FALSE is the default
	 *            behavior.
	 * 
	 * @return The background color
	 * 
	 * @exception SWTException
	 *                <ul>
	 *                <li>ERROR_WIDGET_DISPOSED - if the receiver has been
	 *                disposed</li>
	 *                <li>ERROR_THREAD_INVALID_ACCESS - if not called from the
	 *                thread that created the receiver</li>
	 *                </ul>
	 * 
	 */
	public Color getBackground(boolean itemOnly) {
		checkWidget();

		// If itemOnly, return this item's color attribute.
		if (itemOnly) {
			return this.background;
		}

		// Let the renderer decide the color.
		if (parent.getGroupRenderer() != null) {
			return parent.getGroupRenderer().getBackground(this);
		}

		// Default SWT behavior if no renderer.
		return background != null ? background : parent.getBackground();
	}

	/**
	 * Sets the receiver's background color to the color specified by the
	 * argument, or to the default system color for the item if the argument is
	 * null.
	 * 
	 * @param color
	 *            The new color (or null)
	 * 
	 * @exception IllegalArgumentException
	 *                <ul>
	 *                <li>ERROR_INVALID_ARGUMENT - if the argument has been
	 *                disposed</li>
	 *                </ul>
	 * @exception SWTException
	 *                <ul>
	 *                <li>ERROR_WIDGET_DISPOSED - if the receiver has been
	 *                disposed</li>
	 *                <li>ERROR_THREAD_INVALID_ACCESS - if not called from the
	 *                thread that created the receiver</li>
	 *                </ul>
	 * 
	 */
	public void setBackground(Color background) {
		checkWidget();
		if (background != null && background.isDisposed()) {
			SWT.error(SWT.ERROR_INVALID_ARGUMENT);
		}
		this.background = background;
		this.parent.redraw(this);
	}

	/**
	 * Reset item values to defaults.
	 */
	public void clear() {
		checkWidget();
		// Clear all attributes
		text[0] = EMPTY_STRING;
		text[1] = EMPTY_STRING;
		text[2] = EMPTY_STRING;
		super.setImage(null);
		this.font = null;
		background = null;
		foreground = null;

		// Force redraw
		this.parent.redraw(this);
	}

	public void clearAll() {
		clearAll(false);
	}

	public void clearAll(boolean all) {
		checkWidget();

		if (items == null)
			return;

		if (virtualGallery) {
			items = new GalleryItem[items.length];
		} else {
			for (int i = 0; i < items.length; i++) {
				if (items[i] != null) {
					if (all) {
						items[i].clearAll(true);
					}
					items[i].clear();
				}
			}
		}
	}

	/**
	 * Selects all of the items in the receiver.
	 */
	public void selectAll() {
		checkWidget();
		_selectAll();
		parent.redraw();
	}

	protected void _selectAll() {
		select(0, this.getItemCount() - 1);
	}

	public void remove(int index) {
		checkWidget();
		parent._remove(this, index);

		parent.updateStructuralValues(null, false);
		parent.updateScrollBarsProperties();
		parent.redraw();
	}

	public void remove(GalleryItem item) {
		remove(indexOf(item));
	}

	/**
	 * Disposes the gallery Item. This method is call directly by gallery and
	 * should not be used by a client
	 */
	protected void _dispose() {
		removeFromParent();
		_disposeChildren();
		super.dispose();
	}

	protected void _disposeChildren() {
		if (items != null) {
			while (items != null) {
				if (items[items.length - 1] != null) {
					items[items.length - 1]._dispose();
				} else {
					// This is an uninitialized item, just remove the slot
					parent._remove(this, items.length - 1);
				}
			}
		}
	}

	protected void removeFromParent() {
		if (parentItem != null) {
			int index = parent._indexOf(parentItem, this);
			parent._remove(parentItem, index);
		} else {
			int index = parent._indexOf(this);
			parent._remove(index);
		}
	}

	public void dispose() {
		checkWidget();

		removeFromParent();
		_disposeChildren();
		super.dispose();

		parent.updateStructuralValues(null, false);
		parent.updateScrollBarsProperties();
		parent.redraw();
	}

	public void setText(String string) {
		setText(0, string);
	}

	public void setText(int index, String string) {
		checkWidget();
		if (string == null)
			SWT.error(SWT.ERROR_NULL_ARGUMENT);
		text[index] = string;
		parent.redraw(this);
	}

	public String getText() {
		return getText(0);
	}

	public String getText(int index) {
		checkWidget();
		return text[index];
	}

}
