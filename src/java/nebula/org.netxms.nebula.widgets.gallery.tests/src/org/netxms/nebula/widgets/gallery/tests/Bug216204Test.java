package org.netxms.nebula.widgets.gallery.tests;

import junit.framework.TestCase;

import org.netxms.nebula.widgets.gallery.DefaultGalleryGroupRenderer;
import org.netxms.nebula.widgets.gallery.DefaultGalleryItemRenderer;
import org.netxms.nebula.widgets.gallery.Gallery;
import org.netxms.nebula.widgets.gallery.GalleryItem;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Shell;

public class Bug216204Test extends TestCase {

	private Shell s = null;
	private Display d = null;
	private boolean createdDisplay = false;

	protected void setUp() throws Exception {
		d = Display.getCurrent();
		if( d == null ){
			d = new Display();
			createdDisplay = true;
		}
		s = new Shell(d, SWT.NONE);
		super.setUp();
	}

	protected void tearDown() throws Exception {
		if( createdDisplay){
		d.dispose();
		}
		super.tearDown();
	}

	public void testBug212182OnGallery() {
		Gallery g = new Gallery(s, SWT.V_SCROLL);

		// Set Renderers
		DefaultGalleryGroupRenderer gr = new DefaultGalleryGroupRenderer();
		g.setGroupRenderer(gr);

		DefaultGalleryItemRenderer ir = new DefaultGalleryItemRenderer();
		g.setItemRenderer(ir);

		// Create an item
		GalleryItem item1 = new GalleryItem(g, SWT.NONE);

		g.setSelection(new GalleryItem[] { item1 });

		GalleryItem[] selection = g.getSelection();

		assertEquals(1, selection.length);
		assertEquals(item1, selection[0]);

		// Dispose item
		item1.dispose();
		selection = g.getSelection();

		assertEquals(0, selection.length);

		// Create a lot of items
		GalleryItem[] items = new GalleryItem[10];
		GalleryItem[] children = new GalleryItem[10];

		for (int i = 0; i < 10; i++) {
			items[i] = new GalleryItem(g, SWT.None);
		}

		for (int i = 0; i < 10; i++) {
			children[i] = new GalleryItem(items[0], SWT.None);
		}

		assertEquals(10, g.getItemCount());
		assertEquals(10, items[0].getItemCount());

		g.setSelection(new GalleryItem[] { children[5], items[5] });

		selection = g.getSelection();
		// This gallery can only have a single selection
		assertEquals(1, selection.length);
		assertEquals(items[5], selection[0]);

		// Clean
		g.dispose();
	}

	public void testBug212182OnGalleryMulti() {
		Gallery g = new Gallery(s, SWT.V_SCROLL | SWT.MULTI);

		// Set Renderers
		DefaultGalleryGroupRenderer gr = new DefaultGalleryGroupRenderer();
		g.setGroupRenderer(gr);

		DefaultGalleryItemRenderer ir = new DefaultGalleryItemRenderer();
		g.setItemRenderer(ir);

		// Create an item
		GalleryItem item1 = new GalleryItem(g, SWT.NONE);

		g.setSelection(new GalleryItem[] { item1 });

		GalleryItem[] selection = g.getSelection();

		assertEquals(1, selection.length);
		assertEquals(item1, selection[0]);

		// Dispose item
		item1.dispose();
		selection = g.getSelection();

		assertEquals(0, selection.length);

		// Create a lot of items
		GalleryItem[] items = new GalleryItem[10];
		GalleryItem[] children = new GalleryItem[10];

		for (int i = 0; i < 10; i++) {
			items[i] = new GalleryItem(g, SWT.None);
			items[i].setText("G" + i);
		}

		for (int i = 0; i < 10; i++) {
			children[i] = new GalleryItem(items[0], SWT.None);
			children[i].setText("G0-" + i);
		}

		assertEquals(10, g.getItemCount());
		assertEquals(10, items[0].getItemCount());

		g.setSelection(new GalleryItem[] { children[5], items[5] });

		selection = g.getSelection();

		assertEquals(2, selection.length);
		assertEquals(children[5], selection[0]);
		assertEquals(items[5], selection[1]);

		items[0].dispose();

		selection = g.getSelection();

		// Since we disposed the parent of a selected item, it should have been
		// removed from the selection
		assertEquals(1, selection.length);
		assertEquals(items[5], selection[0]);

		// Clean
		g.dispose();
	}

}
