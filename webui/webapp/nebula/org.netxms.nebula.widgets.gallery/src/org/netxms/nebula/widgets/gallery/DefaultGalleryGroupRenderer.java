/*******************************************************************************
 * Copyright (c) 2006-2007 Nicolas Richeton.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors :
 *    Nicolas Richeton (nicolas.richeton@gmail.com) - initial API and implementation
 *    Richard Michalsky - bug 195443
 *******************************************************************************/
package org.netxms.nebula.widgets.gallery;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.graphics.Transform;
import org.eclipse.swt.widgets.Item;
import org.netxms.nebula.animation.AnimationRunner;
import org.netxms.nebula.animation.movement.IMovement;
import org.netxms.nebula.animation.movement.LinearInOut;

/**
 * <p>
 * Default group renderer used by the Gallery widget. Supports multi-line text,
 * images, animation and several other features.
 * </p>
 * <p>
 * NOTE: THIS WIDGET AND ITS API ARE STILL UNDER DEVELOPMENT.
 * </p>
 * 
 * @author Nicolas Richeton (nicolas.richeton@gmail.com)
 * @contributor Richard Michalsky (bug 195443)
 * 
 */
public class DefaultGalleryGroupRenderer extends AbstractGridGroupRenderer {

	private AnimationRunner animationRunner = new AnimationRunner();

	private static final String PARENTHESIS_OPEN = " ("; //$NON-NLS-1$

	private static final String PARENTHESIS_CLOSE = ")"; //$NON-NLS-1$

	private int fontHeight = 0;

	private int titleHeight = fontHeight + 5;

	private Color titleForeground;

	private Color descriptionColor;

	private Color titleBackground = null;

	private boolean titleBackgroundGradient = true;

	// Used for gradient
	private Color titleBackground2 = null;

	private int maxImageWidth = 32;

	private int maxImageHeight = 32;

	private Point imageSize = null;
	/**
	 * If true, this flag will enable a special behavior when the items are so
	 * large that only one can fit in the client area. In this case, items are
	 * always resized and centered to fit best in the client area.
	 */
	private boolean fillIfSingleColumn = false;

	/**
	 * This flag is set during layout, if fillIfSigle is true, and if there is
	 * only one column or row
	 */
	private boolean fill = false;

	/**
	 * True if margins have already been calculated. Prevents margins
	 * calculation for each group
	 */
	boolean marginCalculated = false;

	private Font font = null;

	protected boolean animation = false;

	protected int animationLength = 500;

	protected IMovement animationOpenMovement = new LinearInOut();

	protected IMovement animationCloseMovement = new LinearInOut();

	protected static final String DATA_ANIMATION = "org.netxms.nebula.gallery.internal.animation"; //$NON-NLS-1$

