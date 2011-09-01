package org.netxms.ui.eclipse.perfview.views;

import java.util.Random;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.part.ViewPart;
import org.netxms.ui.eclipse.tools.WidgetHelper;

public class StatusView extends ViewPart {

   public static final String ID = "org.netxms.ui.eclipse.perfview.views.YesNoView";

   @Override
   public void createPartControl(final Composite parent) {
      final GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      parent.setLayout(layout);

      layout.numColumns = 2;

      for (int i = 0; i < 5; i++) {
         createElement(parent, "Element " + i);
      }
   }

   private void createElement(final Composite parent, final String title) {
      final Canvas drawingCanvas = new Canvas(parent, SWT.BORDER);
      final GridData data = new GridData(GridData.FILL_BOTH);
      drawingCanvas.setLayoutData(data);

      drawingCanvas.addPaintListener(new PaintListener() {

         @Override
         public void paintControl(PaintEvent e) {
            e.gc.setAntialias(SWT.ON);
            Canvas canvas = (Canvas) e.widget;

            final Point textExtent = e.gc.textExtent(title);

            final Rectangle clientArea = canvas.getClientArea();

            e.gc.drawText(title, clientArea.width / 2 - textExtent.x / 2, clientArea.height - textExtent.y);

            int size = Math.min(clientArea.width, clientArea.height);

            final Color color = parent.getShell().getDisplay().getSystemColor(new Random().nextBoolean() ? SWT.COLOR_RED : SWT.COLOR_GREEN);
            e.gc.setBackground(color);

            size -= textExtent.y;
            size -= WidgetHelper.INNER_SPACING;

            final int x = (clientArea.width / 2) - (size / 2);
            final int y = (clientArea.height / 2) - (size / 2) - (textExtent.y / 2);
            e.gc.fillOval(x, y, size, size);
         }
      });
   }

   @Override
   public void setFocus() {
   }

}
