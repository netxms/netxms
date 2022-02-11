/**
 * 
 */
package org.netxms.ui.eclipse.charts.widgets;

import java.util.List;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.FontData;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Label;
import org.netxms.client.datacollection.ChartConfiguration;
import org.netxms.client.datacollection.DataFormatter;
import org.netxms.client.datacollection.GraphItem;
import org.netxms.ui.eclipse.charts.api.DataSeries;
import org.netxms.ui.eclipse.tools.ColorConverter;

/**
 * Chart legend
 */
public class ChartLegend extends Composite
{
   private static final int SYMBOL_WIDTH = 12;
   private static final int EXTENDED_LEGEND_DATA_SPACING = 6;

   private final Color defaultForeground = Display.getCurrent().getSystemColor(SWT.COLOR_BLACK);

   private Chart chart;
   private Label[] headerLabels = new Label[4];
   private Label[][] dataLabels = new Label[ChartConfiguration.MAX_GRAPH_ITEM_COUNT][4];
   private Font headerFont = null;
   private boolean vertical;
   private MouseListener mouseListener;

   /**
    * Create chart legend.
    *
    * @param chart owning chart
    */
   public ChartLegend(Chart chart, boolean vertical)
   {
      super(chart, SWT.NONE);

      this.chart = chart;
      this.vertical = vertical;
      super.setBackground(chart.getBackground());
      updateHeaderFont();

      addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            if (headerFont != null)
               headerFont.dispose();
         }
      });

      mouseListener = new MouseListener() {
         @Override
         public void mouseUp(MouseEvent e)
         {
         }

         @Override
         public void mouseDown(MouseEvent e)
         {
         }

         @Override
         public void mouseDoubleClick(MouseEvent e)
         {
            chart.fireDoubleClickListeners();
         }
      };

      rebuild();
   }

   /**
    * @see org.eclipse.swt.widgets.Control#setForeground(org.eclipse.swt.graphics.Color)
    */
   @Override
   public void setForeground(Color color)
   {
      if (color == null)
         super.setForeground(defaultForeground);
      else
         super.setForeground(color);
   }

   /**
    * @see org.eclipse.swt.widgets.Control#setBackground(org.eclipse.swt.graphics.Color)
    */
   @Override
   public void setBackground(Color color)
   {
      if (color == null)
         super.setBackground(chart.getBackground());
      else
         super.setBackground(color);
   }

   /**
    * Update header font
    */
   private void updateHeaderFont()
   {
      if (headerFont != null)
         headerFont.dispose();

      FontData fd = getFont().getFontData()[0];
      fd.setStyle(SWT.BOLD);
      headerFont = new Font(getDisplay(), fd);
   }

   /**
    * @see org.eclipse.swt.widgets.Control#setFont(org.eclipse.swt.graphics.Font)
    */
   @Override
   public void setFont(Font font)
   {
      super.setFont(font);
      updateHeaderFont();
      for(int i = 0; i < headerLabels.length; i++)
      {
         if (headerLabels[i] != null)
            headerLabels[i].setFont(headerFont);
      }
      chart.layout();
   }

   /**
    * Rebuild legend after configuration or metric set change
    */
   public void rebuild()
   {
      // Dispose all elements
      for(Control c : getChildren())
         c.dispose();

      ChartConfiguration configuration = chart.getConfiguration();
      if (configuration == null)
         return;

      if (configuration.isExtendedLegend())
      {
         GridLayout layout = new GridLayout();
         layout.numColumns = 5;
         layout.marginWidth = 0;
         layout.marginHeight = 0;
         layout.verticalSpacing = 1;
         setLayout(layout);

         // Column headers
         new Label(this, SWT.NONE); // Empty label

         headerLabels[0] = new Label(this, SWT.NONE);
         headerLabels[0].setText("Curr");
         headerLabels[0].setFont(headerFont);
         GridData gd = new GridData();
         gd.horizontalIndent = EXTENDED_LEGEND_DATA_SPACING;
         headerLabels[0].setLayoutData(gd);

         headerLabels[1] = new Label(this, SWT.NONE);
         headerLabels[1].setText("Min");
         headerLabels[1].setFont(headerFont);

         headerLabels[2] = new Label(this, SWT.NONE);
         headerLabels[2].setText("Max");
         headerLabels[2].setFont(headerFont);

         headerLabels[3] = new Label(this, SWT.NONE);
         headerLabels[3].setText("Avg");
         headerLabels[3].setFont(headerFont);

         List<GraphItem> metrics = chart.getItems();
         for(int i = 0; i < metrics.size(); i++)
         {
            int color = metrics.get(i).getColor();
            new LegendLabel(this, (color != -1) ? ColorConverter.rgbFromInt(color) : chart.getPaletteEntry(i).getRGBObject(), metrics.get(i).getDescription());
            for(int j = 0; j < headerLabels.length; j++)
            {
               dataLabels[i][j] = new Label(this, SWT.NONE);
               if (j == 0)
               {
                  gd = new GridData();
                  gd.horizontalIndent = EXTENDED_LEGEND_DATA_SPACING;
                  dataLabels[i][j].setLayoutData(gd);
               }
            }
         }

         refresh();
      }
      else
      {
         RowLayout layout = new RowLayout(vertical ? SWT.VERTICAL : SWT.HORIZONTAL);
         layout.pack = false;
         layout.marginBottom = 0;
         layout.marginTop = 0;
         layout.marginLeft = 0;
         layout.marginRight = 0;
         setLayout(layout);

         List<GraphItem> metrics = chart.getItems();
         for(int i = 0; i < metrics.size(); i++)
         {
            int color = metrics.get(i).getColor();
            new LegendLabel(this, (color != -1) ? ColorConverter.rgbFromInt(color) : chart.getPaletteEntry(i).getRGBObject(), metrics.get(i).getDescription());
         }
      }

      for(Control c : getChildren())
         c.addMouseListener(mouseListener);
   }

   /**
    * Refresh data in extended legend
    */
   public void refresh()
   {
      boolean useMultipliers = chart.getConfiguration().isUseMultipliers();
      int row = 0;
      for(DataSeries s : chart.getDataSeries())
      {
         dataLabels[row][0].setText(useMultipliers ? DataFormatter.roundDecimalValue(s.getCurrentValue(), 0.005, 3) : Double.toString(s.getCurrentValue()));
         dataLabels[row][1].setText(useMultipliers ? DataFormatter.roundDecimalValue(s.getMinValue(), 0.005, 3) : Double.toString(s.getMinValue()));
         dataLabels[row][2].setText(useMultipliers ? DataFormatter.roundDecimalValue(s.getMaxValue(), 0.005, 3) : Double.toString(s.getMaxValue()));
         dataLabels[row][3].setText(useMultipliers ? DataFormatter.roundDecimalValue(s.getAverageValue(), 0.005, 3) : Double.toString(s.getAverageValue()));
         row++;
      }
      chart.layout();
   }

   /**
    * Legend label - color indicator + text
    */
   private class LegendLabel extends Composite
   {
      private Color color;

      LegendLabel(Composite parent, RGB color, String text)
      {
         super(parent, SWT.NONE);

         this.color = chart.getColorCache().create(color);

         setBackground(parent.getBackground());

         GridLayout layout = new GridLayout();
         layout.numColumns = 2;
         layout.horizontalSpacing = 4;
         layout.marginWidth = 0;
         layout.marginHeight = 0;
         setLayout(layout);

         Canvas symbol = new Canvas(this, SWT.NONE) {
            @Override
            public Point computeSize(int wHint, int hHint, boolean changed)
            {
               return new Point(SYMBOL_WIDTH, SYMBOL_WIDTH);
            }
         };
         symbol.setBackground(getBackground());
         symbol.addPaintListener(new PaintListener() {
            @Override
            public void paintControl(PaintEvent e)
            {
               e.gc.setBackground(LegendLabel.this.color);
               e.gc.fillRectangle(getClientArea());
            }
         });
         symbol.setLayoutData(new GridData(SWT.CENTER, SWT.CENTER, false, false));

         Label label = new Label(this, SWT.NONE);
         label.setText(text);
         label.setBackground(getBackground());
         label.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      }
   }
}
