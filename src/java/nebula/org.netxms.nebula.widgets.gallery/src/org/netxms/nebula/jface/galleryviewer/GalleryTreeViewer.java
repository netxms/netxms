/*******************************************************************************
 * Copyright (c) 2007-2008 Peter Centgraf.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors :
 *    Peter Centgraf - initial implementation
 *******************************************************************************/
package org.netxms.nebula.jface.galleryviewer;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;

import org.eclipse.jface.viewers.AbstractTreeViewer;
import org.eclipse.jface.viewers.ColumnViewerEditor;
import org.eclipse.jface.viewers.ColumnViewerEditorActivationEvent;
import org.eclipse.jface.viewers.IContentProvider;
import org.eclipse.jface.viewers.IStructuredContentProvider;
import org.eclipse.jface.viewers.ITreeContentProvider;
import org.eclipse.jface.viewers.ITreePathContentProvider;
import org.eclipse.jface.viewers.TreePath;
import org.eclipse.jface.viewers.TreeSelection;
import org.eclipse.jface.viewers.ViewerCell;
import org.eclipse.jface.viewers.ViewerRow;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.TreeListener;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Item;
import org.eclipse.swt.widgets.Widget;
import org.netxms.nebula.widgets.gallery.DefaultGalleryGroupRenderer;
import org.netxms.nebula.widgets.gallery.DefaultGalleryItemRenderer;
import org.netxms.nebula.widgets.gallery.Gallery;
import org.netxms.nebula.widgets.gallery.GalleryItem;

/**
 * A concrete viewer based on a Nebula <code>Gallery</code> control.
 * <p>
 * This class is not intended to be subclassed outside the viewer framework. It
 * is designed to be instantiated with a pre-existing Nebula Gallery control and
 * configured with a domain-specific content provider, label provider, element
 * filter (optional), and element sorter (optional).
 * </p>
 * 
 * <p>
 * SWT.VIRTUAL is currently unsupported
 * </p>
 * 
 * <p>
 * THIS VIEWER SHOULD BE CONSIDERED AS ALPHA
 * </p>
 * 
 * @author Peter Centgraf
 * @contributor Nicolas Richeton
 * @since Dec 5, 2007
 */
public class GalleryTreeViewer extends AbstractTreeViewer {

	protected Gallery gallery;

	/**
	 * true if we are inside a preservingSelection() call
	 */
	protected boolean preservingSelection;

	/**
	 * The row object reused
	 */
	private GalleryViewerRow cachedRow;

	/**
	 * Creates a gallery viewer on a newly-created gallery control under the
	 * given parent. The gallery control is created using the SWT style bits
	 * <code>MULTI, V_SCROLL,</code> and <code>BORDER</code>. The viewer has no
	 * input, no content provider, a default label provider, no sorter, and no
	 * filters.
	 * 
	 * @param parent
	 *            the parent control
	 */
	public GalleryTreeViewer(Composite parent) {
		this(parent, SWT.MULTI | SWT.V_SCROLL | SWT.BORDER);
	}

	/**
	 * Creates a gallery viewer on a newly-created gallery control under the
	 * given parent. The gallery control is created using the given SWT style
	 * bits. The viewer has no input, no content provider, a default label
	 * provider, no sorter, and no filters.
	 * 
	 * @param parent
	 *            the parent control
	 * @param style
	 *            the SWT style bits used to create the gallery.
	 */
	public GalleryTreeViewer(Composite parent, int style) {
		gallery = new Gallery(parent, style);
		gallery.setGroupRenderer(new DefaultGalleryGroupRenderer());
		gallery.setItemRenderer(new DefaultGalleryItemRenderer());
		super.setAutoExpandLevel(ALL_LEVELS);

		hookControl(gallery);
	}

