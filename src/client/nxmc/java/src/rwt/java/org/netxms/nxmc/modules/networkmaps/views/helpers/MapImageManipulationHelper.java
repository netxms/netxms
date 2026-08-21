package org.netxms.nxmc.modules.networkmaps.views.helpers;

import org.eclipse.rap.rwt.RWT;
import org.eclipse.rap.rwt.client.service.JavaScriptExecutor;
import org.eclipse.rap.rwt.widgets.WidgetUtil;
import org.eclipse.swt.widgets.Shell;
import org.netxms.nxmc.modules.networkmaps.widgets.helpers.ExtendedGraphViewer;
import org.slf4j.Logger;

/**
 * Helper for capturing network map image in web client. RAP images are immutable and cannot be used as drawing target, so map content
 * cannot be rendered into server side image buffer. Instead, currently rendered map is captured on client side from the browser DOM
 * (via {@code domtoimage} helper), same way as it is done for geographical maps, charts, and dashboards. As a consequence only visible
 * part of the map is captured.
 */
public class MapImageManipulationHelper
{
   /**
    * Save map image to file. Shell and logger are unused in web client (capture and download are browser driven), and file name is used
    * as suggested download name, defaulting to "map.png" when null.
    *
    * @return true when the request was dispatched to the client
    */
   public static boolean saveMapImageToFile(Shell shell, ExtendedGraphViewer viewer, Logger logger, String fileName)
   {
      JavaScriptExecutor executor = RWT.getClient().getService(JavaScriptExecutor.class);
      if (executor == null)
         return false;
      StringBuilder js = new StringBuilder();
      js.append("RWTUtil_widgetToImage('");
      js.append(WidgetUtil.getId(viewer.getControl()));
      js.append("', 'div', '");
      js.append((fileName != null) ? fileName : "map.png");
      js.append("');");
      executor.execute(js.toString());
      return true;
   }

   /**
    * Copy map image to clipboard using browser's clipboard API.
    */
   public static void copyMapImageToClipboard(ExtendedGraphViewer viewer)
   {
      JavaScriptExecutor executor = RWT.getClient().getService(JavaScriptExecutor.class);
      if (executor == null)
         return;
      StringBuilder js = new StringBuilder();
      js.append("RWTUtil_widgetToClipboard('");
      js.append(WidgetUtil.getId(viewer.getControl()));
      js.append("', 'div');");
      executor.execute(js.toString());
   }
}
