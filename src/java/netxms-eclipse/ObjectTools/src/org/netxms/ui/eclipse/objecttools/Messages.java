package org.netxms.ui.eclipse.objecttools;

import org.eclipse.osgi.util.NLS;

public class Messages extends NLS
{
   private static final String BUNDLE_NAME = "org.netxms.ui.eclipse.objecttools.messages"; //$NON-NLS-1$
   public static String ObjectToolsAdapterFactory_Error;
   public static String ObjectToolsAdapterFactory_LoaderErrorText;
   static
   {
      // initialize resource bundle
      NLS.initializeMessages(BUNDLE_NAME, Messages.class);
   }

   private Messages()
   {
   }
}
