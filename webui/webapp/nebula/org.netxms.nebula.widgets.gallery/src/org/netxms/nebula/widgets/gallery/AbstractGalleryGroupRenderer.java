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
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;

/**
 * <p>
 * Base class used to implement a custom gallery group renderer.
 * </p>
 * <p>
 * NOTE: THIS WIDGET AND ITS API ARE STILL UNDER DEVELOPMENT.
 * </p>
 * 
 * @author Nicolas Richeton (nicolas.richeton@gmail.com)
 */
public abstract class AbstractGalleryGroupRenderer {

	protected Gallery gallery;

	protected boolean expanded;

	/**
	 * Get the expand/collapse state of the current group
	 * 
	 * @return true is the current group is expanded
	 */
	public boolean isExpanded() {
		return this.expanded;
	}

	/**
	 * @see AbstractGalleryGroupRenderer#isExpanded()
	 * @param selected
	 */
	public void setExpanded(boolean selected) {
		this.expanded = selected;
	}

	/**
	 * This method is called before drawing the first item. It can be used to
	 * calculate some values (like font metrics) that will be used for each
	 * item.
	 * 
	 * @param gc
	 */
	public void preDraw(GC gc) {
		// Nothing required here. This method can be overridden when needed.
	}

	/**
	 * This method is called after drawing the last item. It may be used to
	 * cleanup and release resources created in preDraw().
	 * 
	 * @param gc
	 */
	public void postDraw(GC gc) {
		// Nothing required here. This method can be overridden when needed.
	}

	/**
	 * Group size informations can be retrieved from group. Clipping
	 * informations
	 * 
	 * @param gc
	 * @param group
	 * @param x
	 * @param y
	 */
	public abstract void draw(GC gc, GalleryItem group, int x, int y,
			int clipX, int clipY, int clipWidth, int clipHeight);

	public abstract void dispose();

	/**
	 * Returns the item that should be selected when the current item is 'item'
	 * and the 'key' is pressed
	 * 
	 * @param item
	 * @param key
	 * @return
	 */
	public abstract GalleryItem getNextItem(GalleryItem item, int key);

	/**
	 * This method is called before the layout of the first item. It can be used
	 * to calculate some values (like font metrics) that will be used for each
	 * item.
	 * 
	 * @param gc
	 */
	public void preLayout(GC gc) {
		// Nothing required here. This method can be overridden when needed.
	}

	/**
	 * This method is called after the layout of the last item.
	 * 
	 * @param gc
	 */
	public void postLayout(GC gc) {
		// Nothing required here. This method can be overridden when needed.
	}

	/**
	 * This method is called on each root item when the Gallery changes (resize,
	 * item addition or removal) in order to update the gallery size.
	 * 
	 * The implementation must update the item internal size (px) using
	 * setGroupSize(item, size); before returning.
	 * 
	 * @param gc
	 * @param group
	 */
	public abstract void layout(GC gc, GalleryItem group);

	/**
	 * Returns the item at coords relative to the parent group.
	 * 
	 * @param group
	 * @param coords
	 * @return
	 */
	public abstract GalleryItem getItem(GalleryItem group, Point coords);

	/**
	 * Returns the size of a group.
	 * 
	 * @param item
	 * @return
	 */
	public abstract Rectangle getSize(GalleryItem item);

	public abstract boolean mouseDown(GalleryItem group, MouseEvent e,
			Point coords);

	public Gallery getGallery() {
		return this.gallery;
	}

	public void setGallery(Gallery gallery) {
		this.gallery = gallery;
	}

	protected Point getGroupSize(GalleryItem item) {
		return new Point(item.width, item.height);

	}

	protected Point getGroupPosition(GalleryItem item) {
		return new Point(item.x, item.y);
	}

	protected void setGroupSize(GalleryItem item, Point size) {
		item.width = size.x;
		item.height = size.y;
	}

	/**
	 * This method can be used as a condition to print trace or debug
	 * informations in standard output.
	 * 
	 * @return true if Debug mode is enabled
	 */
	protected boolean isDebugMode() {
		return Gallery.DEBUG;
	}

	/**
	 * Notifies the Gallery that the control expanded/collapsed state has
	 * changed.
	 * 
	 * @param group
	 */
	protected void notifyTreeListeners(GalleryItem group) {
		gallery.notifyTreeListeners(group, group.isExpanded());
	}

	/**
	 * Forces an update of the gallery layout.
	 * 
	 * @param keeplocation
	 *            if true, the gallery will try to keep the current visible
	 *            items in the client area after the new layout has been
	 *            calculated.
	 */
	protected void updateStructuralValues(boolean keeplocation) {
		gallery.updateStructuralValues(null, keeplocation);
	}

	protected void updateScrollBarsProperties() {
		gallery.updateScrollBarsProperties();
	}

	/**
	 * Returns the preferred Scrollbar increment for the current gallery layout.
	 * 
	 * @return
	 */
	public int getScrollBarIncrement() {
		return 16;
	}

	/**
	 * Returns item background color. This method is called by
	 * {@link GalleryItem#getBackground()} and should be overridden by any group
	 * renderer which use additional colors.
	 * 
	 * Note that item renderer is automatically used for items.
	 * 
	 * @param item
	 *            a GalleryItem
	 * @return Color The current background color (never null)
	 */
	protected Color getBackground(GalleryItem item) {
		if (item != null) {

			if (item.getParentItem() == null
					&& gallery.getItemRenderer() != null) {
				// This is an item, let the renderer decide
				return gallery.getItemRenderer().getBackground(item);
			}

			// This is a group, or no item renderer. Use standard SWT behavior :

			// Use item color first
			if (item.background != null) {
				return item.background;
			}

			// Then parent color.
			return item.getParent().getBackground();

		}
		return null;
	}

	/**
	 * Returns item foreground color. This method is called by
	 * {@link GalleryItem#getForeground()} and should be overridden by any group
	 * renderer which use additional colors.
	 * 
	 * Note that item renderer is automatically used for items.
	 * 
	 * @param item
	 *            a GalleryItem
	 * @return The current foreground (never null)
	 */
	protected Color getForeground(GalleryItem item) {
		if (item != null) {

			if (item.getParentItem() != null
					&& gallery.getItemRenderer() != null) {
				// This is an item, let the renderer decide
				return gallery.getItemRenderer().getForeground(item);
			}
			// This is a group, or no item renderer. Use standard SWT behavior :

			// Use item color first
			if (item.foreground != null) {
				return item.foreground;
			}

			// Then parent color.
			return item.getParent().getForeground();

		}
		return null;
	}

	/**
	 * Returns item font. This method is called by {@link GalleryItem#getFont()}
	 * and should be overridden by any group renderer which use additional
	 * fonts.
	 * 
	 * Note that item renderer is automatically used for items.
	 * 
	 * @param item
	 *            a GalleryItem
	 * @return The current item Font (never null)
	 */
	protected Font getFont(GalleryItem item) {
		if (item != null) {

			if (item.getParentItem() != null
					&& gallery.getItemRenderer() != null) {
				// This is an item, let the renderer decide
				return gallery.getItemRenderer().getFont(item);
			}
			// This is a group, or no item renderer. Use standard SWT behavior :

			// Use item font first
			if (item.font != null) {
				return item.font;
			}

			// Then parent font.
			return item.getParent().getFont();

		}
		return null;
	}

}