	/**
	 * Creates a gallery viewer on the given gallery control. The viewer has no
	 * input, no content provider, a default label provider, no sorter, and no
	 * filters.
	 * 
	 * @param gallery
	 *            the gallery control
	 */
	public GalleryTreeViewer(Gallery gallery) {
		super();
		this.gallery = gallery;
		super.setAutoExpandLevel(ALL_LEVELS);

		hookControl(gallery);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.eclipse.jface.viewers.AbstractTreeViewer#addTreeListener(org.eclipse
	 * .swt.widgets.Control, org.eclipse.swt.events.TreeListener)
	 */
	protected void addTreeListener(Control control, TreeListener listener) {
		((Gallery) control).addTreeListener(listener);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.eclipse.jface.viewers.AbstractTreeViewer#getChild(org.eclipse.swt
	 * .widgets.Widget, int)
	 */
	protected Item getChild(Widget widget, int index) {
		if (widget instanceof GalleryItem) {
			return ((GalleryItem) widget).getItem(index);
		}
		if (widget instanceof Gallery) {
			return ((Gallery) widget).getItem(index);
		}
		return null;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.eclipse.jface.viewers.AbstractTreeViewer#getChildren(org.eclipse.
	 * swt.widgets.Widget)
	 */
	protected Item[] getChildren(Widget widget) {
		if (widget instanceof GalleryItem) {
			return ((GalleryItem) widget).getItems();
		}
		if (widget instanceof Gallery) {
			return ((Gallery) widget).getItems();
		}
		return null;
	}

	protected Widget getColumnViewerOwner(int columnIndex) {
		if (columnIndex == 0) {
			return getGallery();
		}
		return null;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.eclipse.jface.viewers.AbstractTreeViewer#getExpanded(org.eclipse.
	 * swt.widgets.Item)
	 */
	protected boolean getExpanded(Item item) {
		return ((GalleryItem) item).isExpanded();
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.eclipse.jface.viewers.AbstractTreeViewer#getItemCount(org.eclipse
	 * .swt.widgets.Control)
	 */
	protected int getItemCount(Control control) {
		return ((Gallery) control).getItemCount();
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.eclipse.jface.viewers.AbstractTreeViewer#getItemCount(org.eclipse
	 * .swt.widgets.Item)
	 */
	protected int getItemCount(Item item) {
		return ((GalleryItem) item).getItemCount();
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.eclipse.jface.viewers.AbstractTreeViewer#getItems(org.eclipse.swt
	 * .widgets.Item)
	 */
	protected Item[] getItems(Item item) {
		return ((GalleryItem) item).getItems();
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.eclipse.jface.viewers.AbstractTreeViewer#getParentItem(org.eclipse
	 * .swt.widgets.Item)
	 */
	protected Item getParentItem(Item item) {
		return ((GalleryItem) item).getParentItem();
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.eclipse.jface.viewers.AbstractTreeViewer#getSelection(org.eclipse
	 * .swt.widgets.Control)
	 */
	protected Item[] getSelection(Control control) {
		Item[] selection = ((Gallery) control).getSelection();
		if (selection == null) {
			return new GalleryItem[0];
		}

		List<Object> notDisposed = new ArrayList<Object>(selection.length);
		for (int i = 0; i < selection.length; i++) {
			if (!selection[i].isDisposed()) {
				notDisposed.add(selection[i]);
			} else {
				System.out.println("GalleryItem was disposed (ignoring)");
			}
		}
		selection = (GalleryItem[]) notDisposed
				.toArray(new GalleryItem[notDisposed.size()]);

		return selection;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.eclipse.jface.viewers.AbstractTreeViewer#newItem(org.eclipse.swt.
	 * widgets.Widget, int, int)
	 */
	protected Item newItem(Widget parent, int style, int index) {

		GalleryItem item;

		if (parent instanceof GalleryItem) {
			item = (GalleryItem) createNewRowPart(getViewerRowFromItem(parent),
					style, index).getItem();
		} else {
			item = (GalleryItem) createNewRowPart(null, style, index).getItem();
		}

		return item;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.eclipse.jface.viewers.AbstractTreeViewer#removeAll(org.eclipse.swt
	 * .widgets.Control)
	 */
	protected void removeAll(Control control) {
		((Gallery) control).removeAll();
	}

