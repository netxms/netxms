package org.netxms.nebula.widgets.gallery.tests;

import junit.framework.TestCase;

import org.netxms.nebula.widgets.gallery.DefaultGalleryGroupRenderer;
import org.netxms.nebula.widgets.gallery.DefaultGalleryItemRenderer;
import org.netxms.nebula.widgets.gallery.Gallery;
import org.netxms.nebula.widgets.gallery.GalleryItem;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Shell;

public class Bug276435Test extends TestCase {

	private Shell s = null;
	private Display d = null;
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

	public void testBug276435() {
		Gallery g = new Gallery(s, SWT.V_SCROLL);

		// Set Renderers
		DefaultGalleryGroupRenderer gr = new DefaultGalleryGroupRenderer();
		g.setGroupRenderer(gr);

		DefaultGalleryItemRenderer ir = new DefaultGalleryItemRenderer();
		g.setItemRenderer(ir);

		GalleryItem group = new GalleryItem(g, SWT.NONE);
		GalleryItem item1 = new GalleryItem(group, SWT.NONE);
		GalleryItem item2 = new GalleryItem(group, SWT.NONE);

		g.remove(item2);
		
		assertEquals(0, g.indexOf(group));
		assertEquals(0, g.indexOf(item1));
		assertEquals(-1, g.indexOf(item2));

		g.dispose();
	}

}
