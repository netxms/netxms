package org.netxms.nxmc.modules.networkmaps.views.helpers;

import org.apache.commons.lang3.SystemUtils;
import org.eclipse.swt.SWT;
import org.eclipse.swt.dnd.Clipboard;
import org.eclipse.swt.dnd.ImageTransfer;
import org.eclipse.swt.dnd.Transfer;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.ImageData;
import org.eclipse.swt.graphics.ImageLoader;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.FileDialog;
import org.eclipse.swt.widgets.Shell;
import org.netxms.nxmc.modules.networkmaps.widgets.helpers.ExtendedGraphViewer;
import org.netxms.nxmc.tools.PngTransfer;
import org.slf4j.Logger;

public class MapImageManipulationHelper
{

   /**
    * Save map image to file
    */
   public static boolean saveMapImageToFile(Shell shell, ExtendedGraphViewer viewer, Logger logger, String fileName)
   {
      return saveImageToFile(shell, viewer.takeSnapshot(), logger, fileName);
   }

   /**
    * Save an already-captured snapshot to a PNG file. The caller hands over
    * ownership of {@code image}; this method disposes it before returning.
    */
   public static boolean saveImageToFile(Shell shell, Image image, Logger logger, String fileName)
   {
      if (image == null)
         return false;
      try
      {
         if (fileName == null)
         {
            FileDialog dlg = new FileDialog(shell, SWT.SAVE);
            dlg.setFilterExtensions(new String[] { ".png" });
            dlg.setOverwrite(true);
            fileName = dlg.open();
            if (fileName == null)
               return false;
         }
         ImageLoader loader = new ImageLoader();
         loader.data = new ImageData[] { image.getImageData() };
         loader.save(fileName, SWT.IMAGE_PNG);
         return true;
      }
      catch(Exception e)
      {
         logger.error("Exception in saveImageToFile", e);
         return false;
      }
      finally
      {
         image.dispose();
      }
   }

   public static void copyMapImageToClipboard(ExtendedGraphViewer viewer)
   {
      copyImageToClipboard(viewer.getControl().getDisplay(), viewer.takeSnapshot());
   }

   /**
    * Push an already-captured snapshot onto the system clipboard. The caller
    * hands over ownership of {@code image}; the helper does not dispose it
    * (the clipboard implementation copies the data internally), but callers
    * may dispose the original after the call returns.
    */
   public static void copyImageToClipboard(Display display, Image image)
   {
      if (image == null)
         return;
      Transfer imageTransfer = SystemUtils.IS_OS_LINUX ? PngTransfer.getInstance() : ImageTransfer.getInstance();
      final Clipboard clipboard = new Clipboard(display);
      clipboard.setContents(new Object[] { image.getImageData() }, new Transfer[] { imageTransfer });
   }
}
