/*******************************************************************************
 * Copyright (c) 2006-2009 Nicolas Richeton.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors :
 *    Nicolas Richeton (nicolas.richeton@gmail.com) - initial API and implementation
 *    Tom Schindl      (tom.schindl@bestsolution.at) - fix for bug 174933
 *******************************************************************************/

package org.netxms.nebula.widgets.gallery;

import java.lang.reflect.Array;
import java.util.ArrayList;

import org.eclipse.swt.SWT;
import org.eclipse.swt.SWTException;
import org.eclipse.swt.events.ControlAdapter;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.KeyEvent;
import org.eclipse.swt.events.KeyListener;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.events.TreeListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Item;
import org.eclipse.swt.widgets.ScrollBar;
import org.eclipse.swt.widgets.TypedListener;

/**
 * <p>
 * SWT Widget that displays an image gallery<br/>
 * see http://www.eclipse.org/nebula/widgets/gallery/gallery.php<br/>
 * This widget requires jdk-1.4+
 * </p>
 * <p>
 * Style <code>VIRTUAL</code> is used to create a <code>Gallery</code> whose
 * <code>GalleryItem</code>s are to be populated by the client on an on-demand
 * basis instead of up-front. This can provide significant performance
 * improvements for galleries that are very large or for which
 * <code>GalleryItem</code> population is expensive (for example, retrieving
 * values from an external source).
 * </p>
 * <p>
 * Here is an example of using a <code>Gallery</code> with style
 * <code>VIRTUAL</code>: <code><pre>
 * final Gallery gallery = new Gallery(parent, SWT.VIRTUAL | V_SCROLL | SWT.BORDER);
 * gallery.setGroupRenderer(new DefaultGalleryGroupRenderer());
 * gallery.setItemRenderer(new DefaultGalleryItemRenderer());
 * gallery.setItemCount(1000000);
 * gallery.addListener(SWT.SetData, new Listener() {
 * 	public void handleEvent(Event event) {
 * 		GalleryItem item = (GalleryItem) event.item;
 * 		int index = gallery.indexOf(item);
 * 		item.setText(&quot;Item &quot; + index);
 * 		System.out.println(item.getText());
 * 	}
 * });
 * </pre></code>
 * </p>
 * <p>
 * <dl>
 * <dt><b>Styles:</b></dt>
 * <dd>SINGLE, MULTI, VIRTUAL, V_SCROLL, H_SCROLL</dd>
 * </dl>
 * </p>
 * <p>
 * Note: Only one of the styles SINGLE and MULTI may be specified.
 * </p>
 * <p>
 * Note: Only one of the styles V_SCROLL and H_SCROLL may be specified.
 * </p>
 * <p>
 * <dl>
 * <dt><b>Events:</b></dt>
 * <dd>Selection, DefaultSelection, SetData, PaintItem</dd>
 * </dl>
 * </p>
 * <p>
 * NOTE: THIS WIDGET AND ITS API ARE STILL UNDER DEVELOPMENT.
 * </p>
 * 
 * @author Nicolas Richeton (nicolas.richeton@gmail.com)
 * @contributor Peter Centgraf (bugs 212071, 212073)
 * @contributor Robert Handschmann (bug 215817)
 */

public class Gallery extends Canvas {

	private static final String BUG_PLATFORM_LINUX_GTK_174932 = "gtk"; //$NON-NLS-1$

	/**
	 * Used to enable debug logging in the Gallery widget.
	 */
	protected static boolean DEBUG = false;

	GalleryItem[] items = null;

	private GalleryItem[] selection = null;

	private int[] selectionIndices = null;

	/**
	 * Virtual mode flag.
	 */
	boolean virtual = false;

	/**
	 * Ultra virtual : non visible groups are not initialized.
	 */
	boolean virtualGroups = false;
	boolean virtualGroupsCompatibilityMode = false;
	int virtualGroupDefaultItemCount = 10;

	/**
	 * Scrolling direction flag. True : V_SCROLL, false : H_SCROLL.
	 */
	boolean vertical = true;

	/**
	 * Multi-selection flag
	 */
	boolean multi = false;

	/**
	 * Image quality : interpolation
	 */
	int interpolation = SWT.HIGH;

	/**
	 * Image quality : antialias
	 */
	int antialias = SWT.ON;

	// Internals

	private int gHeight = 0;

	private int gWidth = 0;

	int lastIndexOf = 0;

	/**
	 * Keeps track of the last selected item. This is necessary to support
	 * "Shift+Mouse button" where we have to select all items between the
	 * previous and the current item and keyboard navigation.
	 */
	protected GalleryItem lastSingleClick = null;

	/**
	 * Current translation (scroll bar position). Can be used by renderer during
	 * paint.
	 */
	protected int translate = 0;

	/**
	 * Low quality on user action : decrease drawing quality on scrolling or
	 * resize. (faster)
	 */
	boolean lowQualityOnUserAction = false;
	protected int lastTranslateValue = 0;
	protected int lastControlWidth = 0;
	protected int lastControlHeight = 0;
	protected int lastContentHeight = 0;
	protected int lastContentWidth = 0;
	protected int higherQualityDelay = 500;

	/**
	 * Keep track of processing the current mouse event.
	 */
	private boolean mouseClickHandled = false;

	/**
	 * Background color, if Control#getBackground() is not used.
	 * 
	 * @see Gallery#useControlColors
	 */
	private Color backgroundColor;

	/**
	 * Foreground color, if Control#getForeground() is not used.
	 * 
	 * @see Gallery#useControlColors
	 */
	private Color foregroundColor;

	/**
	 * If set to true, the gallery will get colors from parent Control. This may
	 * generate more objects and slightly slow down the application. See Bug
	 * 279822 : https://bugs.eclipse.org/bugs/show_bug.cgi?id=279822
	 */
	private boolean useControlColors = false;

	AbstractGalleryItemRenderer itemRenderer;

	AbstractGalleryGroupRenderer groupRenderer;

	/**
	 * Return the number of root-level items in the receiver. Does not include
	 * children.
	 * 
	 * @return
	 */
	public int getItemCount() {
		checkWidget();

		if (items == null)
			return 0;

		return items.length;
	}

	/**
	 * Sets the number of root-level items contained in the receiver. Only work
	 * in VIRTUAL mode.
	 * 
	 * @return
	 */
	public void setItemCount(int count) {
		checkWidget();

		if (DEBUG)
			System.out.println("setCount" + count); //$NON-NLS-1$

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

		updateStructuralValues(null, false);
		this.updateScrollBarsProperties();
		redraw();

	}

	/**
	 * Get current item renderer
	 * 
	 * @return
	 */
	public AbstractGalleryItemRenderer getItemRenderer() {
		checkWidget();
		return itemRenderer;
	}

	/**
	 * Set item receiver. Usually, this does not trigger gallery update. redraw
	 * must be called right after setGroupRenderer to reflect this change.
	 * 
	 * @param itemRenderer
	 */
	public void setItemRenderer(AbstractGalleryItemRenderer itemRenderer) {
		checkWidget();
		this.itemRenderer = itemRenderer;

		if (this.itemRenderer != null)
			this.itemRenderer.setGallery(this);

		redraw();
	}

	/**
	 * Adds the listener to the collection of listeners who will be notified
	 * when the receiver's selection changes, by sending it one of the messages
	 * defined in the <code>SelectionListener</code> interface.
	 * <p>
	 * When <code>widgetSelected</code> is called, the item field of the event
	 * object is valid.
	 * </p>
	 * 
	 * @param listener
	 *            the listener which should be notified
	 * 
	 * @exception IllegalArgumentException
	 *                <ul>
	 *                <li>ERROR_NULL_ARGUMENT - if the listener is null</li>
	 *                </ul>
	 * @exception SWTException
	 *                <ul>
	 *                <li>ERROR_WIDGET_DISPOSED - if the receiver has been
	 *                disposed</li>
	 *                <li>ERROR_THREAD_INVALID_ACCESS - if not called from the
	 *                thread that created the receiver</li>
	 *                </ul>
	 * 
	 * @see SelectionListener
	 * @see #removeSelectionListener
	 * @see SelectionEvent
	 */
	public void addSelectionListener(SelectionListener listener) {
		checkWidget();
		if (listener == null)
			SWT.error(SWT.ERROR_NULL_ARGUMENT);

		TypedListener typedListener = new TypedListener(listener);
		addListener(SWT.Selection, typedListener);
		addListener(SWT.DefaultSelection, typedListener);
	}

