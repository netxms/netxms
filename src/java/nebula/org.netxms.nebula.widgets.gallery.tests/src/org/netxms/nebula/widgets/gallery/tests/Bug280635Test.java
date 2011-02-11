package org.netxms.nebula.widgets.gallery.tests;

import junit.framework.TestCase;

import org.netxms.nebula.widgets.gallery.DefaultGalleryGroupRenderer;
import org.netxms.nebula.widgets.gallery.DefaultGalleryItemRenderer;
import org.netxms.nebula.widgets.gallery.Gallery;
import org.netxms.nebula.widgets.gallery.GalleryItem;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Listener;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Tree;
import org.eclipse.swt.widgets.TreeItem;

public class Bug280635Test extends TestCase {
	Display d = null;
	Shell s = null;
	private boolean createdDisplay = false;

	protected void setUp() throws Exception {
		d = Display.getCurrent();
		if (d == null) {
			d = new Display();
			createdDisplay = true;
		}
		s = new Shell(d, SWT.NONE);
		super.setUp();
	}

	protected void tearDown() throws Exception {
		if (createdDisplay) {
			d.dispose();
		}
		super.tearDown();
	}

	public void testBug280635() {
		final Gallery g = createGallery(SWT.V_SCROLL | SWT.VIRTUAL);

		g.addListener(SWT.SetData, new Listener() {

			public void handleEvent(Event event) {
				GalleryItem item = (GalleryItem) event.item;
				int index;
				if (item.getParentItem() != null) {
					index = item.getParentItem().indexOf(item);
					item.setItemCount(0);
				} else {
					index = g.indexOf(item);
					item.setItemCount(100);
				}

				System.out.println("setData index " + index); //$NON-NLS-1$
				// Your image here
				// item.setImage(eclipseImage);
				item.setText("Item " + index); //$NON-NLS-1$
			}

		});

		g.setItemCount(1);

		g.getItem(0).dispose();

		assertEquals(g.getItemCount(), 0);

		g.dispose();

	}

	public void testBug280635ForTree() {
		final Tree g = createTree(SWT.V_SCROLL | SWT.VIRTUAL);

		g.addListener(SWT.SetData, new Listener() {

			public void handleEvent(Event event) {
				TreeItem item = (TreeItem) event.item;
				int index;
				if (item.getParentItem() != null) {
					index = item.getParentItem().indexOf(item);
					item.setItemCount(0);
				} else {
					index = g.indexOf(item);
					item.setItemCount(100);
				}

				System.out.println("setData index " + index); //$NON-NLS-1$
				// Your image here
				// item.setImage(eclipseImage);
				item.setText("Item " + index); //$NON-NLS-1$
			}

		});

		g.setItemCount(1);

		g.getItem(0).dispose();

		assertEquals(g.getItemCount(), 0);
		g.dispose();

	}

	private Tree createTree(int flags) {
		Tree tree = new Tree(s, flags);

		return tree;
	}

	private Gallery createGallery(int flags) {
		Gallery g = new Gallery(s, flags);

		// Renderers
		DefaultGalleryGroupRenderer gr = new DefaultGalleryGroupRenderer();
		gr.setMinMargin(2);
		gr.setItemHeight(56);
		gr.setItemWidth(72);
		gr.setAutoMargin(true);
		g.setGroupRenderer(gr);

		DefaultGalleryItemRenderer ir = new DefaultGalleryItemRenderer();
		g.setItemRenderer(ir);

		return g;
	}
}
