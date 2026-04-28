package org.netxms.nxmc.modules.networkmaps.views.helpers;

import java.io.File;
import org.eclipse.swt.SWT;
import org.eclipse.swt.dnd.Clipboard;
import org.eclipse.swt.dnd.ImageTransfer;
import org.eclipse.swt.dnd.Transfer;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.ImageData;
import org.eclipse.swt.graphics.ImageLoader;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Shell;
import org.netxms.nxmc.DownloadServiceHandler;
import org.netxms.nxmc.modules.networkmaps.widgets.helpers.ExtendedGraphViewer;
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
    * Save an already-captured snapshot using the RAP download handler. The
    * caller hands over ownership of {@code image}; this method disposes it.
    */
   public static boolean saveImageToFile(Shell shell, Image image, Logger logger, String fileName)
   {
      if (image == null)
         return false;
      try
      {
         ImageLoader loader = new ImageLoader();
         loader.data = new ImageData[] { image.getImageData() };
         File tempFile = File.createTempFile("MapImage_" + image.hashCode(), "_" + System.currentTimeMillis());
         loader.save(tempFile.getAbsolutePath(), SWT.IMAGE_PNG);
         DownloadServiceHandler.addDownload(tempFile.getName(), "map.png", tempFile, "image/png");
         DownloadServiceHandler.startDownload(tempFile.getName());
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
    * hands over ownership of {@code image}; the helper does not dispose it.
    */
   public static void copyImageToClipboard(Display display, Image image)
   {
      if (image == null)
         return;
      ImageTransfer imageTransfer = ImageTransfer.getInstance();
      final Clipboard clipboard = new Clipboard(display);
      clipboard.setContents(new Object[] { image.getImageData() }, new Transfer[] { imageTransfer });
   }
}