	/**
	 * Removes the listener from the collection of listeners who will be
	 * notified when the receiver's selection changes.
	 * 
	 * @param listener
	 *            the listener which should no longer be notified
	 * 
	 * @exception IllegalArgumentException
	 *                <ul>
	 *                <li>ERROR_NULL_ARGUMENT - if the listener is null</li>
	 *                </ul>
	 * @exception SWTException
	 *                <ul>
	 *                <li>ERROR_WIDGET_DISPOSED - if the receiver has been
	 *                disposed</li>
	 *                <li>ERROR_THREAD_INVALID_ACCESS - if not called from the
	 *                thread that created the receiver</li>
	 *                </ul>
	 * 
	 * @see SelectionListener
	 * @see #addSelectionListener(SelectionListener)
	 */
	public void removeSelectionListener(SelectionListener listener) {
		checkWidget();
		removeListener(SWT.Selection, listener);
		removeListener(SWT.DefaultSelection, listener);
	}

	/**
	 * Removes the listener from the collection of listeners who will be
	 * notified when items in the receiver are expanded or collapsed.
	 * 
	 * @param listener
	 */
	public void removeTreeListener(SelectionListener listener) {
		checkWidget();
		removeListener(SWT.Expand, listener);
	}

	/**
	 * Adds the listener to the collection of listeners who will be notified
	 * when an item in the receiver is expanded or collapsed by sending it one
	 * of the messages defined in the TreeListener interface.
	 * 
	 * @param listener
	 */
	public void addTreeListener(TreeListener listener) {
		checkWidget();
		if (listener == null)
			SWT.error(SWT.ERROR_NULL_ARGUMENT);
		addListener(SWT.Expand, new TypedListener(listener));
	}

	/**
	 * Send SWT.PaintItem for one item.
	 * 
	 * @param item
	 * @param index
	 * @param gc
	 * @param x
	 * @param y
	 */
	protected void sendPaintItemEvent(Item item, int index, GC gc, int x,
			int y, int width, int height) {

		Event e = new Event();
		e.item = item;
		e.type = SWT.PaintItem;
		e.index = index;
		// TODO: Does clipping need to be set ?
		// gc.setClipping(x, y, width, height);
		e.gc = gc;
		e.x = x;
		e.y = y;
		e.width = width;
		e.height = height;
		this.notifyListeners(SWT.PaintItem, e);
	}

	/**
	 * #see {@link #setLowQualityOnUserAction(boolean)}
	 * 
	 * @return
	 */
	public boolean isLowQualityOnUserAction() {
		return lowQualityOnUserAction;
	}

	/**
	 * If set to true, the gallery will disable antialiasing and interpolation
	 * while the user is resizing or scrolling the gallery. This enables faster
	 * redraws at the cost of lower image quality. When every redraw is finished
	 * a last one will be issued using the default (higher) quality.
	 * 
	 * @see #setHigherQualityDelay(int)
	 * @param lowQualityOnUserAction
	 */
	public void setLowQualityOnUserAction(boolean lowQualityOnUserAction) {
		this.lowQualityOnUserAction = lowQualityOnUserAction;
	}

	/**
	 * @see #setHigherQualityDelay(int)
	 * @return
	 */
	public int getHigherQualityDelay() {
		return higherQualityDelay;
	}

	/**
	 * Set the delay after the last user action before the redraw at higher
	 * quality is triggered
	 * 
	 * @see #setLowQualityOnUserAction(boolean)
	 * @param higherQualityDelay
	 */
	public void setHigherQualityDelay(int higherQualityDelay) {
		this.higherQualityDelay = higherQualityDelay;
	}

	/**
	 * @see #setInterpolation(int)
	 * @return
	 */
	public int getInterpolation() {
		return interpolation;
	}

	/**
	 * Sets the gallery's interpolation setting to the parameter, which must be
	 * one of <code>SWT.DEFAULT</code>, <code>SWT.NONE</code>,
	 * <code>SWT.LOW</code> or <code>SWT.HIGH</code>.
	 * 
	 * @param interpolation
	 */
	public void setInterpolation(int interpolation) {
		this.interpolation = interpolation;
	}

	/**
	 * @see #setAntialias(int)
	 * @return
	 */
	public int getAntialias() {
		return antialias;
	}

	/**
	 * 
	 * Sets the gallery's anti-aliasing value to the parameter, which must be
	 * one of <code>SWT.DEFAULT</code>, <code>SWT.OFF</code> or
	 * <code>SWT.ON</code>.
	 * 
	 * @param antialias
	 */
	public void setAntialias(int antialias) {
		this.antialias = antialias;
	}

	/**
	 * Send a selection event for a gallery item
	 * 
	 * @param item
	 */
	protected void notifySelectionListeners(GalleryItem item, int index,
			boolean isDefault) {

		Event e = new Event();
		e.widget = this;
		e.item = item;
		if (item != null) {
			e.data = item.getData();
		}
		// TODO: report index
		// e.index = index;
		try {
			if (isDefault) {
				notifyListeners(SWT.DefaultSelection, e);
			} else {
				notifyListeners(SWT.Selection, e);
			}
		} catch (RuntimeException ex) {
			ex.printStackTrace();
		}
	}

	/**
	 * Send an Expand event for a GalleryItem
	 * 
	 * @param item
	 * @param index
	 */
	protected void notifyTreeListeners(GalleryItem item, boolean state) {

		Event e = new Event();
		e.widget = this;
		e.item = item;
		if (item != null) {
			e.data = item.getData();
		}
		// TODO: report index
		// e.index = index;
		try {
			notifyListeners(SWT.Expand, e);
		} catch (RuntimeException ex) {
			ex.printStackTrace();
		}
	}

	/**
	 * Create a Gallery
	 * 
	 * 
	 * @param parent
	 * @param style
	 *            - SWT.VIRTUAL switches in virtual mode. <br/>
	 *            SWT.V_SCROLL add vertical slider and switches to vertical
	 *            mode. <br/>
	 *            SWT.H_SCROLL add horizontal slider and switches to horizontal
	 *            mode. <br/>
	 *            if both V_SCROLL and H_SCROLL are specified, the gallery is in
	 *            vertical mode by default. Mode can be changed afterward using
	 *            setVertical<br/>
	 *            SWT.MULTI allows only several items to be selected at the same
	 *            time.
	 */
	public Gallery(Composite parent, int style) {
		super(parent, style | SWT.DOUBLE_BUFFERED);
		virtual = (style & SWT.VIRTUAL) > 0;
		vertical = (style & SWT.V_SCROLL) > 0;
		multi = (style & SWT.MULTI) > 0;
		setForeground(getDisplay().getSystemColor(SWT.COLOR_LIST_FOREGROUND));

		// Add listeners : redraws, mouse and keyboard
		_addDisposeListeners();
		_addResizeListeners();
		_addPaintListeners();
		_addScrollBarsListeners();
		_addMouseListeners();
		_addKeyListeners();

		// Set defaults
		_setDefaultRenderers();

		// Layout
		updateStructuralValues(null, false);
		updateScrollBarsProperties();
		redraw();
	}

	private void _setDefaultRenderers() {
		// Group renderer
		DefaultGalleryGroupRenderer gr = new DefaultGalleryGroupRenderer();
		gr.setMinMargin(2);
		gr.setItemHeight(56);
		gr.setItemWidth(72);
		gr.setAutoMargin(true);
		gr.setGallery(this);
		this.groupRenderer = gr;

		// Item renderer
		this.itemRenderer = new DefaultGalleryItemRenderer();
		itemRenderer.setGallery(this);
	}

	/**
	 * Add internal dispose listeners to this gallery.
	 */
	private void _addDisposeListeners() {
		this.addDisposeListener(new DisposeListener() {
			public void widgetDisposed(DisposeEvent e) {
				onDispose();
			}
		});
	}

	/**
	 * Add internal paint listeners to this gallery.
	 */
	private void _addPaintListeners() {
		addPaintListener(new PaintListener() {
			public void paintControl(PaintEvent event) {
				onPaint(event.gc);
			}
		});
	}

