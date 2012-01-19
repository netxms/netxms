/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.charts.widgets;

import java.text.DecimalFormat;
import java.util.ArrayList;
import java.util.List;
import org.eclipse.jface.preference.PreferenceConverter;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.events.ControlListener;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.client.datacollection.GraphItem;
import org.netxms.client.datacollection.Threshold;
import org.netxms.ui.eclipse.charts.api.ChartColor;
import org.netxms.ui.eclipse.charts.api.DialChart;
import org.netxms.ui.eclipse.charts.widgets.internal.DataComparisonElement;
import org.netxms.ui.eclipse.shared.SharedColors;
import org.netxms.ui.eclipse.tools.ColorCache;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Dial chart implementation
 */
/**
 * @author Victor
 *
 */
public class DialChartWidget extends GenericChart implements DialChart, PaintListener, DisposeListener
{
	private static final int OUTER_MARGIN_WIDTH = 5;
	private static final int OUTER_MARGIN_HEIGHT = 5;
	private static final int INNER_MARGIN_WIDTH = 5;
	private static final int INNER_MARGIN_HEIGHT = 5;
	private static final int NEEDLE_PIN_RADIUS = 8;
	private static final int SCALE_OFFSET = 30;	// In percents
	private static final int SCALE_WIDTH = 10;	// In percents
	
	private static final RGB GREEN_ZONE_COLOR = new RGB(0, 224, 0);
	private static final RGB YELLOW_ZONE_COLOR = new RGB(255, 242, 0);
	private static final RGB RED_ZONE_COLOR = new RGB(224, 0, 0);
	private static final RGB NEEDLE_COLOR = new RGB(51, 78, 113);
	private static final RGB NEEDLE_PIN_COLOR = new RGB(239, 228, 176);
	
	private static Font[] scaleFonts = null;
	private static Font[] valueFonts = null;
	
	private List<DataComparisonElement> parameters = new ArrayList<DataComparisonElement>(MAX_CHART_ITEMS);
	private Image chartImage = null;
	private ColorCache colors;
	private double minValue = 0.0;
	private double maxValue = 100.0;
	private double leftRedZone = 0.0;
	private double leftYellowZone = 0.0;
	private double rightYellowZone = 70.0;
	private double rightRedZone = 90.0;
	private boolean legendInside = true;
	private boolean gridVisible = true;
	
	/**
	 * @param parent
	 * @param style
	 */
	public DialChartWidget(Composite parent, int style)
	{
		super(parent, style | SWT.NO_BACKGROUND);
		
		if (scaleFonts == null)
			createFonts();
		
		colors = new ColorCache(this);
		addPaintListener(this);
		addDisposeListener(this);
		addControlListener(new ControlListener() {
			@Override
			public void controlResized(ControlEvent e)
			{
				if (chartImage != null)
				{
					chartImage.dispose();
					chartImage = null;
				}
				refresh();
			}
			
			@Override
			public void controlMoved(ControlEvent e)
			{
			}
		});
	}
	
