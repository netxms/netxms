/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.nxmc.modules.charts.widgets;

import java.util.List;
import java.util.function.Consumer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.MouseAdapter;
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
import org.netxms.client.constants.DataType;
import org.netxms.client.datacollection.ChartConfiguration;
import org.netxms.client.datacollection.ChartDciConfig;
import org.netxms.client.datacollection.DataFormatter;
import org.netxms.client.datacollection.DataSeries;
import org.netxms.client.datacollection.TimeFormatter;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.modules.charts.api.ChartType;
import org.netxms.nxmc.tools.ColorConverter;

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
    * @param foreground foreground color for legend
    * @param vertical true if legend should have vertical orientation
    */
   public ChartLegend(Chart chart, Color foreground, boolean vertical)
   {
      super(chart, SWT.NONE);

      this.chart = chart;
      this.vertical = vertical;
      super.setBackground(chart.getBackground());
      super.setForeground(foreground);
      updateHeaderFont();

      addDisposeListener((e) -> {
         if (headerFont != null)
            headerFont.dispose();
      });

      mouseListener = new MouseAdapter() {
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
      Color effectiveColor = (color != null) ? color : defaultForeground;
      super.setForeground(effectiveColor);
      updateChildren(this, (c) -> c.setForeground(effectiveColor));
   }

   /**
    * @see org.eclipse.swt.widgets.Control#setBackground(org.eclipse.swt.graphics.Color)
    */
   @Override
   public void setBackground(Color color)
   {
      Color effectiveColor = (color != null) ? color : chart.getBackground(); 
      super.setBackground(effectiveColor);
      updateChildren(this, (c) -> c.setBackground(effectiveColor));
   }

   /**
    * Update child controls recursively by executing given operation on each of them.
    *
    * @param parent parent composite
    * @param operation operation to execute
    */
   private void updateChildren(Composite parent, Consumer<Control> operation)
   {
      for(Control c : parent.getChildren())
      {
         operation.accept(c);
         if (c instanceof Composite)
            updateChildren((Composite)c, operation);
      }
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

      if (configuration.isExtendedLegend() && (chart.getType() == ChartType.LINE))
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
         headerLabels[0].setBackground(getBackground());
         headerLabels[0].setForeground(getForeground());
         GridData gd = new GridData();
         gd.horizontalIndent = EXTENDED_LEGEND_DATA_SPACING;
         headerLabels[0].setLayoutData(gd);

         headerLabels[1] = new Label(this, SWT.NONE);
         headerLabels[1].setText("Min");
         headerLabels[1].setFont(headerFont);
         headerLabels[1].setBackground(getBackground());
         headerLabels[1].setForeground(getForeground());

         headerLabels[2] = new Label(this, SWT.NONE);
         headerLabels[2].setText("Max");
         headerLabels[2].setFont(headerFont);
         headerLabels[2].setBackground(getBackground());
         headerLabels[2].setForeground(getForeground());

         headerLabels[3] = new Label(this, SWT.NONE);
         headerLabels[3].setText("Avg");
         headerLabels[3].setFont(headerFont);
         headerLabels[3].setBackground(getBackground());
         headerLabels[3].setForeground(getForeground());

         List<ChartDciConfig> metrics = chart.getItems();
         for(int i = 0; i < metrics.size(); i++)
         {
            int color = metrics.get(i).getColorAsInt();
            new LegendLabel(this, (color != -1) ? ColorConverter.rgbFromInt(color) : chart.getPaletteEntry(i).getRGBObject(), metrics.get(i).getLabel());
            for(int j = 0; j < 4; j++)
            {
               dataLabels[i][j] = new Label(this, SWT.NONE);
               dataLabels[i][j].setBackground(getBackground());
               dataLabels[i][j].setForeground(getForeground());
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
      else if (configuration.isExtendedLegend() && ((chart.getType() == ChartType.BAR) || (chart.getType() == ChartType.PIE)))
      {
         GridLayout layout = new GridLayout();
         layout.numColumns = 3;
         layout.marginWidth = 0;
         layout.marginHeight = 0;
         layout.verticalSpacing = 1;
         setLayout(layout);

         // Column headers
         new Label(this, SWT.NONE); // Empty label

         headerLabels[0] = new Label(this, SWT.NONE);
         headerLabels[0].setText("Value");
         headerLabels[0].setFont(headerFont);
         headerLabels[0].setBackground(getBackground());
         headerLabels[0].setForeground(getForeground());
         GridData gd = new GridData();
         gd.horizontalIndent = EXTENDED_LEGEND_DATA_SPACING;
         headerLabels[0].setLayoutData(gd);

         headerLabels[1] = new Label(this, SWT.NONE);
         headerLabels[1].setText("Pct");
         headerLabels[1].setFont(headerFont);
         headerLabels[1].setBackground(getBackground());
         headerLabels[1].setForeground(getForeground());

         List<ChartDciConfig> metrics = chart.getItems();
         for(int i = 0; i < metrics.size(); i++)
         {
            int color = metrics.get(i).getColorAsInt();
            new LegendLabel(this, (color != -1) ? ColorConverter.rgbFromInt(color) : chart.getPaletteEntry(i).getRGBObject(), metrics.get(i).getLabel());
            for(int j = 0; j < 2; j++)
            {
               dataLabels[i][j] = new Label(this, SWT.NONE);
               dataLabels[i][j].setBackground(getBackground());
               dataLabels[i][j].setForeground(getForeground());
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

         List<ChartDciConfig> metrics = chart.getItems();
         for(int i = 0; i < metrics.size(); i++)
         {
            int color = metrics.get(i).getColorAsInt();
            new LegendLabel(this, (color != -1) ? ColorConverter.rgbFromInt(color) : chart.getPaletteEntry(i).getRGBObject(), metrics.get(i).getLabel());
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

      double total = 0;
      if ((chart.getType() == ChartType.BAR) || (chart.getType() == ChartType.PIE))
      {
         for(DataSeries s : chart.getDataSeries())
            total += s.getCurrentValue();
      }

      int row = 0;
      for(DataSeries s : chart.getDataSeries())
      {
         ChartDciConfig item = chart.getItem(row);
         DataFormatter formatter = s.getDataFormatter()
               .setDefaultFormatString("%{u}.3f", "%{m,u}.3f")
               .setFormatString(item.getDisplayFormat())
               .setDataType(DataType.FLOAT)
               .setDefaultForMultipliers(useMultipliers);
         TimeFormatter timeFormatter = DateFormatFactory.getTimeFormatter();
         dataLabels[row][0].setText(formatter.format(s.getCurrentValueAsString(), timeFormatter));
         if (chart.getType() == ChartType.LINE)
         {
            dataLabels[row][1].setText(formatter.format(Double.toString(s.getMinValue()), timeFormatter));
            dataLabels[row][2].setText(formatter.format(Double.toString(s.getMaxValue()), timeFormatter));
            dataLabels[row][3].setText(formatter.format(Double.toString(s.getAverageValue()), timeFormatter));
         }
         else
         {
            dataLabels[row][1].setText(String.format("%.2f%%", (total > 0) ? (s.getCurrentValue() / total * 100) : 0.0));
         }
         row++;
      }
      chart.layout(true, true);
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
         label.setForeground(parent.getForeground());
         label.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      }
   }
}