	/**
	 * This group renderer draws a title line, then items in a grid layout.
	 */
	public DefaultGalleryGroupRenderer() {
		super();
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.netxms.nebula.widgets.gallery.AbstractGalleryGroupRenderer#setGallery
	 * (org.netxms.nebula.widgets.gallery.Gallery)
	 */
	public void setGallery(Gallery gallery) {
		super.setGallery(gallery);

		// Set defaults
		if (titleForeground == null) {
			// Reset defaults.
			this.setTitleForeground(null);
		}

		if (titleBackground == null) {
			// Reset default gradient.
			setTitleBackgroundGradient(null, null);
		}

		if (descriptionColor == null) {
			descriptionColor = gallery.getDisplay().getSystemColor(
					SWT.COLOR_DARK_BLUE);
		}

	}

	/**
	 * Draw group background using system default gradient or the user-defined
	 * color.
	 * 
	 * @param gc
	 * @param item
	 *            TODO
	 * @param x
	 * @param y
	 * @param width
	 * @param height
	 */
	protected void drawGroupBackground(GC gc, GalleryItem item, int x, int y,
			int width, int height) {
		Color itemLocalBackground = item.getBackground(true);
		if (!titleBackgroundGradient || itemLocalBackground != null) {
			// User defined background
			gc.setBackground(itemLocalBackground != null ? itemLocalBackground
					: titleBackground);
			gc.fillRectangle(x, y, width, height);
		} else {
			// Default gradient Background
			gc.setBackground(this.titleBackground);
			gc.setForeground(this.titleBackground2);
			gc.fillGradientRectangle(x, y, width, height, true);
		}
	}

	/**
	 * Draw the toggle button.
	 * 
	 * @param gc
	 * @param x
	 * @param y
	 * @param group
	 */
	protected int drawGroupToggleButton(GC gc, int x, int y, GalleryItem group) {
		if (!isAlwaysExpanded()) {
			// Toggle Button

			int xShift = RendererHelper.getShift(titleHeight, 9);
			int yShift = RendererHelper.getShift(titleHeight, 9);

			int toggleX = x + xShift;
			int toggleY = y + yShift;

			gc.setBackground(gc.getDevice().getSystemColor(
					SWT.COLOR_LIST_BACKGROUND));
			gc.fillRectangle(toggleX, toggleY, 8, 8);

			gc.setForeground(gc.getDevice().getSystemColor(
					SWT.COLOR_WIDGET_FOREGROUND));
			gc.drawLine(toggleX + 2, toggleY + 4, toggleX + 6, toggleY + 4);
			if (!expanded) {
				gc.drawLine(toggleX + 4, toggleY + 2, toggleX + 4, toggleY + 6);
			}
			gc.setForeground(gc.getDevice().getSystemColor(
					SWT.COLOR_WIDGET_NORMAL_SHADOW));
			gc.drawRectangle(toggleX, toggleY, 8, 8);

			// if (isFocus()) {
			// gc.setBackground(back);
			// gc.setForeground(fore);
			// gc.drawFocus(-1, -1, 11, 11);
			// }

		}

		return titleHeight + minMargin;
	}

	protected Rectangle getToggleButtonBounds() {
		return new Rectangle(minMargin
				+ RendererHelper.getShift(titleHeight, 9), RendererHelper
				.getShift(titleHeight, 9), 9, 9);
	}

	protected int getGroupHeight(GalleryItem group) {
		int groupHeight = titleHeight;

		if (group.getImage() != null) {
			Point imageSize = RendererHelper.getBestSize(group.getImage()
					.getBounds().width, group.getImage().getBounds().height,
					maxImageWidth, maxImageHeight);
			groupHeight = Math.max(titleHeight, imageSize.y + 2 * minMargin);
		}

		// Ensure there is enough room to display all text.
		int lineCount = 1;
		if (group.getText(1) != null && !EMPTY_STRING.equals(group.getText(1))) {
			lineCount++;
		}

		if (group.getText(2) != null && !EMPTY_STRING.equals(group.getText(2))) {
			lineCount++;
		}

		groupHeight = Math.max(groupHeight, lineCount * (fontHeight + 2) + 2);

		return groupHeight;
	}

	protected void drawGroup(GC gc, GalleryItem group, int x, int y, int clipX,
			int clipY, int clipWidth, int clipHeight) {
		// Do not paint group if on single column and filling on.
		if (fill)
			return;

		imageSize = null;
		if (group.getImage() != null) {
			imageSize = RendererHelper.getBestSize(
					group.getImage().getBounds().width, group.getImage()
							.getBounds().height, maxImageWidth, maxImageHeight);
		}
		int groupHeight = getGroupHeight(group);

		if (gallery.isVertical()) {
			int baseX = x + minMargin;
			int baseY = y;

			// Center if image
			if (group.getImage() != null) {
				baseY += (imageSize.y - fontHeight) / 2;
			}

			int textY = baseY + 2;
			for (int i = 1; i < 3; i++) {
				if (group.getText(i) != null
						&& !EMPTY_STRING.equals(group.getText(i))) {
					textY -= fontHeight / 2 + 1;
				}
			}
			textY = Math.max(y + 2, textY);

			// Title background
			drawGroupBackground(gc, group, x, y, group.width, groupHeight);

			baseX += drawGroupToggleButton(gc, baseX, textY - 1, group);
			baseX += drawGroupImage(gc, group, baseX, y, imageSize);

			// Color for text
			gc.setForeground(group.getForeground(true) != null ? group
					.getForeground(true) : titleForeground);

			// Title text
			gc.setFont(getFont(group));
			gc.drawText(getGroupTitle(group), baseX, textY, true);

			// Description
			gc.setForeground(descriptionColor);
			for (int i = 1; i < 3; i++) {
				if (group.getText(i) != null
						&& !EMPTY_STRING.equals(group.getText(i))) {
					gc.drawText(group.getText(i), baseX, textY + i
							* (2 + fontHeight), true);
				}
			}

		} else {

			Transform transform = new Transform(gc.getDevice());
			transform.rotate(-90);
			gc.setTransform(transform);

			int baseX = x;
			int baseY = y - group.height;

			// Center if image
			if (group.getImage() != null) {
				baseX += (imageSize.y - fontHeight) / 2;
			}

			int textX = baseX + 2;
			for (int i = 1; i < 3; i++) {
				if (group.getText(i) != null) {
					textX -= fontHeight / 2 + 1;
				}
			}
			textX = Math.max(x + 2, textX);

			// Title background
			drawGroupBackground(gc, group, y - group.height, x, group.height,
					groupHeight);

			baseY += drawGroupToggleButton(gc, baseY, textX - 1, group);
			baseY += drawGroupImage(gc, group, baseY, x, imageSize);

			// Color for text
			gc.setForeground(group.foreground != null ? group.foreground
					: titleForeground);

			// Title text
			gc.setFont(getFont(group));

			gc.drawText(getGroupTitle(group), baseY, textX, true);

			gc.setForeground(descriptionColor);
			for (int i = 1; i < 3; i++) {
				if (group.getText(i) != null) {
					gc.drawText(group.getText(i), baseY, textX + i
							* (2 + fontHeight), true);
				}
			}
			gc.setTransform(null);
			transform.dispose();
		}
	}

	// /*
	// * (non-Javadoc)
	// *
	// * @see
	// * org.netxms.nebula.widgets.gallery.AbstractGalleryGroupRenderer#getFont
	// * (org.netxms.nebula.widgets.gallery.GalleryItem)
	// */
	// protected Font getFont(GalleryItem item) {
	// if (item != null) {
	//
	// if (item.getParentItem() != null) {
	// return super.getFont(item);
	// }
	//
	// // This is a group
	//
	// // Use item font first
	// if (item.font != null) {
	// return item.font;
	// }
	//
	// if (font != null) {
	// return font;
	// }
	//
	// // Then parent font.
	// return item.getParent().getFont();
	// }
	//
	// return null;
	// }

	private int drawGroupImage(GC gc, GalleryItem group, int x, int y,
			Point imageSize2) {
		if (imageSize2 == null)
			return 0;

		Image img = group.getImage();
		Rectangle imgSize = img.getBounds();

		Point offset = RendererHelper.getImageOffset(imageSize2.x,
				imageSize2.y, maxImageWidth, getGroupHeight(group));
		gc.drawImage(img, 0, 0, imgSize.width, imgSize.height, x + offset.x, y
				+ offset.y, imageSize2.x, imageSize2.y);

		return maxImageWidth + 2 * minMargin;
	}

	protected String getGroupTitle(GalleryItem group) {
		StringBuffer titleBuffer = new StringBuffer();
		titleBuffer.append(group.getText());
		titleBuffer.append(PARENTHESIS_OPEN);
		titleBuffer.append(group.getItemCount());
		titleBuffer.append(PARENTHESIS_CLOSE);
		return titleBuffer.toString();
	}

	/**
	 * Returns a group offset (size of title + margin)
	 * 
	 * @param item
	 * @return group offset or 0 if the item is not a group
	 */
	protected int getGroupOffset(GalleryItem item) {
		if (item.getParentItem() != null) {
			return 0;
		}

		return getGroupHeight(item) + minMargin;
	}

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
		// Draw group
		drawGroup(gc, group, x, y, clipX, clipY, clipWidth, clipHeight);

		int groupOffset = getGroupOffset(group);

		// Display item
		if (isGroupExpanded(group)) {
			int[] indexes = getVisibleItems(group, x, y, clipX, clipY,
					clipWidth, clipHeight, groupOffset);

			if (fill) {
				indexes = new int[] { indexes[0] };
			}

			if (indexes != null && indexes.length > 0) {
				for (int i = indexes.length - 1; i >= 0; i--) {

					boolean selected = group.isSelected(group
							.getItem(indexes[i]));

					if (Gallery.DEBUG) {
						System.out.println("Selected : " + selected //$NON-NLS-1$
								+ " index : " + indexes[i] + "item : " //$NON-NLS-1$//$NON-NLS-2$
								+ group.getItem(indexes[i]));
					}

					drawItem(gc, indexes[i], selected, group, groupOffset);

				}
			}
		}
	}

