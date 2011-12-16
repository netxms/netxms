/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.client.datacollection.GraphItem;
import org.netxms.client.datacollection.Threshold;
import org.netxms.ui.eclipse.charts.api.DataComparisonChart;
import org.netxms.ui.eclipse.charts.widgets.internal.DataComparisonElement;
import org.netxms.ui.eclipse.shared.SharedColors;
import org.netxms.ui.eclipse.tools.ColorCache;

/**
 * Dial chart implementation
 */
public class DialChart extends GenericChart implements DataComparisonChart, PaintListener, DisposeListener
{
	private static final int OUTER_MARGIN_WIDTH = 5;
	private static final int OUTER_MARGIN_HEIGHT = 5;
	private static final int INNER_MARGIN_WIDTH = 5;
	private static final int INNER_MARGIN_HEIGHT = 5;
	private static final int NEEDLE_PIN_RADIUS = 8;
	private static final int SCALE_WIDTH = 40;	// In percents
	
	private static final RGB GREEN_ZONE_COLOR = new RGB(0, 224, 0);
	private static final RGB YELLOW_ZONE_COLOR = new RGB(255, 242, 0);
	private static final RGB RED_ZONE_COLOR = new RGB(224, 0, 0);
	private static final RGB NEEDLE_COLOR = new RGB(51, 78, 113);
	private static final RGB NEEDLE_PIN_COLOR = new RGB(239, 228, 176);
	
	private List<DataComparisonElement> parameters = new ArrayList<DataComparisonElement>(MAX_CHART_ITEMS);
	private Image chartImage = null;
	private ColorCache colors;
	private double minValue = 0.0;
	private double maxValue = 100.0;
	private double leftRedZone = 0.0;
	private double leftYellowZone = 0.0;
	private double rightYellowZone = 70.0;
	private double rightRedZone = 90.0;
	
	/**
	 * @param parent
	 * @param style
	 */
	public DialChart(Composite parent, int style)
	{
		super(parent, style | SWT.NO_BACKGROUND);
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
		if ((w > 10) && (h > 10))
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
		int scaleWidth = (rect.width / 2) * SCALE_WIDTH / 100;
		
		int cx = rect.x + rect.width / 2 + 1;
		int cy = rect.y + rect.height / 2 + 1;
		Point p1 = positionOnArc(cx, cy, outerRadius, 225);
		Point p2 = positionOnArc(cx, cy, outerRadius, -45);
		gc.setBackground(getColorFromPreferences("Chart.Colors.PlotArea"));
		gc.fillRectangle(p1.x, cy, p2.x - p1.x, p2.y - cy);
		
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
		gc.fillArc(rect.x + scaleWidth, rect.y + scaleWidth, rect.width - scaleWidth * 2, rect.height - scaleWidth * 2, -50, 290);
		gc.drawArc(rect.x, rect.y, rect.width, rect.height, -45, 270);
		gc.drawLine(p1.x, p1.y, p2.x, p2.y);
		
		// Draw needle
		gc.setBackground(colors.create(NEEDLE_COLOR));
		int angle = (int)(225 - (dci.getValue() - minValue) / angleValue);
		Point needleEnd = positionOnArc(cx, cy, outerRadius - ((rect.width / 2) * (SCALE_WIDTH / 2) / 100), angle);
		Point np1 = positionOnArc(cx, cy, NEEDLE_PIN_RADIUS / 2, angle - 90);
		Point np2 = positionOnArc(cx, cy, NEEDLE_PIN_RADIUS / 2, angle + 90);
		gc.fillPolygon(new int[] { np1.x, np1.y, needleEnd.x, needleEnd.y, np2.x, np2.y });
		gc.fillArc(cx - NEEDLE_PIN_RADIUS, cy - NEEDLE_PIN_RADIUS, NEEDLE_PIN_RADIUS * 2 - 1, NEEDLE_PIN_RADIUS * 2 - 1, 0, 360);
		gc.setBackground(colors.create(NEEDLE_PIN_COLOR));
		gc.fillArc(cx - NEEDLE_PIN_RADIUS / 2, cy - NEEDLE_PIN_RADIUS / 2, NEEDLE_PIN_RADIUS - 1, NEEDLE_PIN_RADIUS - 1, 0, 360);
		
		// Draw current value
		String value = getValueAsDisplayString(dci);
		Point ext = gc.textExtent(value, SWT.DRAW_TRANSPARENT);
		gc.drawText(value, p1.x + ((p2.x - p1.x - ext.x) / 2), p1.y - ext.y - 4, true);
		
		// Draw legend, ignore legend position
		if (legendVisible)
		{
			ext = gc.textExtent(dci.getName(), SWT.DRAW_TRANSPARENT);
			gc.drawText(dci.getName(), rect.x + ((rect.width - ext.x) / 2), p1.y + 4, true);
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
		int angle = (int)((maxValue - minValue) / angleValue);
		if (angle <= 0)
			return startAngle;
		
		gc.setBackground(colors.create(color));
		gc.fillArc(rect.x, rect.y, rect.width, rect.height, startAngle, -angle);
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

	/**
	 * @return the minValue
	 */
	public double getMinValue()
	{
		return minValue;
	}

	/**
	 * @param minValue the minValue to set
	 */
	public void setMinValue(double minValue)
	{
		this.minValue = minValue;
	}

	/**
	 * @return the maxValue
	 */
	public double getMaxValue()
	{
		return maxValue;
	}

	/**
	 * @param maxValue the maxValue to set
	 */
	public void setMaxValue(double maxValue)
	{
		this.maxValue = maxValue;
	}

	/**
	 * @return the leftRedZone
	 */
	public double getLeftRedZone()
	{
		return leftRedZone;
	}

	/**
	 * @param leftRedZone the leftRedZone to set
	 */
	public void setLeftRedZone(double leftRedZone)
	{
		this.leftRedZone = leftRedZone;
	}

	/**
	 * @return the leftYelowZone
	 */
	public double getLeftYellowZone()
	{
		return leftYellowZone;
	}

	/**
	 * @param leftYelowZone the leftYelowZone to set
	 */
	public void setLeftYellowZone(double leftYellowZone)
	{
		this.leftYellowZone = leftYellowZone;
	}

	/**
	 * @return the rightYellowZone
	 */
	public double getRightYellowZone()
	{
		return rightYellowZone;
	}

	/**
	 * @param rightYellowZone the rightYellowZone to set
	 */
	public void setRightYellowZone(double rightYellowZone)
	{
		this.rightYellowZone = rightYellowZone;
	}

	/**
	 * @return the rightRedZone
	 */
	public double getRightRedZone()
	{
		return rightRedZone;
	}

	/**
	 * @param rightRedZone the rightRedZone to set
	 */
	public void setRightRedZone(double rightRedZone)
	{
		this.rightRedZone = rightRedZone;
	}
}
