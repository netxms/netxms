package org.netxms.nebula.widgets.gallery.tests;

import junit.framework.Test;
import junit.framework.TestSuite;

public class AllTests {

	public static Test suite() {
		TestSuite suite = new TestSuite(
				"Test for org.netxms.nebula.widgets.gallery.tests");
		//$JUnit-BEGIN$
		suite.addTestSuite(GalleryVirtualBehaviorTest.class);
		suite.addTestSuite(Bug212182Test.class);
		suite.addTestSuite(GalleryTest.class);
		suite.addTestSuite(Bug217988Test.class);
		suite.addTestSuite(Bug216204Test.class);
		suite.addTestSuite(Bug276435Test.class);
		suite.addTestSuite(Bug280635Test.class);
		//$JUnit-END$
		return suite;
	}

}
