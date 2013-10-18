package org.netxms.nebula.widgets.gallery.tests;

import java.util.ArrayList;

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

public class GalleryVirtualBehaviorTest extends TestCase {
	Display d = null;
	Shell s = null;
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

	public void testGalleryVirtualNew() {
		Gallery widget = createGallery(SWT.V_SCROLL | SWT.VIRTUAL);
		final ArrayList<Object> setDataCalls = new ArrayList<Object>();
		widget.addListener(SWT.SetData, new Listener() {
			public void handleEvent(Event event) {
				setDataCalls.add(event.item);
			}
		});
		
		assertEquals(0, widget.getItemCount());

		GalleryItem item1 = new GalleryItem(widget, SWT.NONE);
		assertEquals(1, widget.getItemCount());
		assertEquals(item1, widget.getItem(0));
		assertFalse( setDataCalls.contains(item1));

		GalleryItem item2 = new GalleryItem(item1, SWT.NONE);
		assertEquals(1, widget.getItemCount());
		assertEquals(1, item1.getItemCount());
		assertEquals(item2, item1.getItem(0));
		assertFalse( setDataCalls.contains(item2));

		item1.dispose();
		assertEquals( 0, widget.getItemCount());
		assertTrue( item2.isDisposed());

		// Test removeAll
		item1 = new GalleryItem(widget, SWT.NONE);
		assertEquals(1, widget.getItemCount());
		assertEquals(item1, widget.getItem(0));
		assertFalse( setDataCalls.contains(item1));

		widget.removeAll();
		assertEquals(0, widget.getItemCount());
		
	}

	/**
	 * This test is for the reference behavior of Tree
	 */
	public void testTreeVirtualNew() {
		Tree widget = new Tree(s, SWT.VIRTUAL);
		final ArrayList<Object> setDataCalls = new ArrayList<Object>();
		widget.addListener(SWT.SetData, new Listener() {
			public void handleEvent(Event event) {
				setDataCalls.add(event.item);
			}
		});

		
		assertEquals(0, widget.getItemCount());
		TreeItem item1 = new TreeItem(widget, SWT.NONE);
		assertEquals(1, widget.getItemCount());
		assertEquals(item1, widget.getItem(0));
		assertFalse( setDataCalls.contains(item1));
		
		TreeItem item2 = new TreeItem(item1, SWT.NONE);
		assertEquals(1, widget.getItemCount());
		assertEquals(1, item1.getItemCount());
		assertEquals(item2, item1.getItem(0));
		assertFalse( setDataCalls.contains(item2));

		item1.dispose();
		assertEquals( 0, widget.getItemCount());
		assertTrue( item2.isDisposed());
		
		// Test removeAll
		item1 = new TreeItem(widget, SWT.NONE);
		assertEquals(1, widget.getItemCount());
		assertEquals(item1, widget.getItem(0));
		assertFalse( setDataCalls.contains(item1));

		widget.removeAll();
		assertEquals(0, widget.getItemCount());
	
	}
}