	public void setAutoExpandLevel(int level) {
		throw new UnsupportedOperationException(
				"Gallery must be fully expanded.");
	}

	/**
	 * <p>
	 * Gallery expects contents to have exactly 2 levels of hierarchy, with
	 * groups as the root elements and image thumbnails as direct children of
	 * the groups. This method accepts ITreeContentProvider and
	 * ITreePathContentProvider as-is, and relies on the providers to return
	 * contents with the correct structure.
	 * </p>
	 * <p>
	 * This method also accepts IStructuredContentProvider and wraps it in a
	 * FlatTreeContentProvider with an empty string as the root node. If you
	 * need a different root node, construct your own FlatTreeContentProvider
	 * and pass it here. If you want the Gallery to suppress the collapsable
	 * group header, call
	 * </p>
	 * <code>getGallery().setGroupRenderer(new NoGroupRenderer());</code>
	 */
	public void setContentProvider(IContentProvider provider) {
		if (provider instanceof IStructuredContentProvider
				&& !(provider instanceof ITreeContentProvider || provider instanceof ITreePathContentProvider)) {
			// Wrap a table-style contents with a single root node.
			super.setContentProvider(new FlatTreeContentProvider(
					(IStructuredContentProvider) provider));
		} else {
			super.setContentProvider(provider);
		}
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.eclipse.jface.viewers.AbstractTreeViewer#setExpanded(org.eclipse.
	 * swt.widgets.Item, boolean)
	 */
	protected void setExpanded(Item item, boolean expand) {
		((GalleryItem) item).setExpanded(expand);
		// if (contentProviderIsLazy) {
		// // force repaints to happen
		// getControl().update();
		// }
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.eclipse.jface.viewers.AbstractTreeViewer#setSelection(java.util.List)
	 */
	@SuppressWarnings({ "rawtypes", "unchecked" })
	protected void setSelection(List items) {
		Item[] current = getSelection(getGallery());

		// Don't bother resetting the same selection
		if (isSameSelection(items, current)) {
			return;
		}

		GalleryItem[] newItems = new GalleryItem[items.size()];
		items.toArray(newItems);
		getGallery().setSelection(newItems);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.eclipse.jface.viewers.AbstractTreeViewer#showItem(org.eclipse.swt
	 * .widgets.Item)
	 */
	protected void showItem(Item item) {
		gallery.showItem((GalleryItem) item);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.viewers.Viewer#getControl()
	 */
	public Control getControl() {
		return gallery;
	}

	/**
	 * Returns this gallery viewer's gallery.
	 * 
	 * @return the gallery control
	 */
	public Gallery getGallery() {
		return gallery;
	}

	protected Item getItemAt(Point point) {
		return gallery.getItem(point);
	}

	protected ColumnViewerEditor createViewerEditor() {
		// TODO: implement editing support
		return null;
	}

	/**
	 * For a GalleryViewer with a gallery with the VIRTUAL style bit set, set
	 * the number of children of the given element or tree path. To set the
	 * number of children of the invisible root of the gallery, you can pass the
	 * input object or an empty tree path.
	 * 
	 * @param elementOrTreePath
	 *            the element, or tree path
	 * @param count
	 * 
	 * @since 3.2
	 */
	public void setChildCount(final Object elementOrTreePath, final int count) {
		// if (isBusy())
		// return;
		preservingSelection(new Runnable() {
			public void run() {
				if (internalIsInputOrEmptyPath(elementOrTreePath)) {
					getGallery().setItemCount(count);
					return;
				}
				Widget[] items = internalFindItems(elementOrTreePath);
				for (int i = 0; i < items.length; i++) {
					GalleryItem galleryItem = (GalleryItem) items[i];
					galleryItem.setItemCount(count);
				}
			}
		});
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.eclipse.jface.viewers.ColumnViewer#getRowPartFromItem(org.eclipse
	 * .swt.widgets.Widget)
	 */
	protected ViewerRow getViewerRowFromItem(Widget item) {
		if (cachedRow == null) {
			cachedRow = new GalleryViewerRow((GalleryItem) item);
		} else {
			cachedRow.setItem((GalleryItem) item);
		}

		return cachedRow;
	}

	/**
	 * Create a new ViewerRow at rowIndex
	 * 
	 * @param parent
	 * @param style
	 * @param rowIndex
	 * @return ViewerRow
	 */
	private ViewerRow createNewRowPart(ViewerRow parent, int style, int rowIndex) {
		if (parent == null) {
			if (rowIndex >= 0) {
				return getViewerRowFromItem(new GalleryItem(gallery, style,
						rowIndex));
			}
			return getViewerRowFromItem(new GalleryItem(gallery, style));
		}

		if (rowIndex >= 0) {
			return getViewerRowFromItem(new GalleryItem((GalleryItem) parent
					.getItem(), SWT.NONE, rowIndex));
		}

		return getViewerRowFromItem(new GalleryItem((GalleryItem) parent
				.getItem(), SWT.NONE));
	}

	/**
	 * Removes the element at the specified index of the parent. The selection
	 * is updated if required.
	 * 
	 * @param parentOrTreePath
	 *            the parent element, the input element, or a tree path to the
	 *            parent element
	 * @param index
	 *            child index
	 * @since 3.3
	 */
	public void remove(final Object parentOrTreePath, final int index) {
		// if (isBusy())
		// return;
		final List<Object> oldSelection = new LinkedList<Object>(Arrays
				.asList(((TreeSelection) getSelection()).getPaths()));
		preservingSelection(new Runnable() {
			public void run() {
				TreePath removedPath = null;
				if (internalIsInputOrEmptyPath(parentOrTreePath)) {
					Gallery gallery = (Gallery) getControl();
					if (index < gallery.getItemCount()) {
						GalleryItem item = gallery.getItem(index);
						if (item.getData() != null) {
							removedPath = getTreePathFromItem(item);
							disassociate(item);
						}
						item.dispose();
					}
				} else {
					Widget[] parentItems = internalFindItems(parentOrTreePath);
					for (int i = 0; i < parentItems.length; i++) {
						GalleryItem parentItem = (GalleryItem) parentItems[i];
						if (index < parentItem.getItemCount()) {
							GalleryItem item = parentItem.getItem(index);
							if (item.getData() != null) {
								removedPath = getTreePathFromItem(item);
								disassociate(item);
							}
							item.dispose();
						}
					}
				}
				if (removedPath != null) {
					boolean removed = false;
					for (Iterator<?> it = oldSelection.iterator(); it.hasNext();) {
						TreePath path = (TreePath) it.next();
						if (path.startsWith(removedPath, getComparer())) {
							it.remove();
							removed = true;
						}
					}
					if (removed) {
						setSelection(new TreeSelection(
								(TreePath[]) oldSelection
										.toArray(new TreePath[oldSelection
												.size()]), getComparer()),
								false);
					}

				}
			}
		});
	}

	public void editElement(Object element, int column) {
		if (element instanceof TreePath) {
			setSelection(new TreeSelection((TreePath) element));
			GalleryItem[] items = gallery.getSelection();

			if (items.length == 1) {
				ViewerRow row = getViewerRowFromItem(items[0]);

				if (row != null) {
					ViewerCell cell = row.getCell(column);
					if (cell != null) {
						getControl().setRedraw(false);
						triggerEditorActivationEvent(new ColumnViewerEditorActivationEvent(
								cell));
						getControl().setRedraw(true);
					}
				}
			}
		} else {
			super.editElement(element, column);
		}
	}

}