	/**
	 * Create fonts
	 */
	private void createFonts()
	{
		scaleFonts = new Font[16];
		for(int i = 0; i < scaleFonts.length; i++)
			scaleFonts[i] = new Font(getDisplay(), "Verdana", i + 6, SWT.NORMAL);

		valueFonts = new Font[16];
		for(int i = 0; i < valueFonts.length; i++)
			valueFonts[i] = new Font(getDisplay(), "Verdana", i + 6, SWT.BOLD);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#initializationComplete()
	 */
	@Override
	public void initializationComplete()
	{
	}

   /**
	 * Create color object from preference string
	 *  
	 * @param name Preference name
	 * @return Color object
	 */
	private Color getColorFromPreferences(final String name)
	{
		return colors.create(PreferenceConverter.getColor(preferenceStore, name));
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#setChartTitle(java.lang.String)
	 */
	@Override
	public void setChartTitle(String title)
	{
		this.title = title;
		refresh();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#setTitleVisible(boolean)
	 */
	@Override
	public void setTitleVisible(boolean visible)
	{
		titleVisible = visible;
		refresh();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#setLegendVisible(boolean)
	 */
	@Override
	public void setLegendVisible(boolean visible)
	{
		legendVisible = visible;
		refresh();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#set3DModeEnabled(boolean)
	 */
	@Override
	public void set3DModeEnabled(boolean enabled)
	{
		displayIn3D = enabled;
		refresh();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#setLogScaleEnabled(boolean)
	 */
	@Override
	public void setLogScaleEnabled(boolean enabled)
	{
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataComparisonChart#addParameter(org.netxms.client.datacollection.GraphItem, double)
	 */
	@Override
	public int addParameter(GraphItem dci, double value)
	{
		parameters.add(new DataComparisonElement(dci, value));
		return parameters.size() - 1;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataComparisonChart#updateParameter(int, double, boolean)
	 */
	@Override
	public void updateParameter(int index, double value, boolean updateChart)
	{
		try
		{
			parameters.get(index).setValue(value);
		}
		catch(IndexOutOfBoundsException e)
		{
		}

 		if (updateChart)
			refresh();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataComparisonChart#updateParameterThresholds(int, org.netxms.client.datacollection.Threshold[])
	 */
	@Override
	public void updateParameterThresholds(int index, Threshold[] thresholds)
	{
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataComparisonChart#setChartType(int)
	 */
	@Override
	public void setChartType(int chartType)
	{
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataComparisonChart#getChartType()
	 */
	@Override
	public int getChartType()
	{
		return DIAL_CHART;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataComparisonChart#setTransposed(boolean)
	 */
	@Override
	public void setTransposed(boolean transposed)
	{
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataComparisonChart#isTransposed()
	 */
	@Override
	public boolean isTransposed()
	{
		return false;
	}

	@Override
	public void setLabelsVisible(boolean visible)
	{
		// TODO Auto-generated method stub
		
	}

	@Override
	public boolean isLabelsVisible()
	{
		// TODO Auto-generated method stub
		return false;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataComparisonChart#setRotation(double)
	 */
	@Override
	public void setRotation(double angle)
	{
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataComparisonChart#getRotation()
	 */
	@Override
	public double getRotation()
	{
		return 0;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#refresh()
	 */
	@Override
	public void refresh()
	{
		render();
		redraw();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#hasAxes()
	 */
	@Override
	public boolean hasAxes()
	{
		return false;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.events.PaintListener#paintControl(org.eclipse.swt.events.PaintEvent)
	 */
	@Override
	public void paintControl(PaintEvent e)
	{
		if (chartImage != null)
			e.gc.drawImage(chartImage, 0, 0);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.events.DisposeListener#widgetDisposed(org.eclipse.swt.events.DisposeEvent)
	 */
	@Override
	public void widgetDisposed(DisposeEvent e)
	{
		if (chartImage != null)
			chartImage.dispose();
	}
	
	/**
	 * Render chart
	 */
	private void render()
	{
		Point size = getSize();
		if (chartImage == null)
		{
			if ((size.x <= 0) || (size.y <= 0))
				return;
			chartImage = new Image(getDisplay(), size.x, size.y);
		}
		
		GC gc = new GC(chartImage);
		gc.setBackground(getColorFromPreferences("Chart.Colors.Background"));
		gc.fillRectangle(0, 0, size.x, size.y);
		gc.setAntialias(SWT.ON);
		gc.setTextAntialias(SWT.ON);
		
		int top = OUTER_MARGIN_HEIGHT;
		
		// Draw title
		if (titleVisible && (title != null))
		{
			Point ext = gc.textExtent(title, SWT.DRAW_TRANSPARENT);
			int x = (ext.x < size.x) ? (size.x - ext.x) / 2 : 0;
			gc.drawText(title, x, top, true);
			top += ext.y + INNER_MARGIN_HEIGHT;
		}
		
		if ((parameters.size() == 0) || (size.x < OUTER_MARGIN_WIDTH * 2) || (size.y < OUTER_MARGIN_HEIGHT * 2))
		{
			gc.dispose();
			return;
		}
		
		int w = (size.x - OUTER_MARGIN_WIDTH * 2) / parameters.size();
		int h = size.y - OUTER_MARGIN_HEIGHT - top;
		if ((w > 40 * parameters.size()) && (h > 40))
		{
			for(int i = 0; i < parameters.size(); i++)
			{
				renderElement(gc, parameters.get(i), i * w, top, w, h);
			}
		}
		
		gc.dispose();
	}

	/**
	 * @param dataComparisonElement
	 * @param i
	 * @param w
	 */
	private void renderElement(GC gc, DataComparisonElement dci, int x, int y, int w, int h)
	{
		Rectangle rect = new Rectangle(x + INNER_MARGIN_WIDTH, y + INNER_MARGIN_HEIGHT, w - INNER_MARGIN_WIDTH * 2, h - INNER_MARGIN_HEIGHT * 2);
		if (legendVisible && !legendInside)
		{
			rect.height -= gc.textExtent("MMM").y - 4;
		}
		if (rect.height > rect.width)
		{
			rect.y += (rect.height - rect.width) / 2;
			rect.height = rect.width;
		}
		else
		{
			rect.x += (rect.width - rect.height) / 2;
			rect.width = rect.height;
		}
		
		double angleValue = (maxValue - minValue) / 270;
		int outerRadius = (rect.width + 1) / 2;
		int scaleOuterOffset = ((rect.width / 2) * SCALE_OFFSET / 100);
		int scaleInnerOffset = ((rect.width / 2) * (SCALE_OFFSET + SCALE_WIDTH) / 100);
		
		int cx = rect.x + rect.width / 2 + 1;
		int cy = rect.y + rect.height / 2 + 1;
		gc.setBackground(getColorFromPreferences("Chart.Colors.PlotArea"));
		gc.fillArc(rect.x, rect.y, rect.width, rect.height, 0, 360);
		
		// Draw zones
		int startAngle = 225;
		startAngle = drawZone(gc, rect, startAngle, minValue, leftRedZone, angleValue, RED_ZONE_COLOR);
		startAngle = drawZone(gc, rect, startAngle, leftRedZone, leftYellowZone, angleValue, YELLOW_ZONE_COLOR);
		startAngle = drawZone(gc, rect, startAngle, leftYellowZone, rightYellowZone, angleValue, GREEN_ZONE_COLOR);
		startAngle = drawZone(gc, rect, startAngle, rightYellowZone, rightRedZone, angleValue, YELLOW_ZONE_COLOR);
		startAngle = drawZone(gc, rect, startAngle, rightRedZone, maxValue, angleValue, RED_ZONE_COLOR);
		
		// Draw center part and border
		gc.setBackground(getColorFromPreferences("Chart.Colors.PlotArea"));
		gc.setForeground(SharedColors.BLACK);
		gc.fillArc(rect.x + scaleInnerOffset, rect.y + scaleInnerOffset, rect.width - scaleInnerOffset * 2, rect.height - scaleInnerOffset * 2, 0, 360);
		gc.setLineWidth(2);
		gc.drawArc(rect.x, rect.y, rect.width, rect.height, 0, 360);
		gc.setLineWidth(1);
		
		// Draw scale
		gc.setForeground(getColorFromPreferences("Chart.Axis.X.Color"));
		int textOffset = ((rect.width / 2) * SCALE_OFFSET / 200);
		double arcLength = (outerRadius - scaleOuterOffset) * 4.7123889803846898576939650749193;	// r * (270 degrees angle in radians)
		int step = (arcLength >= 200) ? 27 : 54;
		double valueStep = Math.abs((maxValue - minValue) / ((arcLength >= 200) ? 10 : 20)); 
		int textWidth = (int)(Math.sqrt((outerRadius - scaleOuterOffset) * (outerRadius - scaleOuterOffset) / 2) * 0.7);
		final Font markFont = WidgetHelper.getBestFittingFont(gc, scaleFonts, "900MM", textWidth, outerRadius - scaleOuterOffset);
		gc.setFont(markFont);
		for(int i = 225; i >= -45; i -= step)
		{
			if (gridVisible)
			{
				Point l1 = positionOnArc(cx, cy, outerRadius - scaleOuterOffset, i);
				Point l2 = positionOnArc(cx, cy, outerRadius - scaleInnerOffset, i);
				gc.drawLine(l1.x, l1.y, l2.x, l2.y);
			}

			String value = roundedMarkValue(i, angleValue, valueStep);
			Point t = positionOnArc(cx, cy, outerRadius - textOffset, i);
			Point ext = gc.textExtent(value, SWT.DRAW_TRANSPARENT);
			gc.drawText(value, t.x - ext.x / 2, t.y - ext.y / 2, SWT.DRAW_TRANSPARENT);
		}
		gc.drawArc(rect.x + scaleOuterOffset, rect.y + scaleOuterOffset, rect.width - scaleOuterOffset * 2, rect.height - scaleOuterOffset * 2, -45, 270);
		gc.drawArc(rect.x + scaleInnerOffset, rect.y + scaleInnerOffset, rect.width - scaleInnerOffset * 2, rect.height - scaleInnerOffset * 2, -45, 270);
		
		// Draw needle
		gc.setBackground(colors.create(NEEDLE_COLOR));
		double dciValue = dci.getValue();
		if (dciValue < minValue)
			dciValue = minValue;
		if (dciValue > maxValue)
			dciValue = maxValue;
		int angle = (int)(225 - (dciValue - minValue) / angleValue);
		Point needleEnd = positionOnArc(cx, cy, outerRadius - ((rect.width / 2) * (SCALE_WIDTH / 2) / 100), angle);
		Point np1 = positionOnArc(cx, cy, NEEDLE_PIN_RADIUS / 2, angle - 90);
		Point np2 = positionOnArc(cx, cy, NEEDLE_PIN_RADIUS / 2, angle + 90);
		gc.fillPolygon(new int[] { np1.x, np1.y, needleEnd.x, needleEnd.y, np2.x, np2.y });
		gc.fillArc(cx - NEEDLE_PIN_RADIUS, cy - NEEDLE_PIN_RADIUS, NEEDLE_PIN_RADIUS * 2 - 1, NEEDLE_PIN_RADIUS * 2 - 1, 0, 360);
		gc.setBackground(colors.create(NEEDLE_PIN_COLOR));
		gc.fillArc(cx - NEEDLE_PIN_RADIUS / 2, cy - NEEDLE_PIN_RADIUS / 2, NEEDLE_PIN_RADIUS - 1, NEEDLE_PIN_RADIUS - 1, 0, 360);
		
		// Draw current value
		String value = getValueAsDisplayString(dci);
		gc.setFont(WidgetHelper.getMatchingSizeFont(valueFonts, markFont));
		Point ext = gc.textExtent(value, SWT.DRAW_TRANSPARENT);
		gc.setLineWidth(3);
		gc.setBackground(colors.create(NEEDLE_COLOR));
		int boxW = Math.max(outerRadius - scaleInnerOffset - 6, ext.x + 8);
		gc.fillRoundRectangle(cx - boxW / 2, cy + rect.height / 4, boxW, ext.y + 6, 3, 3);
		gc.setForeground(SharedColors.WHITE);
		gc.drawText(value, cx - ext.x / 2, cy + rect.height / 4 + 3, true);
		
		// Draw legend, ignore legend position
		if (legendVisible)
		{
			ext = gc.textExtent(dci.getName(), SWT.DRAW_TRANSPARENT);
			gc.setForeground(SharedColors.BLACK);
			if (legendInside)
			{
				gc.setFont(markFont);
				gc.drawText(dci.getName(), rect.x + ((rect.width - ext.x) / 2), rect.y + scaleInnerOffset / 2 + rect.height / 4, true);
			}
			else
			{
				gc.setFont(null);
				gc.drawText(dci.getName(), rect.x + ((rect.width - ext.x) / 2), rect.y + rect.height + 4, true);
			}
		}
	}
	
	/**
	 * Draw colored zone.
	 * 
	 * @param gc
	 * @param rect
	 * @param startAngle
	 * @param maxValue2
	 * @param leftRedZone2
	 * @param angleValue
	 * @param color color
	 * @return
	 */
	private int drawZone(GC gc, Rectangle rect, int startAngle, double minValue, double maxValue, double angleValue, RGB color)
	{
		if (minValue >= maxValue)
			return startAngle;	// Ignore incorrect zone settings
		
		int angle = (int)((maxValue - minValue) / angleValue);
		if (angle <= 0)
			return startAngle;
		
		int offset = ((rect.width / 2) * SCALE_OFFSET / 100);
		
		gc.setBackground(colors.create(color));
		gc.fillArc(rect.x + offset, rect.y + offset, rect.width - offset * 2, rect.height - offset * 2, startAngle, -angle);
		return startAngle - angle;
	}

	/**
	 * Find point coordinates on arc by given angle and radius. Angles are 
	 * interpreted such that 0 degrees is at the 3 o'clock position.
	 * 
	 * @param cx center point X coordinate
	 * @param cy center point Y coordinate
	 * @param radius radius
	 * @param angle angle
	 * @return point coordinates
	 */
	private Point positionOnArc(int cx, int cy, int radius, int angle)
	{
		return new Point((int)(radius * Math.cos(Math.toRadians(angle)) + cx), (int)(radius * -Math.sin(Math.toRadians(angle)) + cy));
	}
	
	/**
	 * Get rounded value for scale mark
	 * 
	 * @param angle
	 * @param angleValue
	 * @return
	 */
	private String roundedMarkValue(int angle, double angleValue, double step)
	{
		double value = (225 - angle) * angleValue + minValue;
		double absValue = Math.abs(value);
		if (absValue >= 1000000000)
		{
			return Long.toString(Math.round(value / 1000000000)) + "G";
		}
		else if (absValue >= 1000000)
		{
			return Long.toString(Math.round(value / 1000000)) + "M";
		}
		else if (absValue >= 1000)
		{
			return Long.toString(Math.round(value / 1000)) + "K";
		}
		else if ((absValue >= 1) && (step >= 1))
		{
			return Long.toString(Math.round(value));
		}
		else if (absValue == 0)
		{
			return "0";
		}
		else
		{
			if (step < 0.00001)
				return Double.toString(value);
			if (step < 0.0001)
				return new DecimalFormat("#.#####").format(value);
			if (step < 0.001)
				return new DecimalFormat("#.####").format(value);
			if (step < 0.01)
				return new DecimalFormat("#.###").format(value);
			return new DecimalFormat("#.##").format(value);
		}
	}

	/**
	 * @param dci
	 * @return
	 */
	private String getValueAsDisplayString(DataComparisonElement dci)
	{
		switch(dci.getObject().getDataType())
		{
			case DataCollectionItem.DT_INT:
				return Integer.toString((int)dci.getValue());
			case DataCollectionItem.DT_UINT:
			case DataCollectionItem.DT_INT64:
			case DataCollectionItem.DT_UINT64:
				return Long.toString((long)dci.getValue());
			default:
				return Double.toString(dci.getValue());
		}
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DialChart#getMinValue()
	 */
	@Override
	public double getMinValue()
	{
		return minValue;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DialChart#setMinValue(double)
	 */
	@Override
	public void setMinValue(double minValue)
	{
		this.minValue = minValue;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DialChart#getMaxValue()
	 */
	@Override
	public double getMaxValue()
	{
		return maxValue;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DialChart#setMaxValue(double)
	 */
	@Override
	public void setMaxValue(double maxValue)
	{
		this.maxValue = maxValue;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DialChart#getLeftRedZone()
	 */
	@Override
	public double getLeftRedZone()
	{
		return leftRedZone;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DialChart#setLeftRedZone(double)
	 */
	@Override
	public void setLeftRedZone(double leftRedZone)
	{
		this.leftRedZone = leftRedZone;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DialChart#getLeftYellowZone()
	 */
	@Override
	public double getLeftYellowZone()
	{
		return leftYellowZone;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DialChart#setLeftYellowZone(double)
	 */
	@Override
	public void setLeftYellowZone(double leftYellowZone)
	{
		this.leftYellowZone = leftYellowZone;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DialChart#getRightYellowZone()
	 */
	@Override
	public double getRightYellowZone()
	{
		return rightYellowZone;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DialChart#setRightYellowZone(double)
	 */
	@Override
	public void setRightYellowZone(double rightYellowZone)
	{
		this.rightYellowZone = rightYellowZone;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DialChart#getRightRedZone()
	 */
	@Override
	public double getRightRedZone()
	{
		return rightRedZone;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DialChart#setRightRedZone(double)
	 */
	@Override
	public void setRightRedZone(double rightRedZone)
	{
		this.rightRedZone = rightRedZone;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DialChart#isLegendInside()
	 */
	@Override
	public boolean isLegendInside()
	{
		return legendInside;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DialChart#setLegendInside(boolean)
	 */
	@Override
	public void setLegendInside(boolean legendInside)
	{
		this.legendInside = legendInside;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#isGridVisible()
	 */
	@Override
	public boolean isGridVisible()
	{
		return gridVisible;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#setGridVisible(boolean)
	 */
	@Override
	public void setGridVisible(boolean visible)
	{
		gridVisible = visible;
	}

	@Override
	public void setBackgroundColor(ChartColor color)
	{
		// TODO Auto-generated method stub
		
	}

	@Override
	public void setPlotAreaColor(ChartColor color)
	{
		// TODO Auto-generated method stub
		
	}

	@Override
	public void setLegendColor(ChartColor foreground, ChartColor background)
	{
		// TODO Auto-generated method stub
		
	}

	@Override
	public void setAxisColor(ChartColor color)
	{
		// TODO Auto-generated method stub
		
	}

	@Override
	public void setGridColor(ChartColor color)
	{
		// TODO Auto-generated method stub
		
	}
}