	/**
	 * Add internal resize listeners to this gallery.
	 */
	private void _addResizeListeners() {
		addControlListener(new ControlAdapter() {
			public void controlResized(ControlEvent event) {
				updateStructuralValues(null, true);
				updateScrollBarsProperties();
				redraw();
			}
		});
	}

	/**
	 * Add internal scrollbars listeners to this gallery.
	 */
	private void _addScrollBarsListeners() {
		// Vertical bar
		ScrollBar verticalBar = getVerticalBar();
		if (verticalBar != null) {
			verticalBar.addSelectionListener(new SelectionAdapter() {

				public void widgetSelected(SelectionEvent event) {
					if (vertical)
						scrollVertical();
				}
			});
		}

		// Horizontal bar

		ScrollBar horizontalBar = getHorizontalBar();
		if (horizontalBar != null) {
			horizontalBar.addSelectionListener(new SelectionAdapter() {

				public void widgetSelected(SelectionEvent event) {
					if (!vertical)
						scrollHorizontal();
				}
			});
		}

	}

	private void _addKeyListeners() {
		this.addKeyListener(new KeyListener() {

			public void keyPressed(KeyEvent e) {

				switch (e.keyCode) {
				case SWT.ARROW_LEFT:
				case SWT.ARROW_RIGHT:
				case SWT.ARROW_UP:
				case SWT.ARROW_DOWN:
				case SWT.PAGE_UP:
				case SWT.PAGE_DOWN:
				case SWT.HOME:
				case SWT.END:
					GalleryItem newItem = groupRenderer.getNextItem(
							lastSingleClick, e.keyCode);

					if (newItem != null) {
						_deselectAll(false);
						setSelected(newItem, true, true);
						lastSingleClick = newItem;
						_showItem(newItem);
						redraw();
					}

					break;
				case SWT.CR:
					GalleryItem[] selection = getSelection();
					GalleryItem item = null;
					if (selection != null && selection.length > 0) {
						item = selection[0];
					}

					notifySelectionListeners(item, 0, true);
					break;
				}
			}

			public void keyReleased(KeyEvent e) {
				// Nothing yet.
			}

		});
	}

	/**
	 * Scroll the Gallery in order to make 'item' visible.
	 * 
	 * @param item
	 *            Item to show
	 */
	public void showItem(GalleryItem item) {
		this.checkWidget();
		this._showItem(item);
	}

	void _showItem(GalleryItem item) {
		int y;
		int height;
		Rectangle rect = groupRenderer.getSize(item);
		if (rect == null) {
			return;
		}

		if (vertical) {
			y = rect.y;
			height = rect.height;
			if (y < translate) {
				translate = y;
			} else if (translate + this.getClientArea().height < y + height) {
				translate = y + height - this.getClientArea().height;
			}

		} else {
			y = rect.x;
			height = rect.width;

			if (y < translate) {
				translate = y;
			} else if (translate + this.getClientArea().width < y + height) {
				translate = y + height - this.getClientArea().width;
			}
		}

		this.updateScrollBarsProperties();
		redraw();

	}

	/**
	 * Add internal mouse listeners to this gallery.
	 */
	private void _addMouseListeners() {
		addMouseListener(new MouseListener() {

			public void mouseDoubleClick(MouseEvent e) {
				onMouseDoubleClick(e);
			}

			public void mouseDown(MouseEvent e) {
				onMouseDown(e);
			}

			public void mouseUp(MouseEvent e) {
				onMouseUp(e);
			}

		});
	}

	private void select(int from, int to) {
		for (int i = from; i <= to; i++) {
			GalleryItem item = getItem(i);
			this._addSelection(item);
			item._selectAll();

		}
	}

	private void select(GalleryItem from, GalleryItem to) {
		GalleryItem fromParent = from.getParentItem();
		GalleryItem toParent = to.getParentItem();

		if (fromParent == toParent) {

			if (fromParent == null) {
				int fromIndex = indexOf(from);
				int toIndex = indexOf(to);
				select(fromIndex, toIndex);
			} else {
				int fromIndex = fromParent.indexOf(from);
				int toIndex = toParent.indexOf(to);
				fromParent.select(fromIndex, toIndex);
			}
		} else {
			int fromParentIndex = indexOf(fromParent);
			int toParentIndex = indexOf(toParent);
			int fromIndex = fromParent.indexOf(from);
			int toIndex = toParent.indexOf(to);

			fromParent.select(fromIndex, fromParent.getItemCount() - 1);
			for (int i = fromParentIndex + 1; i < toParentIndex; i++) {
				getItem(i)._selectAll();
			}
			toParent.select(0, toIndex);

		}
		this.notifySelectionListeners(to, indexOf(to), false);
		redraw();
	}

	private boolean getOrder(GalleryItem before, GalleryItem after) {

		if (before == null || after == null)
			return true;

		GalleryItem newParent = before.getParentItem();
		GalleryItem oldParent = after.getParentItem();

		int beforeParentIndex = indexOf(newParent);
		int afterParentIndex = indexOf(oldParent);

		if (newParent == oldParent) {
			int newParentIndex;
			int oldParentIndex;
			if (newParent == null) {
				newParentIndex = indexOf(before);
				oldParentIndex = indexOf(after);

			} else {
				newParentIndex = newParent.indexOf(before);
				oldParentIndex = newParent.indexOf(after);
			}
			return (newParentIndex < oldParentIndex);
		}

		return beforeParentIndex < afterParentIndex;
	}

	/**
	 * Toggle item selection status
	 * 
	 * @param item
	 *            Item which state is to be changed.
	 * @param selected
	 *            true is the item is now selected, false if it is now
	 *            unselected.
	 * @param notifyListeners
	 *            If true, a selection event will be sent to all the current
	 *            selection listeners.
	 * 
	 */
	protected void setSelected(GalleryItem item, boolean selected,
			boolean notifyListeners) {
		if (selected) {
			if (!isSelected(item)) {
				_addSelection(item);

			}

		} else {
			if (isSelected(item)) {
				_removeSelection(item);
			}
		}

		// Notify listeners if necessary.
		if (notifyListeners) {

			GalleryItem notifiedItem = null;

			if (item != null && selected) {
				notifiedItem = item;
			} else {
				if (selection != null && selection.length > 0) {
					notifiedItem = selection[selection.length - 1];
				}
			}

			int index = -1;
			if (notifiedItem != null) {
				index = indexOf(notifiedItem);
			}

			notifySelectionListeners(notifiedItem, index, false);
		}

	}

