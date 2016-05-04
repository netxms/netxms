package org.netxms.ui.eclipse.tools;

import java.io.IOException;
import java.io.InputStream;
import java.util.PropertyResourceBundle;
import java.util.ResourceBundle;
import org.eclipse.ui.commands.ActionHandler;
import org.eclipse.ui.handlers.IHandlerService;
import org.eclipse.ui.part.ViewPart;
import org.eclipse.ui.texteditor.FindReplaceAction;
import org.netxms.ui.eclipse.console.Activator;

public class NXFindAndReplaceAction
{
   public static FindReplaceAction getFindReplaceAction(ViewPart wp)
   {
      FindReplaceAction action = null;
      try
      {
         action = new FindReplaceAction(getResourceBundle(wp), "actions.find_and_replace.", wp); //$NON-NLS-1$
         IHandlerService hs = (IHandlerService)wp.getSite().getService(IHandlerService.class);
         hs.activateHandler("org.eclipse.ui.edit.findReplace", new ActionHandler(action));        //$NON-NLS-1$
      }
      catch(IOException e)
      {
         e.printStackTrace();
      }
      return action;
   }
   
   /**
    * Get resource bundle
    * @return
    * @throws IOException
    */
   private static ResourceBundle getResourceBundle(ViewPart wp) throws IOException
   {
      InputStream in = null;
      String resource = "resource.properties"; //$NON-NLS-1$
      ClassLoader loader = Activator.getDefault().getClass().getClassLoader();
      if (loader != null)
      {
         in = loader.getResourceAsStream(resource);
      }
      else
      {
         in = ClassLoader.getSystemResourceAsStream(resource);
      }

      return new PropertyResourceBundle(in);
   }
}