	public void layout(GC gc, GalleryItem group) {

		int countLocal = group.getItemCount();

		double animationRatio = 1;

		// If animation is used, load the current size ratio from the object
		// itself.
		if (animation) {
			Object animationGroupData = group
					.getData(DefaultGalleryGroupRenderer.DATA_ANIMATION);
			if (animationGroupData != null
					&& animationGroupData instanceof Double) {
				animationRatio = ((Double) animationGroupData).doubleValue();
				if (animationRatio < 0) {
					animationRatio = 0;
				}
			}
		}

		if (gallery.isVertical()) {
			int sizeX = group.width;
			group.height = getGroupOffset(group);

			Point l = gridLayout(sizeX, countLocal, itemWidth);
			int hCount = l.x;
			int vCount = l.y;

			if (autoMargin && hCount > 0) {
				// If margins have not been calculated
				if (!marginCalculated) {
					// Calculate best margins
					margin = calculateMargins(sizeX, hCount, itemWidth);
					marginCalculated = true;

					if (Gallery.DEBUG)
						System.out.println("margin " + margin); //$NON-NLS-1$
				}
			}

			if (isGroupExpanded(group)) {

				Point s = this.getSize(hCount, vCount, itemWidth, itemHeight,
						minMargin, margin);
				group.height += s.y * animationRatio;

				if (Gallery.DEBUG)
					System.out.println("group.height " + group.height); //$NON-NLS-1$

				group.setData(H_COUNT, new Integer(hCount));
				group.setData(V_COUNT, new Integer(vCount));
				if (Gallery.DEBUG)
					System.out.println("Hnb" + hCount + "Vnb" + vCount); //$NON-NLS-1$//$NON-NLS-2$

				fill = (fillIfSingleColumn && hCount == 1);
			}

		} else {
			// Horizontal
			int sizeY = group.height;
			group.width = getGroupOffset(group);

			Point l = gridLayout(sizeY, countLocal, itemHeight);
			int vCount = l.x;
			int hCount = l.y;
			if (autoMargin && vCount > 0) {
				// Calculate best margins
				margin = calculateMargins(sizeY, vCount, itemHeight);
				marginCalculated = true;

				if (Gallery.DEBUG)
					System.out.println("margin " + margin); //$NON-NLS-1$
			}

			if (isGroupExpanded(group)) {

				Point s = this.getSize(hCount, vCount, itemWidth, itemHeight,
						minMargin, margin);
				group.width += s.x * animationRatio;

				group.setData(H_COUNT, new Integer(hCount));
				group.setData(V_COUNT, new Integer(vCount));

				fill = (fillIfSingleColumn && vCount == 1);

			}
		}

	}