	protected void _addSelection(GalleryItem item) {

		if (item == null)
			return;

		if (this.isSelected(item))
			return;

		// Deselect all items is multi selection is disabled
		if (!multi) {
			_deselectAll(false);
		}

		if (item.getParentItem() != null) {
			item.getParentItem()._addSelection(item);
		} else {
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

		if (selection == null) {
			selection = new GalleryItem[1];
		} else {
			GalleryItem[] oldSelection = selection;
			selection = new GalleryItem[oldSelection.length + 1];
			System
					.arraycopy(oldSelection, 0, selection, 0,
							oldSelection.length);
		}
		selection[selection.length - 1] = item;

	}

	private void _removeSelection(GalleryItem item) {

		if (item.getParentItem() == null)
			selectionIndices = _arrayRemoveItem(selectionIndices,
					_arrayIndexOf(selectionIndices, _indexOf(item)));
		else
			_removeSelection(item.getParentItem(), item);

		int index = _arrayIndexOf(selection, item);
		if (index == -1)
			return;

		selection = (GalleryItem[]) _arrayRemoveItem(selection, index);

	}

	protected void _removeSelection(GalleryItem parent, GalleryItem item) {
		parent.selectionIndices = _arrayRemoveItem(parent.selectionIndices,
				_arrayIndexOf(parent.selectionIndices, _indexOf(parent, item)));
	}

	protected boolean isSelected(GalleryItem item) {

		if (item == null)
			return false;

		if (item.getParentItem() != null) {
			return item.getParentItem().isSelected(item);
		}

		if (selectionIndices == null)
			return false;

		int index = indexOf(item);
		for (int i = 0; i < selectionIndices.length; i++) {
			if (selectionIndices[i] == index)
				return true;
		}

		return false;
	}

	/**
	 * Deselects all items.
	 */
	public void deselectAll() {
		checkWidget();
		_deselectAll(false);

		redraw();
	}

	/**
	 * Deselects all items and send selection event depending on parameter.
	 * 
	 * @param notifyListeners
	 *            If true, a selection event will be sent to all the current
	 *            selection listeners.
	 */
	protected void _deselectAll(boolean notifyListeners) {

		if (DEBUG)
			System.out.println("clear"); //$NON-NLS-1$

		this.selection = null;
		this.selectionIndices = null;

		if (items == null)
			return;
		for (int i = 0; i < items.length; i++) {
			if (items[i] != null)
				items[i]._deselectAll();
		}

		// Notify listeners if necessary.
		if (notifyListeners)
			notifySelectionListeners(null, -1, false);
	}

	void onMouseDoubleClick(MouseEvent e) {
		if (DEBUG)
			System.out.println("Mouse Double Click"); //$NON-NLS-1$

		GalleryItem item = getItem(new Point(e.x, e.y));
		if (item != null) {
			notifySelectionListeners(item, 0, true);
		}
		mouseClickHandled = true;
	}

	void onMouseUp(MouseEvent e) {
		if (DEBUG)
			System.out.println("onMouseUp"); //$NON-NLS-1$

		if (mouseClickHandled) {
			if (DEBUG) {
				System.out.println("onMouseUp : mouse event already handled"); //$NON-NLS-1$
			}
			return;
		}

		if (e.button == 1) {
			GalleryItem item = getItem(new Point(e.x, e.y));
			if (item == null)
				return;

			if ((e.stateMask & SWT.MOD1) > 0) {
				onMouseHandleLeftMod1(e, item, false, true);
			} else if ((e.stateMask & SWT.SHIFT) > 0) {
				onMouseHandleLeftShift(e, item, false, true);
			} else {
				onMouseHandleLeft(e, item, false, true);
			}
		}
	}

	/**
	 * Clean up the Gallery and renderers on dispose.
	 */
	void onDispose() {
		// Remove items if not Virtual.
		if (!virtual)
			removeAll();

		// Dispose renderers
		if (itemRenderer != null)
			itemRenderer.dispose();

		if (groupRenderer != null)
			groupRenderer.dispose();

	}

	void onMouseDown(MouseEvent e) {
		if (DEBUG)
			System.out.println("Mouse down "); //$NON-NLS-1$

		mouseClickHandled = false;

		if (!_mouseDown(e)) {
			mouseClickHandled = true;
			return;
		}

		GalleryItem item = getItem(new Point(e.x, e.y));

		if (e.button == 1) {

			if (item == null) {
				_deselectAll(true);
				redraw();
				mouseClickHandled = true;
				lastSingleClick = null;
			} else {
				if ((e.stateMask & SWT.MOD1) > 0) {
					onMouseHandleLeftMod1(e, item, true, false);
				} else if ((e.stateMask & SWT.SHIFT) > 0) {
					onMouseHandleLeftShift(e, item, true, false);
				} else {
					onMouseHandleLeft(e, item, true, false);
				}
			}
		} else if (e.button == 3) {
			onMouseHandleRight(e, item, true, false);
		}
	}

	private void onMouseHandleLeftMod1(MouseEvent e, GalleryItem item,
			boolean down, boolean up) {
		if (up) {
			// if (lastSingleClick != null) {
			if (item != null) {
				if (DEBUG)
					System.out.println("setSelected : inverse"); //$NON-NLS-1$
				setSelected(item, !isSelected(item), true);
				lastSingleClick = item;
				redraw();
			}
			// }
		}
	}

	private void onMouseHandleLeftShift(MouseEvent e, GalleryItem item,
			boolean down, boolean up) {
		if (up) {
			if (lastSingleClick != null) {
				_deselectAll(false);

				if (getOrder(item, lastSingleClick))
					select(item, lastSingleClick);
				else
					select(lastSingleClick, item);
			}
		}
	}

	void onMouseHandleLeft(MouseEvent e, GalleryItem item, boolean down,
			boolean up) {
		if (down) {
			if (!isSelected(item)) {
				_deselectAll(false);

				if (DEBUG)
					System.out.println("setSelected"); //$NON-NLS-1$
				setSelected(item, true, true);

				lastSingleClick = item;
				redraw();
				mouseClickHandled = true;
			}
		} else if (up) {
			if (item == null) {
				_deselectAll(true);
			} else {
				if (DEBUG)
					System.out.println("setSelected"); //$NON-NLS-1$

				_deselectAll(false);
				setSelected(item, true, lastSingleClick != item);
				lastSingleClick = item;
			}
			redraw();
		}
	}

	/**
	 * Handle right click.
	 * 
	 * @param e
	 * @param item
	 *            : The item which is under the cursor or null
	 * @param down
	 * @param up
	 */
	void onMouseHandleRight(MouseEvent e, GalleryItem item, boolean down,
			boolean up) {
		if (down) {
			if (DEBUG)
				System.out.println("right click"); //$NON-NLS-1$

			if (item != null && !isSelected(item)) {
				_deselectAll(false);
				setSelected(item, true, true);
				redraw();
				mouseClickHandled = true;
			}
		}

	}

	void onPaint(GC gc) {
		if (DEBUG)
			System.out.println("paint"); //$NON-NLS-1$

		boolean lowQualityPaint = lowQualityOnUserAction
				&& (translate != lastTranslateValue || (lastControlWidth != getSize().x
						|| lastControlHeight != getSize().y
						|| lastContentHeight != this.gHeight || lastContentWidth != this.gWidth));
		try {
			// Linux-GTK Bug 174932
			if (!SWT.getPlatform().equals(BUG_PLATFORM_LINUX_GTK_174932)) {
				gc.setAdvanced(true);
			}

			if (gc.getAdvanced()) {
				if (lowQualityPaint) {
					gc.setAntialias(SWT.OFF);
					gc.setInterpolation(SWT.OFF);
				} else {
					gc.setAntialias(antialias);
					gc.setInterpolation(interpolation);
				}
			}
			// End of Bug 174932

			Rectangle clipping = gc.getClipping();

			gc.setBackground(this.getBackground());
			drawBackground(gc, clipping.x, clipping.y, clipping.width,
					clipping.height);

			int[] indexes = getVisibleItems(clipping);

			if (indexes != null && indexes.length > 0) {

				// Call preDraw for optimization
				if (groupRenderer != null)
					groupRenderer.preDraw(gc);
				if (itemRenderer != null)
					itemRenderer.preDraw(gc);

				for (int i = indexes.length - 1; i >= 0; i--) {
					if (DEBUG)
						System.out.println("Drawing group " + indexes[i]); //$NON-NLS-1$

					_drawGroup(gc, indexes[i]);
				}

				// Call postDraw for optimization / cleanup
				if (groupRenderer != null)
					groupRenderer.postDraw(gc);
				if (itemRenderer != null)
					itemRenderer.postDraw(gc);
			}
		} catch (Exception e) {
			// We can't let onPaint throw an exception because unexpected
			// results may occur in SWT.
			e.printStackTrace();
		}

		// When lowQualityOnUserAction is enabled, keep last state and wait
		// before updating with a higher quality
		if (lowQualityOnUserAction) {
			lastTranslateValue = translate;
			lastControlWidth = getSize().x;
			lastControlHeight = getSize().y;
			lastContentHeight = gHeight;
			lastContentWidth = gWidth;

			if (lowQualityPaint) {
				// Calling timerExec with the same object just delays the
				// execution (doesn't run twice)
				Display.getCurrent().timerExec(higherQualityDelay, redrawTimer);
			}
		}
	}

	RedrawTimer redrawTimer = new RedrawTimer();

	protected class RedrawTimer implements Runnable {
		public void run() {
			redraw();
		}
	}

	private int[] getVisibleItems(Rectangle clipping) {

		if (items == null)
			return null;

		int start = vertical ? (clipping.y + translate)
				: (clipping.x + translate);

		int end = vertical ? (clipping.y + clipping.height + translate)
				: (clipping.x + clipping.width + translate);

		ArrayList<Object> al = new ArrayList<Object>();
		int index = 0;
		GalleryItem item = null;
		while (index < items.length) {
			if (virtualGroups) {
				item = _getItem(index, false);
			} else {
				item = _getItem(index);
			}
			if ((vertical ? item.y : item.x) > end)
				break;

			if ((vertical ? (item.y + item.height) : (item.x + item.width)) >= start)
				al.add(new Integer(index));

			index++;
		}

		int[] result = new int[al.size()];

		for (int i = 0; i < al.size(); i++)
			result[i] = ((Integer) al.get(i)).intValue();

		return result;
	}

	/**
	 * Refresh item by firering SWT.SetData.
	 * <p>
	 * Currently not implemented.
	 * </p>
	 * 
	 * @param index
	 */
	public void refresh(int index) {
		checkWidget();
		if (index < getItemCount()) {
			// TODO: refresh
		}
	}

	/**
	 * Redraw the item given as parameter.
	 * 
	 * @param item
	 */
	public void redraw(GalleryItem item) {
		checkWidget();

		// Redraw only the item's bounds
		Rectangle bounds = item.getBounds();
		redraw(bounds.x, bounds.y, bounds.width, bounds.height, true);
	}

	/**
	 * Handle the drawing of root items
	 * 
	 * @param gc
	 * @param index
	 */
	private void _drawGroup(GC gc, int index) {
		GalleryItem item = null;

		if (virtualGroups) {
			// Ultra virtual : when a group is about to be drawn, initialize it
			// and update gallery layout according to the new size
			item = _getItem(index, false);
			if (item.isUltraLazyDummy()) {
				boolean updateLocation = (vertical && item.y < translate)
						|| (!vertical && item.x < translate);
				int oldSize = item.height;
				item = _getItem(index, true);

				// Compatibility mode : ensure all previous items are already
				// initialized
				if (virtualGroupsCompatibilityMode) {
					for (int i = 0; i < index; i++) {
						_getItem(i);
					}
				}

				updateStructuralValues(item, false);

				if (DEBUG) {
					System.out.println("old" + oldSize + " new " + item.height //$NON-NLS-1$//$NON-NLS-2$
							+ " translate " + translate); //$NON-NLS-1$
				}

				if (updateLocation) {
					translate += (item.height - oldSize);
					if (DEBUG) {
						System.out.println("updated to : " + translate); //$NON-NLS-1$
					}
				}
				this.updateScrollBarsProperties();
				redraw();
			}
		} else {
			// Default behavior : get the item with no special handling.
			item = getItem(index);
		}

		if (item == null)
			return;

		// update item attributes
		this.groupRenderer.setExpanded(item.isExpanded());

		// Drawing area
		int x = this.vertical ? item.x : item.x - this.translate;
		int y = this.vertical ? item.y - translate : item.y;

		Rectangle clipping = gc.getClipping();
		Rectangle previousClipping = new Rectangle(clipping.x, clipping.y,
				clipping.width, clipping.height);

		clipping.intersect(new Rectangle(x, y, item.width, item.height));
		gc.setClipping(clipping);
		// Draw group
		this.groupRenderer.draw(gc, item, x, y, clipping.x, clipping.y,
				clipping.width, clipping.height);

		gc.setClipping(previousClipping);
	}

	/**
	 * If table is virtual and item at pos i has not been set, call the callback
	 * listener to set its value.
	 * 
	 * @return
	 */
	private void updateItem(GalleryItem parentItem, int i, boolean create) {
		if (virtual) {

			GalleryItem galleryItem;
			if (parentItem == null) {
				// Parent is the Gallery widget
				galleryItem = items[i];

				if (galleryItem == null
						|| (virtualGroups && galleryItem.isUltraLazyDummy() && create)) {

					if (DEBUG) {
						System.out.println("Virtual/creating item "); //$NON-NLS-1$
					}

					galleryItem = new GalleryItem(this, SWT.NONE, i, false);
					items[i] = galleryItem;

					if (virtualGroups && !create) {
						galleryItem.setItemCount(virtualGroupDefaultItemCount);
						galleryItem.setUltraLazyDummy(true);
						galleryItem.setExpanded(true);
					} else {
						setData(galleryItem, i);
					}
				}
			} else {
				// Parent is another GalleryItem
				galleryItem = parentItem.items[i];
				if (galleryItem == null) {
					if (DEBUG) {
						System.out.println("Virtual/creating item "); //$NON-NLS-1$
					}

					galleryItem = new GalleryItem(parentItem, SWT.NONE, i,
							false);
					parentItem.items[i] = galleryItem;
					setData(galleryItem, i);
				}
			}
		}

	}

	/**
	 * Sends SWT.SetData event. Used if SWT.VIRTUAL
	 * 
	 * @param galleryItem
	 * @param index
	 */
	protected void setData(GalleryItem galleryItem, int index) {
		Item item = galleryItem;
		Event e = new Event();
		e.item = item;
		e.type = SWT.SetData;
		e.index = index;
		this.notifyListeners(SWT.SetData, e);
	}

	/**
	 * Recalculate structural values using the group renderer<br>
	 * Gallery and item size will be updated.
	 * 
	 * @param keepLocation
	 *            if true, the current scrollbars position ratio is saved and
	 *            restored even if the gallery size has changed. (Visible items
	 *            stay visible)
	 * @deprecated Use {@link #updateStructuralValues(GalleryItem,boolean)}
	 *             instead
	 */
	protected void updateStructuralValues(boolean keepLocation) {
		updateStructuralValues(null, keepLocation);
	}

	/**
	 * Recalculate structural values using the group renderer<br>
	 * Gallery and item size will be updated.
	 * 
	 * @param changedGroup
	 *            the group that was modified since the last layout. If the
	 *            group renderer or more that one group have changed, use null
	 *            as parameter (full update)
	 * @param keepLocation
	 *            if true, the current scrollbars position ratio is saved and
	 *            restored even if the gallery size has changed. (Visible items
	 *            stay visible)
	 */
	protected void updateStructuralValues(GalleryItem changedGroup,
			boolean keepLocation) {

		if (DEBUG) {
			System.out.println("Client Area : " + this.getClientArea().x + " " //$NON-NLS-1$//$NON-NLS-2$
					+ this.getClientArea().y + " " + this.getClientArea().width //$NON-NLS-1$
					+ " " + this.getClientArea().height); //$NON-NLS-1$
		}

		Rectangle area = this.getClientArea();
		float pos = 0;

		if (vertical) {
			if (gHeight > 0 && keepLocation)
				pos = (float) (translate + 0.5 * area.height) / gHeight;

			gWidth = area.width;
			gHeight = calculateSize(changedGroup);

			if (keepLocation)
				translate = (int) (gHeight * pos - 0.5 * area.height);

		} else {
			if (gWidth > 0 && keepLocation)
				pos = (float) (translate + 0.5 * area.width) / gWidth;

			gWidth = calculateSize(changedGroup);
			gHeight = area.height;

			if (keepLocation)
				translate = (int) (gWidth * pos - 0.5 * area.width);
		}

		validateTranslation();

		if (DEBUG) {
			System.out.println("Content Size : " + gWidth + " " + gHeight); //$NON-NLS-1$//$NON-NLS-2$
		}

	}

	/**
	 * Calculate full height (or width) of the Gallery. The group renderer is
	 * used to calculate the size of each group.
	 * 
	 * @return
	 */
	private int calculateSize(GalleryItem onlyUpdateGroup) {

		if (groupRenderer == null)
			return 0;

		groupRenderer.preLayout(null);

		int currentHeight = 0;

		int mainItemCount = getItemCount();

		for (int i = 0; i < mainItemCount; i++) {
			GalleryItem item = null;
			if (virtualGroups) {
				item = this._getItem(i, false);
			} else {
				item = this._getItem(i);
			}

			if (onlyUpdateGroup != null && !onlyUpdateGroup.equals(item)) {
				// Item has not changed : no layout.
				if (vertical) {
					item.y = currentHeight;
					item.x = getClientArea().x;
					currentHeight += item.height;
				} else {
					item.y = getClientArea().y;
					item.x = currentHeight;
					currentHeight += item.width;
				}
			} else {
				this.groupRenderer.setExpanded(item.isExpanded());
				// TODO: Not used ATM
				// int groupItemCount = item.getItemCount();
				if (vertical) {
					item.y = currentHeight;
					item.x = getClientArea().x;
					item.width = getClientArea().width;
					item.height = -1;
					this.groupRenderer.layout(null, item);
					currentHeight += item.height;
				} else {
					item.y = getClientArea().y;
					item.x = currentHeight;
					item.width = -1;
					item.height = getClientArea().height;
					this.groupRenderer.layout(null, item);
					currentHeight += item.width;
				}
				// Unused ?
				// Point s = this.getSize(item.hCount, item.vCount, itemSizeX,
				// itemSizeY, userMargin, realMargin);

				// item.height = s.y;
			}

		}

		groupRenderer.postLayout(null);

		return currentHeight;
	}

	/**
	 * Move the scrollbar to reflect the current visible items position. <br/>
	 * The bar which is moved depends of the current gallery scrolling :
	 * vertical or horizontal.
	 * 
	 */
	protected void updateScrollBarsProperties() {

		if (vertical) {
			updateScrollBarProperties(getVerticalBar(), getClientArea().height,
					gHeight);
		} else {
			updateScrollBarProperties(getHorizontalBar(),
					getClientArea().width, gWidth);
		}

	}

	/**
	 * Move the scrollbar to reflect the current visible items position.
	 * 
	 * @param bar
	 *            - the scroll bar to move
	 * @param clientSize
	 *            - Client (visible) area size
	 * @param totalSize
	 *            - Total Size
	 */
	private void updateScrollBarProperties(ScrollBar bar, int clientSize,
			int totalSize) {
		if (bar == null)
			return;

		bar.setMinimum(0);
		bar.setPageIncrement(clientSize);
		bar.setMaximum(totalSize);
		bar.setThumb(clientSize);

		// Let the group renderer use a custom increment value.
		if (groupRenderer != null)
			bar.setIncrement(groupRenderer.getScrollBarIncrement());

		if (totalSize > clientSize) {
			if (DEBUG)
				System.out.println("Enabling scrollbar"); //$NON-NLS-1$

			bar.setEnabled(true);
			bar.setVisible(true);
			bar.setSelection(translate);

			// Ensure that translate has a valid value.
			validateTranslation();
		} else {
			if (DEBUG)
				System.out.println("Disabling scrollbar"); //$NON-NLS-1$

			bar.setEnabled(false);
			bar.setVisible(false);
			bar.setSelection(0);
			translate = 0;
		}

	}

	/**
	 * Check the current translation value. Must be &gt; 0 and &lt; gallery
	 * size.<br/>
	 * Invalid values are fixed.
	 */
	private void validateTranslation() {
		Rectangle area = this.getClientArea();
		// Ensure that translate has a valid value.
		int totalSize = 0;
		int clientSize = 0;

		// Fix negative values
		if (translate < 0)
			translate = 0;

		// Get size depending on vertical setting.
		if (vertical) {
			totalSize = gHeight;
			clientSize = area.height;
		} else {
			totalSize = gWidth;
			clientSize = area.width;
		}

		if (totalSize > clientSize) {
			// Fix translate too big.
			if (translate + clientSize > totalSize) {
				translate = totalSize - clientSize;
			}
		} else {
			translate = 0;
		}

	}

	protected void scrollVertical() {
		int areaHeight = getClientArea().height;

		if (gHeight > areaHeight) {
			// image is higher than client area
			ScrollBar bar = getVerticalBar();
			scroll(0, translate - bar.getSelection(), 0, 0,
					getClientArea().width, areaHeight, false);
			translate = bar.getSelection();
		} else {
			translate = 0;
		}
	}

	protected void scrollHorizontal() {

		int areaWidth = getClientArea().width;
		if (gWidth > areaWidth) {
			// image is higher than client area
			ScrollBar bar = getHorizontalBar();
			scroll(translate - bar.getSelection(), 0, 0, 0, areaWidth,
					getClientArea().height, false);
			translate = bar.getSelection();
		} else {
			translate = 0;
		}

	}

	protected void addItem(GalleryItem item, int position) {
		if (position != -1 && (position < 0 || position > getItemCount())) {
			throw new IllegalArgumentException("ERROR_INVALID_RANGE "); //$NON-NLS-1$
		}
		_addItem(item, position);
	}

	private void _addItem(GalleryItem item, int position) {
		// Insert item
		items = (GalleryItem[]) _arrayAddItem(items, item, position);

		// Update Gallery
		updateStructuralValues(null, false);
		updateScrollBarsProperties();
	}

	/**
	 * Get the item at index.<br/>
	 * If SWT.VIRTUAL is used and the item has not been used yet, the item is
	 * created and a SWT.SetData event is fired.
	 * 
	 * @param index
	 *            index of the item.
	 * @return the GalleryItem or null if index is out of bounds
	 */
	public GalleryItem getItem(int index) {
		checkWidget();
		return _getItem(index);
	}

	/**
	 * This method is used by items to implement getItem( index )
	 * 
	 * @param parent
	 * @param index
	 * @return
	 */
	protected GalleryItem _getItem(GalleryItem parent, int index) {

		if (index < parent.getItemCount()) {
			// Refresh item if it is not set yet
			updateItem(parent, index, true);

			if (parent.items == null) {
				return null;
			} else {
				return parent.items[index];
			}
		}

		return null;
	}

	/**
	 * Get the item at index.<br/>
	 * If SWT.VIRTUAL is used and the item has not been used yet, the item is
	 * created and a SWT.SetData is fired.<br/>
	 * 
	 * This is the internal implementation of this method : checkWidget() is not
	 * used.
	 * 
	 * @param index
	 * @return The item at 'index' (not null)
	 */
	protected GalleryItem _getItem(int index) {
		return _getItem(index, true);
	}

	/**
	 * Get the item at 'index'.<br/>
	 * If SWT.VIRTUAL is used, 'create' is true and the item has not been used
	 * yet, the item is created and a SWT.SetData is fired.<br/>
	 * 
	 * @param index
	 * @param create
	 * @return The item at 'index' or null if there was no item and 'create' was
	 *         false.
	 */
	protected GalleryItem _getItem(int index, boolean create) {

		if (index < getItemCount()) {
			updateItem(null, index, create);
			return items[index];
		}

		return null;
	}

	/**
	 * Forward the mouseDown event to the corresponding group according to the
	 * mouse position.
	 * 
	 * @param e
	 * @return
	 */
	protected boolean _mouseDown(MouseEvent e) {
		if (DEBUG)
			System.out.println("getitem " + e.x + " " + e.y); //$NON-NLS-1$//$NON-NLS-2$

		GalleryItem group = this._getGroup(new Point(e.x, e.y));
		if (group != null) {
			int pos = vertical ? (e.y + translate) : (e.x + translate);
			return groupRenderer.mouseDown(group, e, new Point(vertical ? e.x
					: pos, vertical ? pos : e.y));
		}

		return true;
	}

	/**
	 * Get item at pixel position
	 * 
	 * @param coords
	 * @return GalleryItem or null
	 */
	public GalleryItem getItem(Point coords) {
		checkWidget();

		if (DEBUG)
			System.out.println("getitem " + coords.x + " " + coords.y); //$NON-NLS-1$ //$NON-NLS-2$

		int pos = vertical ? (coords.y + translate) : (coords.x + translate);

		GalleryItem group = this._getGroup(coords);
		if (group != null)
			return groupRenderer.getItem(group, new Point(vertical ? coords.x
					: pos, vertical ? pos : coords.y));

		return null;
	}

	/**
	 * Get group at pixel position
	 * 
	 * @param coords
	 * @return GalleryItem or null
	 */
	private GalleryItem _getGroup(Point coords) {
		// If there is no item in the gallery, return asap
		if (items == null)
			return null;

		int pos = vertical ? (coords.y + translate) : (coords.x + translate);

		int index = 0;
		GalleryItem item = null;
		while (index < items.length) {
			item = getItem(index);

			if ((vertical ? item.y : item.x) > pos)
				break;

			if ((vertical ? (item.y + item.height) : (item.x + item.width)) >= pos)
				return item;

			index++;
		}

		return null;
	}

	/**
	 * <p>
	 * Get group at pixel position (relative to client area).
	 * </p>
	 * <p>
	 * This is an experimental API which is exposing an internal method, it may
	 * become deprecated at some point.
	 * </p>
	 * 
	 * @param coords
	 * @return
	 */
	public GalleryItem getGroup(Point coords) {
		checkWidget();
		return _getGroup(coords);
	}

	// TODO: Not used ATM
	// private void clear() {
	// checkWidget();
	// if (virtual) {
	// setItemCount(0);
	// } else {
	// items = null;
	// }
	//
	// updateStructuralValues(true);
	// updateScrollBarsProperties();
	// }

	/**
	 * Clear all Gallery items.<br/>
	 * 
	 * 
	 * If the Gallery is virtual, the item count is not reseted and all items
	 * will be created again at their first use.<br/>
	 * 
	 * @param all
	 *            If true, all children will be cleared. Only groups are cleared
	 *            otherwise.
	 */
	public void clearAll(boolean all) {
		checkWidget();

		if (items == null)
			return;

		if (virtual) {
			items = new GalleryItem[items.length];
		} else {
			for (int i = 0; i < items.length; i++) {
				if (items[i] != null) {
					if (all) {
						items[i].clearAll(true);
					} else {
						items[i].clear();
					}
				}
			}
		}

		// TODO: I'm clearing selection here
		// but we have to check that Table has the same behavior
		this._deselectAll(false);

		updateStructuralValues(null, false);
		updateScrollBarsProperties();
		redraw();

	}

	/**
	 * Clear all GalleryGroups
	 */
	public void clearAll() {
		clearAll(false);
	}

	/**
	 * Clear one item.<br/>
	 * 
	 * @param index
	 */
	public void clear(int index) {
		clear(index, false);
	}

	/**
	 * Clear one item and all its children if 'all' is true
	 * 
	 * @param index
	 * @param all
	 */
	public void clear(int index, boolean all) {
		checkWidget();

		// Item is already cleared, return immediately.
		if (items[index] == null)
			return;

		if (virtual) {
			// Clear item
			items[index] = null;

			// In virtual mode, clearing an item can change its content, so
			// force content update
			updateStructuralValues(null, false);
			updateScrollBarsProperties();
		} else {
			// Reset item
			items[index].clear();
			if (all) {
				items[index].clearAll(true);
			}
		}

		redraw();
	}

	/**
	 * Returns the index of a GalleryItem.
	 * 
	 * @param parentItem
	 * @param item
	 * @return
	 */
	public int indexOf(GalleryItem item) {
		checkWidget();
		if (item == null) {
			SWT.error(SWT.ERROR_NULL_ARGUMENT);
			// SWT.error throws an exception
		}

		if (item.getParentItem() == null) {
			return _indexOf(item);
		}

		return _indexOf(item.getParentItem(), item);
	}

	/**
	 * Returns the index of a GalleryItem when it is a root Item
	 * 
	 * @param parentItem
	 * @param item
	 * @return
	 */
	protected int _indexOf(GalleryItem item) {
		int itemCount = getItemCount();
		if (item == null)
			SWT.error(SWT.ERROR_NULL_ARGUMENT);
		if (1 <= lastIndexOf && lastIndexOf < itemCount - 1) {
			if (items[lastIndexOf] == item)
				return lastIndexOf;
			if (items[lastIndexOf + 1] == item)
				return ++lastIndexOf;
			if (items[lastIndexOf - 1] == item)
				return --lastIndexOf;
		}
		if (lastIndexOf < itemCount / 2) {
			for (int i = 0; i < itemCount; i++) {
				if (items[i] == item)
					return lastIndexOf = i;
			}
		} else {
			for (int i = itemCount - 1; i >= 0; --i) {
				if (items[i] == item)
					return lastIndexOf = i;
			}
		}
		return -1;
	}

	/**
	 * Returns the index of a GalleryItem when it is not a root Item
	 * 
	 * @param parentItem
	 * @param item
	 * @return
	 */
	protected int _indexOf(GalleryItem parentItem, GalleryItem item) {
		int itemCount = parentItem.getItemCount();
		if (item == null)
			SWT.error(SWT.ERROR_NULL_ARGUMENT);
		if (1 <= parentItem.lastIndexOf
				&& parentItem.lastIndexOf < itemCount - 1) {
			if (parentItem.items[parentItem.lastIndexOf] == item)
				return parentItem.lastIndexOf;
			if (parentItem.items[parentItem.lastIndexOf + 1] == item)
				return ++parentItem.lastIndexOf;
			if (parentItem.items[parentItem.lastIndexOf - 1] == item)
				return --parentItem.lastIndexOf;
		}
		if (parentItem.lastIndexOf < itemCount / 2) {
			for (int i = 0; i < itemCount; i++) {
				if (parentItem.items[i] == item)
					return parentItem.lastIndexOf = i;
			}
		} else {
			for (int i = itemCount - 1; i >= 0; --i) {
				if (parentItem.items[i] == item)
					return parentItem.lastIndexOf = i;
			}
		}
		return -1;
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
	 * @see Gallery#setUseControlColors(boolean)
	 * @return true if Gallery uses parent colors.
	 */
	public boolean isUseControlColors() {
		return useControlColors;
	}

	/**
	 * Set useControlColors to true in order to get colors from parent Control
	 * (SWT default). This may generate more objects on painting and slightly
	 * slow down the application. See Bug 279822 :
	 * https://bugs.eclipse.org/bugs/show_bug.cgi?id=279822
	 * 
	 * If enabled, you'll get new Color objects each time you call getXXXColor()
	 * on Gallery or GalleryItem.
	 * 
	 * Default is false : colors are stored locally in Gallery, and you'll get
	 * the same object each time you call getXXXColor() on Gallery
	 * orGalleryItem. The Gallery may not catch color changes on parent control.
	 * 
	 * @param useControlColors
	 */
	public void setUseControlColors(boolean useControlColors) {
		this.useControlColors = useControlColors;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.swt.widgets.Control#getBackground()
	 */
	public Color getBackground() {
		return getBackground(false);
	}

	/**
	 * Returns the receiver's background color.
	 * 
	 * @param galleryOnly
	 *            If TRUE, does not try to parent widget or Display defaults to
	 *            guess the real background color. Note : FALSE is the default
	 *            behavior.
	 * 
	 * @return The background color or null if galleryOnly was used and the
	 *         gallery has not foreground color set.
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
	public Color getBackground(boolean galleryOnly) {
		if (galleryOnly) {
			return this.backgroundColor;
		}

		if (useControlColors) {
			return super.getBackground();
		}

		return backgroundColor != null ? backgroundColor : getDisplay()
				.getSystemColor(SWT.COLOR_LIST_BACKGROUND);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.swt.widgets.Control#getForeground()
	 */
	public Color getForeground() {
		return getForeground(false);
	}

	/**
	 * Returns the receiver's foreground color.
	 * 
	 * @param galleryOnly
	 *            If TRUE, does not try to parent widget or Display defaults to
	 *            guess the real foreground color. Note : FALSE is the default
	 *            behavior.
	 * 
	 * @return The foreground color or null if galleryOnly was used and the
	 *         gallery has not foreground color set.
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
	public Color getForeground(boolean galleryOnly) {

		if (galleryOnly) {
			return this.foregroundColor;
		}

		if (useControlColors) {
			return super.getForeground();
		}

		return foregroundColor != null ? foregroundColor : getDisplay()
				.getSystemColor(SWT.COLOR_LIST_FOREGROUND);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.eclipse.swt.widgets.Control#setBackground(org.eclipse.swt.graphics
	 * .Color)
	 */
	public void setBackground(Color color) {
		// Cache color locally
		if (!useControlColors) {
			this.backgroundColor = color;
		}

		// Always call Control#setBackground(); Additionally, this will trigger
		// a redraw.
		super.setBackground(color);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.eclipse.swt.widgets.Control#setForeground(org.eclipse.swt.graphics
	 * .Color)
	 */
	public void setForeground(Color color) {
		// Cache color locally
		if (!useControlColors) {
			this.foregroundColor = color;
		}

		// Always call Control#setForeground(); Additionally, this will trigger
		// a redraw.
		super.setForeground(color);
	}

	/**
	 * Checks if the Gallery was created with SWT.V_SCROLL (ie has a vertical
	 * scroll bar).
	 * 
	 * @return true if the gallery has the SWT.V_SCROLL style.
	 */
	public boolean isVertical() {
		checkWidget();
		return vertical;
	}

	/**
	 * @deprecated
	 * @param vertical
	 */
	public void setVertical(boolean vertical) {
		checkWidget();
		this.vertical = vertical;
		this.updateStructuralValues(null, true);
		redraw();
	}

	public AbstractGalleryGroupRenderer getGroupRenderer() {
		return groupRenderer;
	}

	public void setGroupRenderer(AbstractGalleryGroupRenderer groupRenderer) {
		this.groupRenderer = groupRenderer;

		if (this.groupRenderer != null) {
			this.groupRenderer.setGallery(this);
		}

		this.updateStructuralValues(null, true);
		this.updateScrollBarsProperties();
		this.redraw();
	}

	public GalleryItem[] getSelection() {
		if (selection == null) {
			return new GalleryItem[0];
		}

		return selection;
	}

	public int getSelectionCount() {
		if (selection == null)
			return 0;

		return selection.length;
	}

	/**
	 * Selects all of the items in the receiver.
	 */
	public void selectAll() {
		checkWidget();
		_selectAll();
		redraw();
	}

	protected void _selectAll() {
		select(0, this.getItemCount() - 1);
	}

	public void setSelection(GalleryItem[] items) {
		checkWidget();
		_deselectAll(false);
		for (int i = 0; i < items.length; i++) {
			this.setSelected(items[i], true, false);

			// Ensure item is visible
			_showItem(items[i]);

			// Simulate mouse click to enable keyboard navigation
			lastSingleClick = items[i];
		}
		redraw();
	}

	public void removeAll() {
		checkWidget();

		if (items != null) {
			// Clear items

			GalleryItem[] tmpArray = new GalleryItem[items.length];
			System.arraycopy(items, 0, tmpArray, 0, items.length);

			for (int i = 0; i < tmpArray.length; i++) {

				// Dispose items if not virtual
				// if (!virtual) {
				if (tmpArray[i] != null) {
					tmpArray[i]._dispose();
				}
				// }
			}
		}
	}

	public void remove(int index) {
		checkWidget();
		_remove(index);

		updateStructuralValues(null, false);
		updateScrollBarsProperties();
		redraw();
	}

	public void remove(GalleryItem item) {
		if (item.getParentItem() == null) {
			remove(indexOf(item));
		} else {
			item.getParentItem().remove(item);
		}
	}

	protected void _remove(int index) {
		// if (!virtual) {
		if (isSelected(items[index])) {
			setSelected(items[index], false, false);
		}

		this.items = (GalleryItem[]) this._arrayRemoveItem(this.items, index);

		// if( virtual)
		// itemCount--;
		// }
	}

	protected void _remove(GalleryItem parent, int index) {
		if (isSelected(parent.items[index])) {
			setSelected(parent.items[index], false, false);
		}

		parent.items = (GalleryItem[]) this._arrayRemoveItem(parent.items,
				index);
	}

	protected Object[] _arrayRemoveItem(Object[] array, int index) {

		if (array == null)
			return null;

		if (array.length == 1 && index == 0)
			return null;

		Object[] newArray = (Object[]) Array.newInstance(array.getClass()
				.getComponentType(), array.length - 1);

		if (index > 0)
			System.arraycopy(array, 0, newArray, 0, index);

		if (index + 1 < array.length)
			System.arraycopy(array, index + 1, newArray, index, newArray.length
					- index);

		return newArray;
	}

	/**
	 * Adds an item to an array.
	 * 
	 * @param array
	 * @param object
	 * @param index
	 *            : if index == -1, item is added at the end of the array.
	 * @return
	 */
	protected Object[] _arrayAddItem(Object[] array, Object object, int index) {

		// Get current array length
		int length = 0;
		if (array != null)
			length = array.length;

		// Create new array
		Object[] newArray = (Object[]) Array.newInstance(object.getClass(),
				length + 1);

		if (array != null)
			System.arraycopy(array, 0, newArray, 0, length);

		if (index != -1) {
			// Move all items
			for (int i = newArray.length - 2; i >= index; i--) {
				if (i >= 0) {
					newArray[i + 1] = newArray[i];
				}
			}

			// Insert item at index
			newArray[index] = object;

		} else {
			// Insert item at the end
			newArray[newArray.length - 1] = object;
		}

		return newArray;
	}

	protected int _arrayIndexOf(int[] array, int value) {
		if (array == null)
			return -1;

		for (int i = array.length - 1; i >= 0; --i) {
			if (array[i] == value) {
				return i;

			}
		}
		return -1;
	}

	protected int _arrayIndexOf(Object[] array, Object value) {
		if (array == null)
			return -1;

		for (int i = array.length - 1; i >= 0; --i) {
			if (array[i] == value) {
				return i;

			}
		}
		return -1;
	}

	protected int[] _arrayRemoveItem(int[] array, int index) {

		if (array == null)
			return null;

		if (array.length == 1 && index == 0)
			return null;

		int[] newArray = new int[array.length - 1];

		if (index > 0)
			System.arraycopy(array, 0, newArray, 0, index);

		if (index + 1 < array.length)
			System.arraycopy(array, index + 1, newArray, index, newArray.length
					- index);

		return newArray;
	}

	public void _setGalleryItems(GalleryItem[] items) {
		this.items = items;
	}

	/**
	 * @see #setVirtualGroups(boolean)
	 * 
	 * @return
	 */
	public boolean isVirtualGroups() {
		return virtualGroups;
	}

	/**
	 * Enable virtual groups
	 * 
	 * <p>
	 * When a gallery has the SWT.VIRTUAL flag, only items are initialized on
	 * display. All groups need to be initialized from the beginning to
	 * calculate the total size of the content.
	 * </p>
	 * 
	 * <p>
	 * Virtual groups enable creating groups AND items lazily at the cost of a
	 * poor approximation of the total size of the content.
	 * </p>
	 * 
	 * <p>
	 * While a group isn't initialized, the item count defined as default item
	 * count is used.
	 * </p>
	 * 
	 * <p>
	 * When a group comes into view, it is initialized using the setData event,
	 * and the size of the gallery content is updated to match the real value.
	 * </p>
	 * 
	 * <p>
	 * From the developer point of view, virtual groups uses exactly the same
	 * code as the standard virtual mode of SWT.
	 * </p>
	 * 
	 * <p>
	 * This mode can create visual glitches with code that automatically scrolls
	 * the widget such as SAT Smooth Scrolling. In that case, you can enable the
	 * compatibility mode which is little less lazy that the default virtual
	 * groups, but still better than the standard virtual mode
	 * </p>
	 * 
	 * @see #setVirtualGroupDefaultItemCount(int)
	 * @see #setVirtualGroupsCompatibilityMode(boolean)
	 * 
	 * @param virtualGroups
	 */
	public void setVirtualGroups(boolean virtualGroups) {
		this.virtualGroups = virtualGroups;
	}

	/**
	 * @see #setVirtualGroupDefaultItemCount(int)
	 * @return
	 */
	public int getVirtualGroupDefaultItemCount() {
		return virtualGroupDefaultItemCount;
	}

	/**
	 * @see #setVirtualGroupsCompatibilityMode(boolean)
	 * @return
	 */
	public boolean isVirtualGroupsCompatibilityMode() {
		return virtualGroupsCompatibilityMode;
	}

	/**
	 * Enable the compatibility workaround for problems with the ultra virtual
	 * mode.
	 * 
	 * @see #setVirtualGroups(boolean)
	 * @param compatibilityMode
	 */
	public void setVirtualGroupsCompatibilityMode(boolean compatibilityMode) {
		this.virtualGroupsCompatibilityMode = compatibilityMode;
	}

	/**
	 * Set the item count used when a group is not yet initialized (with virtual
	 * groups). Since the virtual groups make the size of the gallery change
	 * while scrolling, a fine tuned item count can improve the accuracy of the
	 * slider.
	 * 
	 * @see #setVirtualGroups(boolean)
	 * 
	 * @param defaultItemCount
	 */
	public void setVirtualGroupDefaultItemCount(int defaultItemCount) {
		this.virtualGroupDefaultItemCount = defaultItemCount;
	}
}
