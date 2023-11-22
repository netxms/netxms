package org.netxms.nxmc.modules.networkmaps.views.helpers;

import org.eclipse.rap.rwt.RWT;
import org.eclipse.rap.rwt.client.service.JavaScriptExecutor;
import org.eclipse.rap.rwt.widgets.WidgetUtil;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Shell;
import org.netxms.nxmc.modules.networkmaps.widgets.helpers.ExtendedGraphViewer;
import org.netxms.nxmc.modules.networkmaps.widgets.helpers.ExtendedGraphViewer.ExtGraph;
import org.slf4j.Logger;

public class MapImageManipulationHelper
{

   /**
    * Save map image to file
    */
   public static boolean saveMapImageToFile(Shell shell, ExtendedGraphViewer viewer, Logger logger, String fileName)
   {
      JavaScriptExecutor executor = RWT.getClient().getService(JavaScriptExecutor.class);
      if (executor != null)
      {
         StringBuilder js = new StringBuilder();
         js.append("RWTUtil_widgetToImage('");
         js.append(WidgetUtil.getId(((ExtGraph)viewer.getControl()).getCanvas()));
         js.append("', 'div', 'graph.png');");
         executor.execute(js.toString());
         return true;
      }
      return false;
   }

   public static void copyMapImageToClipboard(ExtendedGraphViewer viewer)
   {
      JavaScriptExecutor executor = RWT.getClient().getService(JavaScriptExecutor.class);
      if (executor != null)
      {
         Canvas canvas = ((ExtGraph)viewer.getControl()).getCanvas();

         org.eclipse.draw2d.geometry.Rectangle mapArea = viewer.getGraphControl().getRootLayer().getBounds();
         Point computedSize = canvas.computeSize(SWT.DEFAULT, SWT.DEFAULT, false);
         System.out.println("Comp: x: " + computedSize.x + " y: " + computedSize.y);
         System.out.println("Rect: x: " + mapArea.x + " y: " + mapArea.y);
         canvas.setSize(new Point(mapArea.x, mapArea.y));
         canvas.redraw();
         StringBuilder js = new StringBuilder();
         js.append("RWTUtil_widgetToClipboard('");
         js.append(WidgetUtil.getId(canvas));
         js.append("', 'div', 'graph.png');");
         executor.execute(js.toString());
      }
   }
}