	public void preDraw(GC gc) {
		pre(gc);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.netxms.nebula.widgets.gallery.AbstractGridGroupRenderer#preLayout
	 * (org.eclipse.swt.graphics.GC)
	 */
	public void preLayout(GC gc) {
		this.marginCalculated = false;
		pre(gc);
		super.preLayout(gc);
	}

	/**
	 * Prepare font metrics and title height for both preLayout and preDraw.
	 * 
	 * @param myGc
	 */
	private void pre(GC myGc) {
		GC gc = myGc;
		boolean gcCreated = false;

		if (gc == null) {
			gc = new GC(gallery, SWT.NONE);
			gcCreated = true;
		}

		// Get font height
		gc.setFont(font);
		fontHeight = gc.getFontMetrics().getHeight();

		// Compute title height & grid offset
		titleHeight = fontHeight + 5;

		if (gcCreated)
			gc.dispose();
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
		// Cannot select an item if the group is not expanded
		if (!isGroupExpanded(group))
			return null;

		return super.getItem(group, coords, getGroupOffset(group));
	}

	protected void startGroupAnimation(GalleryItem group, boolean doOpen) {
		if (animation) {
			if (group.getData(DATA_ANIMATION) == null) {
				group.setData(DATA_ANIMATION, new Double(doOpen ? 0 : 1));
			}

			int start, end;
			IMovement movement;
			if (doOpen) {
				start = 0;
				end = 1;
				movement = animationOpenMovement;
			} else {
				start = 1;
				end = 0;
				movement = animationCloseMovement;

			}

			animationRunner.runEffect(new GalleryGroupResizeEffect(group,
					start, end, animationLength, movement, null, null));
		}

	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.netxms.nebula.widgets.gallery.AbstractGridGroupRenderer#mouseDown
	 * (org.netxms.nebula.widgets.gallery.GalleryItem,
	 * org.eclipse.swt.events.MouseEvent, org.eclipse.swt.graphics.Point)
	 */
	public boolean mouseDown(final GalleryItem group, MouseEvent e, Point coords) {

		if (gallery.isVertical()) { // V_SCROLL
			if (coords.y - group.y <= getGroupHeight(group)) {

				if (!isAlwaysExpanded()
						&& coords.x - group.x <= getToggleButtonBounds().x
								+ getToggleButtonBounds().width
						&& coords.x - group.x > getToggleButtonBounds().x) {
					// This is a click on the toggle button : expand/collapse
					// the group
					// Note : if groups are always expanded, there is no toggle
					// button and the test is ignored

					// Toggle expand state
					boolean doOpen = !group.isExpanded();
					startGroupAnimation(group, doOpen);
					group._setExpanded(doOpen, false);

					// Deselect items if group is collapsed
					if (!isGroupExpanded(group)) {
						group.deselectAll();
					}

					// Notify listeners
					gallery.notifyTreeListeners(group, isGroupExpanded(group));

					if (!animation) {
						// Update library
						gallery.updateStructuralValues(group, false);
						gallery.updateScrollBarsProperties();
						gallery.redraw();
					}

				} else {
					// Click on the title bar : Select all children. Only work
					// if multiple items can be selected (SWT.MULTI)
					if (isGroupExpanded(group)
							&& (this.getGallery().getStyle() & SWT.MULTI) > 0) {
						// Cancel previous selection
						if ((e.stateMask & SWT.MOD1) == 0) {
							gallery.deselectAll();
						}

						// Select all and notify
						group.selectAll();
						gallery.notifySelectionListeners(group, gallery
								.indexOf(group), false);
						gallery.redraw();
					}
				}
				return false;
			}
		} else { // H_SCROLL
			if (coords.x - group.x <= getGroupHeight(group)) {

				if (!isAlwaysExpanded()
						&& group.height - coords.y + 5 <= (getToggleButtonBounds().x + getToggleButtonBounds().width)
						&& group.height - coords.y + 5 > getToggleButtonBounds().x) {
					// This is a click on the toggle button : expand/collapse
					// the group
					// Note : if groups are always expanded, there is no toggle
					// button and the test is ignored

					// Toggle expand state
					// Toggle expand state
					boolean doOpen = !group.isExpanded();
					startGroupAnimation(group, doOpen);
					group._setExpanded(doOpen, false);

					// Deselect items if group is collapsed
					if (!isGroupExpanded(group)) {
						group.deselectAll();
					}
					// Notify listeners
					gallery.notifyTreeListeners(group, isGroupExpanded(group));

					// Update library
					if (!animation) {
						gallery.updateStructuralValues(null, false);
						gallery.updateScrollBarsProperties();
						gallery.redraw();
					}

				} else {
					// Click on the title bar : Select all children. Only work
					// if multiple items can be selected (SWT.MULTI)
					if (isGroupExpanded(group)
							&& (this.getGallery().getStyle() & SWT.MULTI) > 0) {

						// Cancel previous selection
						if ((e.stateMask & SWT.MOD1) == 0) {
							gallery.deselectAll();
						}

						// Select all and notify
						group.selectAll();
						gallery.notifySelectionListeners(group, gallery
								.indexOf(group), false);
						gallery.redraw();
					}
				}
				return false;
			}
		}

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
		// If the item is not a group, get its parent
		GalleryItem group = item.getParentItem();
		if (group == null) {
			group = item;
		}

		return super.getSize(item, getGroupOffset(group));
	}

	/**
	 * Get group title text color.
	 * 
	 * @return current color.
	 */
	public Color getTitleForeground() {
		return titleForeground;
	}

	/**
	 * Change group title text color.
	 * 
	 * @param titleColor
	 *            Color or null to revert to default.
	 */
	public void setTitleForeground(Color titleColor) {
		if (titleColor == null) {
			if (gallery == null) {
				throw new IllegalArgumentException(
						"Please associate this renderer with a Gallery before trying to reset foreground defaults"); //$NON-NLS-1$
			}
			titleForeground = gallery.getDisplay().getSystemColor(
					SWT.COLOR_TITLE_FOREGROUND);
		} else {
			this.titleForeground = titleColor;
		}
	}

	public Color getTitleBackground() {
		return titleBackground;
	}

	public void setTitleBackground(Color titleBackground) {
		this.titleBackgroundGradient = false;
		this.titleBackground = titleBackground;
	}

	public void setTitleBackgroundGradient(Color gradientBackground,
			Color gradientForeground) {
		this.titleBackgroundGradient = true;

		if (gradientBackground != null && gradientForeground != null) {
			this.titleBackground = gradientBackground;
			this.titleBackground2 = gradientForeground;
		} else {
			if (gallery == null) {
				throw new IllegalArgumentException(
						"Please associate this renderer with a Gallery before trying to reset background defaults"); //$NON-NLS-1$
			}

			// Default gradient Background
			this.titleBackground = gallery.getDisplay().getSystemColor(
					SWT.COLOR_TITLE_BACKGROUND);
			this.titleBackground2 = gallery.getDisplay().getSystemColor(
					SWT.COLOR_TITLE_BACKGROUND_GRADIENT);
		}
	}

	/**
	 * Returns the font used for drawing the group title or <tt>null</tt> if
	 * system font is used.
	 * 
	 * @return the font
	 */
	public Font getFont() {
		return font;
	}

	/**
	 * Set the font for drawing the group title or <tt>null</tt> to use system
	 * font.
	 * 
	 * @param font
	 *            the font to set
	 */
	public void setFont(Font font) {
		if (this.font != font) {
			this.font = font;
			if (getGallery() != null)
				getGallery().redraw();
		}
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.netxms.nebula.widgets.gallery.AbstractGridGroupRenderer#drawItem
	 * (org.eclipse.swt.graphics.GC, int, boolean,
	 * org.netxms.nebula.widgets.gallery.GalleryItem, int)
	 */
	protected void drawItem(GC gc, int index, boolean selected,
			GalleryItem parent, int offsetY) {

		if (fill) {
			Item item = parent.getItem(index);

			// No item ? return
			if (item == null)
				return;

			GalleryItem gItem = (GalleryItem) item;

			Rectangle area = gallery.getClientArea();

			gItem.x = area.x;
			gItem.y = area.y + gallery.translate;

			gItem.height = area.height;
			gItem.width = area.width;

			gallery.sendPaintItemEvent(item, index, gc, area.x, area.y,
					area.width, area.height);

			if (gallery.getItemRenderer() != null) {
				gallery.getItemRenderer().setSelected(selected);
				gallery.getItemRenderer().draw(gc, gItem, index, area.x,
						area.y, area.width, area.height);
			}

			return;
		}

		super.drawItem(gc, index, selected, parent, offsetY);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.netxms.nebula.widgets.gallery.AbstractGalleryGroupRenderer#
	 * getScrollBarIncrement()
	 */
	public int getScrollBarIncrement() {
		if (fill) {
			if (gallery.isVertical()) {
				// Vertical fill
				return gallery.getClientArea().height;
			}

			// Horizontal fill
			return gallery.getClientArea().width;
		}

		// Standard behavior
		return super.getScrollBarIncrement();
	}

	/**
	 * @see #setFillIfSingleColumn(boolean)
	 * @return
	 */
	public boolean isFillIfSingleColumn() {
		return fillIfSingleColumn;
	}

	/**
	 * <p>
	 * <b>Experimental feature.</b>
	 * </p>
	 * <p>
	 * If set to true, this will enable a special behavior when the items are so
	 * large that only one can fit in the client area. In this case, items are
	 * always resized and centered to fit best in the client area.
	 * </p>
	 * <p>
	 * See bug 266613 : https://bugs.eclipse.org/266613
	 * </p>
	 * 
	 * @param fillIfSingle
	 */
	public void setFillIfSingleColumn(boolean fillIfSingle) {
		this.fillIfSingleColumn = fillIfSingle;
	}

	/**
	 * @see #setMaxImageWidth(int)
	 * @return
	 */
	public int getMaxImageWidth() {
		return maxImageWidth;
	}

	/**
	 * Set the maximum width for a group image in the title bar.
	 * 
	 * @see GalleryItem#setImage(Image)
	 * 
	 * @param imageWidth
	 */
	public void setMaxImageWidth(int imageWidth) {
		this.maxImageWidth = imageWidth;
	}

	/**
	 * @see #setMaxImageHeight(int)
	 * @return
	 */
	public int getMaxImageHeight() {
		return maxImageHeight;
	}

	/**
	 * Set the maximum height for a group image in the title bar.
	 * 
	 * @see GalleryItem#setImage(Image)
	 * 
	 * @param imageHeight
	 */
	public void setMaxImageHeight(int imageHeight) {
		this.maxImageHeight = imageHeight;
	}

	/**
	 * @see #setAnimation(boolean)
	 * @return
	 */
	public boolean isAnimation() {
		return animation;
	}

	/**
	 * Enable animation for group expand/collapse.
	 * 
	 * @see #setAnimationLength(int)
	 * @see #setAnimationOpenMovement(IMovement)
	 * 
	 * @param animation
	 */
	public void setAnimation(boolean animation) {
		this.animation = animation;
	}

	/**
	 * @see #setAnimationLength(int)
	 * @return
	 */
	public int getAnimationLength() {
		return animationLength;
	}

	/**
	 * Set the length of the animation
	 * 
	 * @see #setAnimation(boolean)
	 * @see #setAnimationOpenMovement(IMovement)
	 * 
	 * @param animationLength
	 */
	public void setAnimationLength(int animationLength) {
		this.animationLength = animationLength;
	}

	/**
	 * Get the current movement used for animation
	 * 
	 * @see #setAnimationOpenMovement(IMovement)
	 * 
	 * @return
	 */
	public IMovement getAnimationOpenMovement() {
		return animationOpenMovement;
	}

	/**
	 * @see #setAnimationCloseMovement(IMovement)
	 * @return
	 */
	public IMovement getAnimationCloseMovement() {
		return animationCloseMovement;
	}

	/**
	 * 
	 * Set the movement used for open animation.
	 * 
	 * @see #setAnimation(boolean)
	 * @see #setAnimationLength(int)
	 * 
	 * @param animationMovement
	 */
	public void setAnimationOpenMovement(IMovement animationMovement) {
		this.animationOpenMovement = animationMovement;
	}

	/**
	 * 
	 * Set the movement used for close animation.
	 * 
	 * @see #setAnimation(boolean)
	 * @see #setAnimationLength(int)
	 * @param animationMovement
	 */
	public void setAnimationCloseMovement(IMovement animationMovement) {
		this.animationCloseMovement = animationMovement;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.netxms.nebula.widgets.gallery.AbstractGridGroupRenderer#isGroupExpanded
	 * (org.netxms.nebula.widgets.gallery.GalleryItem)
	 */
	protected boolean isGroupExpanded(GalleryItem item) {

		if (animation) {
			if (item.getData(DefaultGalleryGroupRenderer.DATA_ANIMATION) != null)
				return true;
		}
		return super.isGroupExpanded(item);
	}

	public boolean isTitleBackgroundGradient() {
		return titleBackgroundGradient;
	}

	public Color getTitleBackground2() {
		return titleBackground2;
	}

}
