package org.netxms.nxmc.modules.networkmaps.views.helpers;

import org.apache.commons.lang3.SystemUtils;
import org.eclipse.swt.SWT;
import org.eclipse.swt.dnd.Clipboard;
import org.eclipse.swt.dnd.ImageTransfer;
import org.eclipse.swt.dnd.Transfer;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.ImageData;
import org.eclipse.swt.graphics.ImageLoader;
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
      if (fileName == null)
      {
         FileDialog dlg = new FileDialog(shell, SWT.SAVE);
         dlg.setFilterExtensions(new String[] { ".png" });
         dlg.setOverwrite(true);
         fileName = dlg.open();
         if (fileName == null)
            return false;
      }
      
      Image image = viewer.takeSnapshot();
      try
      {
         ImageLoader loader = new ImageLoader();
         loader.data = new ImageData[] { image.getImageData() };
         loader.save(fileName, SWT.IMAGE_PNG);
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
      Transfer imageTransfer = SystemUtils.IS_OS_LINUX ? PngTransfer.getInstance() : ImageTransfer.getInstance();
      final Clipboard clipboard = new Clipboard(viewer.getControl().getDisplay());
      clipboard.setContents(new Object[] { image.getImageData() }, new Transfer[] { imageTransfer });
   }
}
