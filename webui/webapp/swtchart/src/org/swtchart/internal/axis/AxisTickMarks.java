/*******************************************************************************
 * Copyright (c) 2008-2011 SWTChart project. All rights reserved. 
 * 
 * This code is distributed under the terms of the Eclipse Public License v1.0
 * which is available at http://www.eclipse.org/legal/epl-v10.html
 *******************************************************************************/
package org.swtchart.internal.axis;

import java.util.ArrayList;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Display;
import org.swtchart.Chart;
import org.swtchart.IAxis.Position;
import org.swtchart.internal.ChartLayoutData;

/**
 * Axis tick marks.
 */
public class AxisTickMarks implements PaintListener
{
	private static final long serialVersionUID = 1L;

	/** the chart */
	private Chart chart;

	/** the axis */
	private Axis axis;

	/** the foreground color */
	private Color foreground;

	/** the width hint of tick marks area */
	private int widthHint;

	/** the height hint of tick marks area */
	private int heightHint;

	/** the bounds of tick marks area */
	private Rectangle bounds;

	/** the line width */
	protected static final int LINE_WIDTH = 1;

	/** the tick length */
	public static final int TICK_LENGTH = 5;

	/** the default foreground */
	private static final int DEFAULT_FOREGROUND = SWT.COLOR_BLUE;

	/**
	 * Constructor.
	 * 
	 * @param chart
	 *           the chart
	 * @param axis
	 *           the axis
	 */
	public AxisTickMarks(Chart chart, Axis axis)
	{
		this.chart = chart;
		this.axis = axis;

		foreground = Display.getDefault().getSystemColor(DEFAULT_FOREGROUND);
		chart.addPaintListener(this);
	}

	/**
	 * Sets the foreground color.
	 * 
	 * @param color
	 *           the foreground color
	 */
	public void setForeground(Color color)
	{
		if (color == null)
		{
			foreground = Display.getDefault().getSystemColor(DEFAULT_FOREGROUND);
		}
		else
		{
			foreground = color;
		}
	}

	/**
	 * Gets the foreground color.
	 * 
	 * @return the foreground color
	 */
	protected Color getForeground()
	{
		if (foreground.isDisposed())
		{
			foreground = Display.getDefault().getSystemColor(DEFAULT_FOREGROUND);
		}
		return foreground;
	}

	/**
	 * Gets the associated axis.
	 * 
	 * @return the axis
	 */
	public Axis getAxis()
	{
		return axis;
	}

	/**
	 * Updates tick marks layout.
	 */
	protected void updateLayoutData()
	{
		widthHint = SWT.DEFAULT;
		heightHint = SWT.DEFAULT;
		if (!axis.getTick().isVisible())
		{
			widthHint = 0;
			heightHint = 0;
		}
		else
		{
			if (axis.isHorizontalAxis())
			{
				heightHint = Axis.MARGIN + TICK_LENGTH;
			}
			else
			{
				widthHint = TICK_LENGTH + Axis.MARGIN;
			}
		}
	}

	/**
	 * Gets the layout data.
	 * 
	 * @return the layout data
	 */
	public ChartLayoutData getLayoutData()
	{
		return new ChartLayoutData(widthHint, heightHint);
	}

	/**
	 * Sets the bounds on chart panel.
	 * 
	 * @param x
	 *           the x coordinate
	 * @param y
	 *           the y coordinate
	 * @param width
	 *           the width
	 * @param height
	 *           the height
	 */
	public void setBounds(int x, int y, int width, int height)
	{
		bounds = new Rectangle(x, y, width, height);
	}

	/**
	 * Gets the bounds on chart panel.
	 * 
	 * @return the bounds on chart panel
	 */
	protected Rectangle getBounds()
	{
		return bounds;
	}

	/**
	 * Disposes the resources.
	 */
	protected void dispose()
	{
		if (!chart.isDisposed())
		{
			chart.removePaintListener(this);
		}
	}

	/*
	 * @see PaintListener#paintControl(PaintEvent)
	 */
	public void paintControl(PaintEvent e)
	{
		ArrayList<Integer> tickLabelPositions = axis.getTick().getAxisTickLabels().getTickLabelPositions();
		e.gc.setBackground(chart.getBackground());
		e.gc.setForeground(getForeground());
		//Rectangle oldClipping = e.gc.getClipping();
		//e.gc.setClipping(bounds);
		if (axis.isHorizontalAxis())
		{
			drawXTickMarks(e.gc, tickLabelPositions, axis.getPosition());
		}
		else
		{
			drawYTickMarks(e.gc, tickLabelPositions, axis.getPosition());
		}
		//e.gc.setClipping(oldClipping);
	}

