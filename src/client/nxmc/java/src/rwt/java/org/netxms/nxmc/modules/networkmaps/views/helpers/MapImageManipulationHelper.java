package org.netxms.nxmc.modules.networkmaps.views.helpers;

import java.io.File;
import org.eclipse.swt.SWT;
import org.eclipse.swt.dnd.Clipboard;
import org.eclipse.swt.dnd.ImageTransfer;
import org.eclipse.swt.dnd.Transfer;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.ImageData;
import org.eclipse.swt.graphics.ImageLoader;
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
      Image image = viewer.takeSnapshot();
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
         logger.error("Exception in saveMapImageToFile", e);
         return false;
      }
      finally
      {
         image.dispose();
      }
   }

   public static void copyMapImageToClipboard(ExtendedGraphViewer viewer)
   {
      Image image = viewer.takeSnapshot();
      ImageTransfer imageTransfer = ImageTransfer.getInstance();
      final Clipboard clipboard = new Clipboard(viewer.getControl().getDisplay());
      clipboard.setContents(new Object[] { image.getImageData() }, new Transfer[] { imageTransfer });
   }
}
