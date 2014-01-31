package org.netxms.ui.eclipse.perfview.objecttabs;

import org.eclipse.osgi.util.NLS;

public class Messages extends NLS
{
   private static final String BUNDLE_NAME = "org.netxms.ui.eclipse.perfview.objecttabs.messages"; //$NON-NLS-1$
   public static String PerformanceTab_perfTabLoadingException;
   static
   {
      // initialize resource bundle
      NLS.initializeMessages(BUNDLE_NAME, Messages.class);
   }

   private Messages()
	{
 }


	private static Messages instance = new Messages();

	public static Messages get()
	{
		return instance;
	}

}