	/**
	 * Draw the X tick marks.
	 * 
	 * @param tickLabelPositions
	 *           the tick label positions
	 * @param position
	 *           the axis position
	 * @param gc
	 *           the graphics context
	 */
	private void drawXTickMarks(GC gc, ArrayList<Integer> tickLabelPositions, Position position)
	{

		// draw tick marks
		//gc.setLineStyle(SWT.LINE_SOLID);
		if (axis.isValidCategoryAxis())
		{
			if (tickLabelPositions.size() > 1)
			{
				int step = tickLabelPositions.get(1).intValue() - tickLabelPositions.get(0).intValue();
				for(int i = 0; i < tickLabelPositions.size() + 1; i++)
				{
					int x;
					if (i < tickLabelPositions.size())
					{
						x = (int)(tickLabelPositions.get(i).intValue() - step / 2d);
					}
					else
					{
						x = (int)(tickLabelPositions.get(i - 1).intValue() + step / 2d);
					}
					int y = 0;
					if (position == Position.Secondary)
					{
						y = bounds.height - 1 - LINE_WIDTH - TICK_LENGTH;
					}
					gc.drawLine(bounds.x + x, bounds.y + y, bounds.x + x, bounds.y + y + TICK_LENGTH);
				}
			}
		}
		else
		{
			for(int i = 0; i < tickLabelPositions.size(); i++)
			{
				int x = tickLabelPositions.get(i);
				int y = 0;
				if (position == Position.Secondary)
				{
					y = bounds.height - 1 - LINE_WIDTH - TICK_LENGTH;
				}
				gc.drawLine(bounds.x + x, bounds.y + y, bounds.x + x, bounds.y + y + TICK_LENGTH);
			}
		}

		// draw axis line
		if (position == Position.Primary)
		{
			gc.drawLine(bounds.x, bounds.y, bounds.x + bounds.width - 1, bounds.y);
		}
		else
		{
			gc.drawLine(bounds.x, bounds.y + bounds.height - 1, bounds.x + bounds.width - 1, bounds.y + bounds.height - 1);
		}
	}

	/**
	 * Draw the Y tick marks.
	 * 
	 * @param tickLabelPositions
	 *           the tick label positions
	 * @param position
	 *           the axis position
	 * @param gc
	 *           the graphics context
	 */
	private void drawYTickMarks(GC gc, ArrayList<Integer> tickLabelPositions, Position position)
	{
		// draw tick marks
		//gc.setLineStyle(SWT.LINE_SOLID);
		if (axis.isValidCategoryAxis())
		{
			if (tickLabelPositions.size() > 1)
			{
				int step = tickLabelPositions.get(1).intValue() - tickLabelPositions.get(0).intValue();
				for(int i = 0; i < tickLabelPositions.size() + 1; i++)
				{
					int x = 0;
					int y;
					if (i < tickLabelPositions.size())
					{
						y = (int)(tickLabelPositions.get(i).intValue() - step / 2d);
					}
					else
					{
						y = (int)(tickLabelPositions.get(i - 1).intValue() + step / 2d);
					}

					if (position == Position.Primary)
					{
						x = bounds.width - 1 - LINE_WIDTH - TICK_LENGTH;
					}
					else
					{
						x = LINE_WIDTH;
					}
					gc.drawLine(bounds.x + x, bounds.y + y, bounds.x + x + TICK_LENGTH, bounds.y + y);
				}
			}
		}
		else
		{
			int y = 0;
			for(int i = 0; i < tickLabelPositions.size(); i++)
			{
				int x = 0;
				if (position == Position.Primary)
				{
					x = bounds.width - 1 - LINE_WIDTH - TICK_LENGTH;
				}
				else
				{
					x = LINE_WIDTH;
				}
				y = bounds.height - 1 - tickLabelPositions.get(i);
				gc.drawLine(bounds.x + x, bounds.y + y, bounds.x + x + TICK_LENGTH, bounds.y + y);
			}
		}

		// draw axis line
		if (position == Position.Primary)
		{
			gc.drawLine(bounds.x + bounds.width - 1, bounds.y, bounds.x + bounds.width - 1, bounds.y + bounds.height - 1);
		}
		else
		{
			gc.drawLine(bounds.x, bounds.y, bounds.x, bounds.y + bounds.height - 1);
		}
	}
}
