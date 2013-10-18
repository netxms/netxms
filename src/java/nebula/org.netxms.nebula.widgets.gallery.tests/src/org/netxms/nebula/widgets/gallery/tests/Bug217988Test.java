package org.netxms.nebula.widgets.gallery.tests;

import java.util.Arrays;

import junit.framework.TestCase;

import org.eclipse.jface.viewers.ITreeContentProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.netxms.nebula.jface.galleryviewer.GalleryTreeViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Shell;

public class Bug217988Test extends TestCase {

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

	// Items class
	static public class Foo {

		private String name;
		private int value;

		public Foo(String message) {
			this.name = message;
		}

		public Foo(String name, int value) {
			this.name = name;
			this.value = value;
		}

		public String toString() {
			return name;
		}

		public String getName() {
			return name;
		}

		public int getValue() {
			return value;
		}

	}

	// Content provider
	public static class GalleryTestContentProvider implements ITreeContentProvider {

		public static final int NUM_GROUPS = 1;
		public static final int NUM_ITEMS = 20;

		String[] groups = new String[NUM_GROUPS];
		Foo[][] items = new Foo[NUM_GROUPS][NUM_ITEMS];

		public GalleryTestContentProvider() {
			for (int i = 0; i < NUM_GROUPS; i++) {
				groups[i] = "Group " + (i + 1);
				for (int j = 0; j < NUM_ITEMS; j++) {
					items[i][j] = new Foo("Item " + ((i * NUM_ITEMS) + (j + 1)), (i * NUM_ITEMS) + (j + 1));
				}
			}
		}

		public Object[] getChildren(Object parentElement) {
			int idx = Arrays.asList(groups).indexOf(parentElement);
			return items[idx];
		}

		public Object getParent(Object element) {
			return null;
		}

		public boolean hasChildren(Object element) {
			if (element instanceof Foo) {
				return false;
			}
			return ((String) element).startsWith("Group");
		}

		public Object[] getElements(Object inputElement) {
			return groups;
		}

		public void dispose() {
		}

		public void inputChanged(Viewer viewer, Object oldInput, Object newInput) {
		}

	}

	public static class GalleryTestLabelProvider extends LabelProvider {
		
		public Image getImage(Object element) {
			return null;
		}

	}

	public void testBug217988() {
		GalleryTreeViewer viewer = new GalleryTreeViewer(s);

		viewer.setContentProvider(new GalleryTestContentProvider());
		viewer.setLabelProvider(new GalleryTestLabelProvider());
		viewer.setComparator(new ViewerComparator());
		viewer.setInput(new Object());

		// Setting input raised a NPE
		viewer.setInput(new Object());
	}

}
