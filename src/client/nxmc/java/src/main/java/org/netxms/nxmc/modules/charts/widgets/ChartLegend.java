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

import java.util.ArrayList;
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
import org.eclipse.swt.graphics.GC;
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
import org.netxms.nxmc.modules.datacollection.widgets.helpers.DataCollectionDisplayInfo;
import org.netxms.nxmc.tools.ColorConverter;

/**
 * Chart legend. Clicking on a legend entry of line, bar, or pie chart toggles corresponding item on the chart; disabled items are
 * shown with strikethrough label and hollow color box but keep their values in extended legend.
 */
public class ChartLegend extends Composite
{
   private static final int SYMBOL_WIDTH = 12;
   private static final int EXTENDED_LEGEND_DATA_SPACING = 6;

   private final Color defaultForeground = Display.getCurrent().getSystemColor(SWT.COLOR_BLACK);

   private Chart chart;
   private Label[] headerLabels = new Label[5];
   private List<Label[]> dataLabels = new ArrayList<>();
   private List<LegendLabel> itemLabels = new ArrayList<>();
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
      updateChildren(this, (c) -> {
         c.setForeground(effectiveColor);
         c.redraw(); // text and color boxes are painted on canvas and do not repaint on color change by themselves
      });
   }

   /**
    * @see org.eclipse.swt.widgets.Control#setBackground(org.eclipse.swt.graphics.Color)
    */
   @Override
   public void setBackground(Color color)
   {
      Color effectiveColor = (color != null) ? color : chart.getBackground();
      super.setBackground(effectiveColor);
      updateChildren(this, (c) -> {
         c.setBackground(effectiveColor);
         c.redraw();
      });
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
    * Check if items of this chart can be toggled from legend.
    *
    * @return true if items can be toggled from legend
    */
   private boolean isItemToggleSupported()
   {
      ChartType type = chart.getType();
      return (type == ChartType.LINE) || (type == ChartType.BAR) || (type == ChartType.PIE);
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

      dataLabels.clear();
      itemLabels.clear();

      if (configuration.isExtendedLegend() && (chart.getType() == ChartType.LINE))
      {
         GridLayout layout = new GridLayout();
         layout.numColumns = 6;
         layout.marginWidth = 0;
         layout.marginHeight = 0;
         layout.verticalSpacing = 1;
         setLayout(layout);

         // Column headers
         new Label(this, SWT.NONE).addMouseListener(mouseListener); // Empty label
         headerLabels[0] = createHeaderLabel("Curr", true);
         headerLabels[1] = createHeaderLabel("Min", false);
         headerLabels[2] = createHeaderLabel("Max", false);
         headerLabels[3] = createHeaderLabel("Avg", false);
         headerLabels[4] = createHeaderLabel("95%", false);

         int count = chart.getItemCount();
         for(int i = 0; i < count; i++)
            createItemRow(i, 5);

         refresh();
      }
      else if (configuration.isExtendedLegend() && ((chart.getType() == ChartType.BAR) || (chart.getType() == ChartType.PIE)))
      {
         boolean showPercent = configuration.isShowPercentInLegend();

         GridLayout layout = new GridLayout();
         layout.numColumns = showPercent ? 3 : 2;
         layout.marginWidth = 0;
         layout.marginHeight = 0;
         layout.verticalSpacing = 1;
         setLayout(layout);

         // Column headers
         new Label(this, SWT.NONE).addMouseListener(mouseListener); // Empty label
         headerLabels[0] = createHeaderLabel("Value", true);
         if (showPercent)
            headerLabels[1] = createHeaderLabel("Pct", false);

         int count = chart.getItemCount();
         for(int i = 0; i < count; i++)
            createItemRow(i, showPercent ? 2 : 1);

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

         int count = chart.getItemCount();
         for(int i = 0; i < count; i++)
            createItemRow(i, 0);
      }
   }

   /**
    * Create column header label for extended legend.
    *
    * @param text header text
    * @param indent true to indent label from previous column
    * @return created label
    */
   private Label createHeaderLabel(String text, boolean indent)
   {
      Label label = new Label(this, SWT.NONE);
      label.setText(text);
      label.setFont(headerFont);
      label.setBackground(getBackground());
      label.setForeground(getForeground());
      if (indent)
      {
         GridData gd = new GridData();
         gd.horizontalIndent = EXTENDED_LEGEND_DATA_SPACING;
         label.setLayoutData(gd);
      }
      label.addMouseListener(mouseListener);
      return label;
   }

   /**
    * Create legend row for chart item: color box with label and, for extended legend, given number of data labels.
    *
    * @param index item index
    * @param dataColumns number of data columns (0 for simple legend)
    */
   private void createItemRow(int index, int dataColumns)
   {
      ChartDciConfig metric = chart.getItem(index);
      int color = metric.getColorAsInt();
      LegendLabel legendLabel = new LegendLabel(this, (color != -1) ? ColorConverter.rgbFromInt(color) : chart.getPaletteEntry(index).getRGBObject(), metric.getLabel());
      legendLabel.setItemEnabled(chart.isItemEnabled(index));
      itemLabels.add(legendLabel);

      MouseListener listener = isItemToggleSupported() ? createItemMouseListener(index) : mouseListener;
      legendLabel.addMouseListener(listener);
      updateChildren(legendLabel, (c) -> c.addMouseListener(listener));

      if (dataColumns > 0)
      {
         Label[] row = new Label[dataColumns];
         for(int j = 0; j < dataColumns; j++)
         {
            row[j] = new Label(this, SWT.NONE);
            row[j].setBackground(getBackground());
            row[j].setForeground(getForeground());
            if (j == 0)
            {
               GridData gd = new GridData();
               gd.horizontalIndent = EXTENDED_LEGEND_DATA_SPACING;
               row[j].setLayoutData(gd);
            }
            row[j].addMouseListener(listener);
         }
         dataLabels.add(row);
      }
   }

   /**
    * Create mouse listener for legend row of given item. Single click toggles item on the chart, double click is passed to chart's
    * double click listeners (preceding single clicks toggle item twice, so item state is unchanged).
    *
    * @param index item index
    * @return mouse listener
    */
   private MouseListener createItemMouseListener(final int index)
   {
      return new MouseAdapter() {
         @Override
         public void mouseUp(MouseEvent e)
         {
            if (e.button == 1)
               toggleItem(index);
         }

         @Override
         public void mouseDoubleClick(MouseEvent e)
         {
            chart.fireDoubleClickListeners();
         }
      };
   }

   /**
    * Toggle item on the chart.
    *
    * @param index item index
    */
   private void toggleItem(int index)
   {
      chart.setItemEnabled(index, !chart.isItemEnabled(index));
   }

   /**
    * Update legend entry after item was enabled or disabled on the chart.
    *
    * @param index item index
    */
   public void updateItemState(int index)
   {
      if (index < itemLabels.size())
         itemLabels.get(index).setItemEnabled(chart.isItemEnabled(index));
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
         int index = 0;
         for(DataSeries s : chart.getDataSeries())
         {
            if (!s.isDataCollectionError() && chart.isItemEnabled(index))
               total += s.getCurrentValue();
            index++;
         }
      }

      int row = 0;
      for(DataSeries s : chart.getDataSeries())
      {
         if (row >= dataLabels.size())
            break;

         Label[] labels = dataLabels.get(row);
         ChartDciConfig item = chart.getItem(row);
         DataFormatter formatter = s.getDataFormatter()
               .setDefaultFormatString("%{u}.3f", "%{m,u}.3f")
               .setFormatString(item.getDisplayFormat())
               .setDataType(DataType.FLOAT)
               .setDefaultForMultipliers(useMultipliers);
         TimeFormatter timeFormatter = DateFormatFactory.getTimeFormatter();
         boolean error = s.isDataCollectionError();
         labels[0].setText(error ? DataCollectionDisplayInfo.getErrorText(s.getLastCollectionError()) : formatter.format(s.getCurrentValueAsString(), timeFormatter));
         if (chart.getType() == ChartType.LINE)
         {
            labels[1].setText(formatter.format(Double.toString(s.getMinValue()), timeFormatter));
            labels[2].setText(formatter.format(Double.toString(s.getMaxValue()), timeFormatter));
            labels[3].setText(formatter.format(Double.toString(s.getAverageValue()), timeFormatter));
            labels[4].setText(formatter.format(Double.toString(s.getPercentile95()), timeFormatter));
         }
         else if (labels.length > 1)
         {
            // Disabled items are excluded from total, so their percentage is meaningless
            labels[1].setText((error || !chart.isItemEnabled(row)) ? "" : String.format("%.2f%%", (total > 0) ? (s.getCurrentValue() / total * 100) : 0.0));
         }
         row++;
      }
      chart.layout(true, true);
   }

   /**
    * Legend label - color indicator + text. Text is drawn on canvas (rather than using Label widget) so that it can be rendered
    * with strikethrough when item is disabled.
    */
   private class LegendLabel extends Composite
   {
      private Color color;
      private String text;
      private boolean itemEnabled = true;
      private Canvas symbol;
      private Canvas label;

      LegendLabel(Composite parent, RGB color, String text)
      {
         super(parent, SWT.NONE);

         this.color = chart.getColorCache().create(color);
         this.text = text;

         setBackground(parent.getBackground());

         GridLayout layout = new GridLayout();
         layout.numColumns = 2;
         layout.horizontalSpacing = 4;
         layout.marginWidth = 0;
         layout.marginHeight = 0;
         setLayout(layout);

         symbol = new Canvas(this, SWT.NONE) {
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
               if (itemEnabled)
               {
                  e.gc.setBackground(LegendLabel.this.color);
                  e.gc.fillRectangle(getClientArea());
               }
               else
               {
                  e.gc.setForeground(LegendLabel.this.color);
                  e.gc.setLineWidth(1);
                  e.gc.drawRectangle(0, 0, SYMBOL_WIDTH - 1, SYMBOL_WIDTH - 1);
               }
            }
         });
         symbol.setLayoutData(new GridData(SWT.CENTER, SWT.CENTER, false, false));

         label = new Canvas(this, SWT.NONE) {
            @Override
            public Point computeSize(int wHint, int hHint, boolean changed)
            {
               GC gc = new GC(this);
               Point size = gc.textExtent(LegendLabel.this.text);
               gc.dispose();
               return size;
            }
         };
         label.setBackground(getBackground());
         label.setForeground(parent.getForeground());
         label.addPaintListener(new PaintListener() {
            @Override
            public void paintControl(PaintEvent e)
            {
               e.gc.setForeground(label.getForeground());
               e.gc.drawText(LegendLabel.this.text, 0, 0, SWT.DRAW_TRANSPARENT);
               if (!itemEnabled)
               {
                  Point size = label.getSize();
                  int y = size.y / 2;
                  e.gc.setLineWidth(1);
                  e.gc.drawLine(0, y, size.x, y);
               }
            }
         });
         label.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      }

      /**
       * Set item state shown by this label.
       *
       * @param enabled true if item is enabled
       */
      void setItemEnabled(boolean enabled)
      {
         if (itemEnabled == enabled)
            return;
         itemEnabled = enabled;
         symbol.redraw();
         label.redraw();
      }
   }
}
