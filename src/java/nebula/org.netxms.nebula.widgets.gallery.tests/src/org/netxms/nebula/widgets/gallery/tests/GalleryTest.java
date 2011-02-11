package org.netxms.nebula.widgets.gallery.tests;

import junit.framework.TestCase;

import org.netxms.nebula.widgets.gallery.DefaultGalleryGroupRenderer;
import org.netxms.nebula.widgets.gallery.DefaultGalleryItemRenderer;
import org.netxms.nebula.widgets.gallery.Gallery;
import org.netxms.nebula.widgets.gallery.GalleryItem;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Shell;

public class GalleryTest extends TestCase {
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

	public void testGalleryAddRemoveClear() {
		Gallery g = createGallery(SWT.V_SCROLL);

		// Create 3 groups
		GalleryItem items[] = new GalleryItem[3];
		for (int i = 0; i < 3; i++) {
			items[i] = new GalleryItem(g, SWT.None);
			items[i].setText("i" + i);
		}

		assertEquals(g.getItemCount(), 3);

		// Add content in groups
		GalleryItem subItems[][] = new GalleryItem[3][3];

		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 3; j++) {
				subItems[i][j] = new GalleryItem(items[i], SWT.None);
				subItems[i][j].setText("si" + i + "_" + j);
			}
			assertEquals(items[i].getItemCount(), 3);
		}

		// dispose si1_1
		subItems[1][1].dispose();
		assertEquals(2, items[1].getItemCount());
		assertEquals("si1_0", items[1].getItem(0).getText());
		assertEquals("si1_2", items[1].getItem(1).getText());

		// dispose si0
		items[0].dispose();
		assertEquals(2, g.getItemCount());
		assertTrue(subItems[0][1].isDisposed());

		// clear si1
		items[1].clear();
		assertEquals("", items[1].getText());
		assertFalse("".equals(subItems[1][0].getText()));
		items[1].setText("si1");

		// clearAll si1
		items[1].clearAll(true);
		assertEquals("", subItems[1][0].getText());
		g.dispose();

	}

	public void testClear() {
		Gallery g = createGallery(SWT.V_SCROLL);

		// Add content
		GalleryItem items[] = new GalleryItem[3];
		GalleryItem subItems[][] = new GalleryItem[3][3];

		for (int i = 0; i < 3; i++) {
			items[i] = new GalleryItem(g, SWT.None);
			items[i].setText("i" + i);
			for (int j = 0; j < 3; j++) {
				subItems[i][j] = new GalleryItem(items[i], SWT.None);
				subItems[i][j].setText("si" + i + "_" + j);
			}
		}

		g.clear(0, true);
		assertEquals("", items[0].getText() ) ;
		assertEquals("", subItems[0][0].getText() ) ;
		assertEquals("", subItems[0][1].getText() ) ;
		assertEquals("", subItems[0][2].getText() ) ;
		
		g.clear(1);
		assertEquals("", items[1].getText() ) ;
		assertFalse("".equals( subItems[1][0].getText() )) ;
		assertFalse("".equals( subItems[1][1].getText() )) ;
		assertFalse("".equals( subItems[1][2].getText() )) ;

		g.clear(2, false);
		assertEquals("", items[2].getText() ) ;
		assertFalse("".equals( subItems[2][0].getText() )) ;
		assertFalse("".equals( subItems[2][1].getText() )) ;
		assertFalse("".equals( subItems[2][2].getText() )) ;

		
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
