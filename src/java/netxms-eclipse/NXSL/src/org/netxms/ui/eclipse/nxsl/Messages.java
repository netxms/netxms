package org.netxms.ui.eclipse.nxsl;

import org.eclipse.osgi.util.NLS;

public class Messages extends NLS
{
   private static final String BUNDLE_NAME = "org.netxms.ui.eclipse.nxsl.messages"; //$NON-NLS-1$
   public static String OpenScriptLibrary_Error;
   public static String OpenScriptLibrary_ErrorMsg;
   static
   {
      // initialize resource bundle
      NLS.initializeMessages(BUNDLE_NAME, Messages.class);
   }

   private Messages()
   {
   }
}
