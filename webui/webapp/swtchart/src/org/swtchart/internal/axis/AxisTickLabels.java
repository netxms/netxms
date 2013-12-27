/*******************************************************************************
 * Copyright (c) 2008-2011 SWTChart project. All rights reserved.
 *
 * This code is distributed under the terms of the Eclipse Public License v1.0
 * which is available at http://www.eclipse.org/legal/epl-v10.html
 *******************************************************************************/
package org.swtchart.internal.axis;

import java.math.BigDecimal;
import java.text.Format;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Date;
import java.util.HashMap;
import java.util.Map;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Display;
import org.swtchart.Chart;
import org.swtchart.IAxis.Position;
import org.swtchart.internal.ChartLayoutData;
import org.swtchart.internal.Util;

/**
 * Axis tick labels.
 */
public class AxisTickLabels implements PaintListener
{
	/** the chart */
	private Chart chart;

	/** the axis */
	private Axis axis;

	/** the foreground color */
	private Color foreground;

	/** the width hint of tick labels area */
	private int widthHint;

	/** the height hint of tick labels area */
	private int heightHint;

	/** the bounds of tick labels area */
	private Rectangle bounds;

	/** the array of tick label vales */
	private ArrayList<Double> tickLabelValues;

	/** the array of tick label */
	private ArrayList<String> tickLabels;

	/** the array of tick label position in pixels */
	private ArrayList<Integer> tickLabelPositions;

	/** the array of visibility state of tick label */
	private ArrayList<Boolean> tickVisibilities;

	/** the maximum length of tick labels */
	private int tickLabelMaxLength;

	/** the format for tick labels */
	private Format format;

	/** the default foreground */
	private static final int DEFAULT_FOREGROUND = SWT.COLOR_BLUE;

	/** the default font */
	private static final Font DEFAULT_FONT = Display.getDefault().getSystemFont();

	/** the possible tick steps */
	private Map<Integer, Integer[]> possibleTickSteps;

	/** the time unit for tick step */
	private int timeUnit;

	/** the font */
	private Font font;

	/**
	 * Constructor.
	 * 
	 * @param chart the chart
	 * @param axis the axis
	 */
	protected AxisTickLabels(Chart chart, Axis axis)
	{
		this.chart = chart;
		this.axis = axis;

		tickLabelValues = new ArrayList<Double>();
		tickLabels = new ArrayList<String>();
		tickLabelPositions = new ArrayList<Integer>();
		tickVisibilities = new ArrayList<Boolean>();

		initializePossibleTickSteps();

		font = DEFAULT_FONT;
		foreground = Display.getDefault().getSystemColor(DEFAULT_FOREGROUND);
		chart.addPaintListener(this);
	}

	/**
	 * Initialized the possible tick steps.
	 */
	private void initializePossibleTickSteps()
	{
		final Integer[] milliseconds = { 1, 2, 5, 10, 20, 50, 100, 200, 500, 999 };
		final Integer[] seconds = { 1, 2, 5, 10, 15, 20, 30, 59 };
		final Integer[] minutes = { 1, 2, 3, 5, 10, 15, 20, 30, 59 };
		final Integer[] hours = { 1, 2, 3, 4, 6, 12, 22 };
		final Integer[] dates = { 1, 7, 14, 28 };
		final Integer[] months = { 1, 2, 3, 4, 6, 11 };
		final Integer[] years = { 1, 2, 5, 10, 20, 50, 100, 200, 500, 1000 };

		possibleTickSteps = new HashMap<Integer, Integer[]>();
		possibleTickSteps.put(Calendar.MILLISECOND, milliseconds);
		possibleTickSteps.put(Calendar.SECOND, seconds);
		possibleTickSteps.put(Calendar.MINUTE, minutes);
		possibleTickSteps.put(Calendar.HOUR_OF_DAY, hours);
		possibleTickSteps.put(Calendar.DATE, dates);
		possibleTickSteps.put(Calendar.MONTH, months);
		possibleTickSteps.put(Calendar.YEAR, years);
	}

	/**
	 * Sets the foreground color.
	 * 
	 * @param color the foreground color
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
	 * Updates the tick labels.
	 * 
	 * @param length the axis length
	 */
	protected void update(int length)
	{
		tickLabelValues.clear();
		tickLabels.clear();
		tickLabelPositions.clear();

		if (axis.isValidCategoryAxis())
		{
			updateTickLabelForCategoryAxis(length);
		}
		else if (axis.isLogScaleEnabled())
		{
			updateTickLabelForLogScale(length);
		}
		else if (axis.isDateEnabled())
		{
			updateTickLabelForDateAxis(length);
		}
		else
		{
			updateTickLabelForLinearScale(length);
		}

		updateTickVisibility();
		updateTickLabelMaxLength();
	}

	/**
	 * Updates tick label for date axis.
	 * 
	 * @param length the length of axis
	 */
	private void updateTickLabelForDateAxis(int length)
	{
		double min = axis.getRange().lower;
		double max = axis.getRange().upper;

		double gridStepHint = Math.abs(max - min) / length * axis.getTick().getTickMarkStepHint();

		timeUnit = getTimeUnit(gridStepHint);

		if (timeUnit == Calendar.MILLISECOND || timeUnit == Calendar.SECOND || timeUnit == Calendar.MINUTE
				|| timeUnit == Calendar.HOUR_OF_DAY || timeUnit == Calendar.DATE)
		{

			Integer[] steps = possibleTickSteps.get(timeUnit);
			for(int i = 0; i < steps.length - 1; i++)
			{
				if (gridStepHint < (getPeriodInMillis(timeUnit, steps[i]) + getPeriodInMillis(timeUnit, steps[i + 1])) / 2d)
				{
					BigDecimal gridStep = new BigDecimal(Long.valueOf(getPeriodInMillis(timeUnit, steps[i])).toString());
					updateTickLabelForLinearScale(length, gridStep);
					break;
				}
			}
		}
		else if (timeUnit == Calendar.MONTH || timeUnit == Calendar.YEAR)
		{

			updateTickLabelForMonthOrYear(length, gridStepHint, timeUnit);
		}
	}

	/**
	 * Updates the tick label for month or year. The month and year are handled differently from other units of time, since 1 month
	 * and 1 year can be different depending on which time to start counting.
	 * 
	 * @param length the length of axis
	 * @param gridStepHint the grid step hint
	 * @param tickStepUnit the tick step unit of time
	 */
	private void updateTickLabelForMonthOrYear(int length, double gridStepHint, int tickStepUnit)
	{
		double min = axis.getRange().lower;
		double max = axis.getRange().upper;

		// get initial position
		Calendar cal = Calendar.getInstance();
		cal.setTime(new Date((long)min));
		int month = cal.get(Calendar.MONTH);
		int year = cal.get(Calendar.YEAR);
		if (tickStepUnit == Calendar.MONTH)
		{
			if (month == Calendar.DECEMBER)
			{
				year++;
				month = Calendar.JANUARY;
			}
			else
			{
				month++;
			}
		}
		else if (tickStepUnit == Calendar.YEAR)
		{
			month = Calendar.JANUARY;
			year++;
		}

		// get tick step
		Integer[] steps = possibleTickSteps.get(tickStepUnit);
		int step = steps[steps.length - 1];
		for(int i = 0; i < steps.length - 1; i++)
		{
			if (gridStepHint < (getPeriodInMillis(tickStepUnit, steps[i]) + getPeriodInMillis(tickStepUnit, steps[i + 1])) / 2d)
			{
				step = steps[i];
				break;
			}
		}

		// set tick labels
		cal.clear();
		cal.set(year, month, 1);
		while(cal.getTimeInMillis() < max)
		{
			tickLabelValues.add(Double.valueOf(cal.getTimeInMillis()));
			tickLabels.add(format(cal.getTimeInMillis(), 0));
			int tickLabelPosition = (int)((cal.getTimeInMillis() - min) / (max - min) * length);
			tickLabelPositions.add(tickLabelPosition);
			if (tickStepUnit == Calendar.MONTH)
			{
				month += step;
				if (month + step > Calendar.DECEMBER)
				{
					year++;
					month -= Calendar.DECEMBER + 1;
				}
			}
			else if (tickStepUnit == Calendar.YEAR)
			{
				year += step;
			}
			cal.clear();
			cal.set(year, month, 1);
		}
	}

	/**
	 * Updates tick label for category axis.
	 * 
	 * @param length the length of axis
	 */
	private void updateTickLabelForCategoryAxis(int length)
	{
		String[] series = axis.getCategorySeries();
		if (series == null)
		{
			return;
		}

		int min = (int)axis.getRange().lower;
		int max = (int)axis.getRange().upper;

		int sizeOfTickLabels = (series.length < max - min + 1) ? series.length : max - min + 1;
		int initialIndex = (min < 0) ? 0 : min;

		for(int i = 0; i < sizeOfTickLabels; i++)
		{
			tickLabels.add(series[i + initialIndex]);

			int tickLabelPosition = (int)(length * (i + 0.5) / sizeOfTickLabels);
			tickLabelPositions.add(tickLabelPosition);
		}
	}

	/**
	 * Updates tick label for log scale.
	 * 
	 * @param length the length of axis
	 */
	private void updateTickLabelForLogScale(int length)
	{
		double min = axis.getRange().lower;
		double max = axis.getRange().upper;

		int digitMin = (int)Math.ceil(Math.log10(min));
		int digitMax = (int)Math.ceil(Math.log10(max));

		final BigDecimal MIN = new BigDecimal(new Double(min).toString());
		BigDecimal tickStep = pow(10, digitMin - 1);
		BigDecimal firstPosition;
		
		if (!axis.isHorizontalAxis())
			chart.setCachedTickStep(tickStep.doubleValue());

		if (MIN.remainder(tickStep).doubleValue() <= 0)
		{
			firstPosition = MIN.subtract(MIN.remainder(tickStep));
		}
		else
		{
			firstPosition = MIN.subtract(MIN.remainder(tickStep)).add(tickStep);
		}

		for(int i = digitMin; i <= digitMax; i++)
		{
			for(BigDecimal j = firstPosition; j.doubleValue() <= pow(10, i).doubleValue(); j = j.add(tickStep))
			{
				if (j.doubleValue() > max)
				{
					break;
				}

				if (axis.isDateEnabled())
				{
					Date date = new Date((long)j.doubleValue());
					tickLabels.add(format(date, 0));
				}
				else
				{
					tickLabels.add(format(j.doubleValue(), tickStep.doubleValue()));
				}
				tickLabelValues.add(j.doubleValue());

				int tickLabelPosition = (int)((Math.log10(j.doubleValue()) - Math.log10(min)) / (Math.log10(max) - Math.log10(min)) * length);
				tickLabelPositions.add(tickLabelPosition);
			}
			tickStep = tickStep.multiply(pow(10, 1));
			firstPosition = tickStep.add(pow(10, i));
		}
	}

	/**
	 * Updates tick label for normal scale.
	 * 
	 * @param length axis length (>0)
	 */
	private void updateTickLabelForLinearScale(int length)
	{
		double min = axis.getRange().lower;
		double max = axis.getRange().upper;
		updateTickLabelForLinearScale(length, getGridStep(length, min, max));
	}

	/**
	 * Updates tick label for normal scale.
	 * 
	 * @param length axis length (>0)
	 * @param tickStep the tick step
	 */
	private void updateTickLabelForLinearScale(int length, BigDecimal tickStep)
	{
		double min = axis.getRange().lower;
		double max = axis.getRange().upper;

		if (!axis.isHorizontalAxis())
			chart.setCachedTickStep(tickStep.doubleValue());
		
		final BigDecimal MIN = new BigDecimal(new Double(min).toString());
		BigDecimal firstPosition;

		/* if (min % tickStep <= 0) */
		if (MIN.remainder(tickStep).doubleValue() <= 0)
		{
			/* firstPosition = min - min % tickStep */
			firstPosition = MIN.subtract(MIN.remainder(tickStep));
		}
		else
		{
			/* firstPosition = min - min % tickStep + tickStep */
			firstPosition = MIN.subtract(MIN.remainder(tickStep)).add(tickStep);
		}

		// the unit time starts from 1:00
		if (axis.isDateEnabled())
		{
			BigDecimal zeroOclock = firstPosition.subtract(new BigDecimal(new Double(3600000).toString()));
			if (MIN.compareTo(zeroOclock) == -1)
			{
				firstPosition = zeroOclock;
			}
		}

		for(BigDecimal b = firstPosition; b.doubleValue() <= max; b = b.add(tickStep))
		{
			if (axis.isDateEnabled())
			{
				Date date = new Date((long)b.doubleValue());
				tickLabels.add(format(date, 0));
			}
			else
			{
				tickLabels.add(format(b.doubleValue(), tickStep.doubleValue()));
			}
			tickLabelValues.add(b.doubleValue());

			int tickLabelPosition = (int)((b.doubleValue() - min) / (max - min) * length);
			tickLabelPositions.add(tickLabelPosition);
		}
	}

	/**
	 * Updates the visibility of tick labels.
	 */
	private void updateTickVisibility()
	{

		// initialize the array of tick label visibility state
		tickVisibilities.clear();
		for(int i = 0; i < tickLabelPositions.size(); i++)
		{
			tickVisibilities.add(Boolean.TRUE);
		}

		if (tickLabelPositions.size() == 0 || axis.getTick().getTickLabelAngle() != 0)
		{
			return;
		}

		// set the tick label visibility
		int previousPosition = 0;
		for(int i = 0; i < tickLabelPositions.size(); i++)
		{

			// check if there is enough space to draw tick label
			boolean hasSpaceToDraw = true;
			if (i != 0)
			{
				hasSpaceToDraw = hasSpaceToDraw(previousPosition, tickLabelPositions.get(i), tickLabels.get(i));
			}

			// check if the tick label value is major
			boolean isMajorTick = true;
			if (!axis.isValidCategoryAxis())
			{
				if (axis.isLogScaleEnabled())
				{
					isMajorTick = isMajorTick(tickLabelValues.get(i));
				}

				// check if the same tick label is repeated
				String currentLabel = tickLabels.get(i);
				try
				{
					double value = Double.parseDouble(currentLabel);
					if (value != tickLabelValues.get(i))
					{
						isMajorTick = false;
					}
				}
				catch(NumberFormatException e)
				{
					// label is not decimal value but string
				}
			}

			if (hasSpaceToDraw && isMajorTick)
			{
				previousPosition = tickLabelPositions.get(i);
			}
			else
			{
				tickVisibilities.set(i, Boolean.FALSE);
			}
		}
	}

	/**
	 * Gets the tick step unit.
	 * 
	 * @param gridStepHint the grid step hint
	 * @return the tick step unit.
	 */
	private int getTimeUnit(double gridStepHint)
	{
		final Integer[] units = { Calendar.MILLISECOND, Calendar.SECOND, Calendar.MINUTE, Calendar.HOUR_OF_DAY, Calendar.DATE,
				Calendar.MONTH, Calendar.YEAR };

		for(Integer unit : units)
		{
			Integer[] steps = possibleTickSteps.get(unit);
			if (gridStepHint < (getPeriodInMillis(unit, steps[steps.length - 2]) + getPeriodInMillis(unit, steps[steps.length - 1])) / 2d)
			{
				return unit;
			}
		}
		return Calendar.YEAR;
	}

	/**
	 * Gets the period in milliseconds of given unit of date and amount. The period is calculated based on UTC of January 1, 1970.
	 * 
	 * @param unit the unit of time like <tt>Calendar.YEAR<tt>.
	 * @param amount the amount of period.
	 * @return the period in milliseconds
	 */
	private long getPeriodInMillis(int unit, int amount)
	{
		Calendar cal = Calendar.getInstance();
		cal.setTimeInMillis(0);
		cal.roll(unit, amount);
		return cal.getTimeInMillis();
	}

	/**
	 * Formats the given object.
	 * 
	 * @param obj the object
	 * @return the formatted string
	 */
	private String format(Object obj, double tickStep)
	{
		if (format == null)
		{
			if (axis.isDateEnabled())
			{
				String dateFormat = "yyyyy.MMMMM.dd";
				if (timeUnit == Calendar.MILLISECOND)
				{
					dateFormat = "HH:mm:ss.SSS";
				}
				else if (timeUnit == Calendar.SECOND)
				{
					dateFormat = "HH:mm:ss";
				}
				else if (timeUnit == Calendar.MINUTE)
				{
					dateFormat = "HH:mm";
				}
				else if (timeUnit == Calendar.HOUR_OF_DAY)
				{
					dateFormat = "dd HH:mm";
				}
				else if (timeUnit == Calendar.DATE)
				{
					dateFormat = "MMMMM d";
				}
				else if (timeUnit == Calendar.MONTH)
				{
					dateFormat = "yyyy MMMMM";
				}
				else if (timeUnit == Calendar.YEAR)
				{
					dateFormat = "yyyy";
				}
				return new SimpleDateFormat(dateFormat).format(obj);
			}
			return Chart.roundedDecimalValue((Double)obj, tickStep);
		}
		return format.format(obj);
	}

	/**
	 * Checks if the tick label is major (...,0.01,0.1,1,10,100,...).
	 * 
	 * @param tickValue the tick label value
	 * @return true if the tick label is major
	 */
	private boolean isMajorTick(double tickValue)
	{
		if (!axis.isLogScaleEnabled())
		{
			return true;
		}

		if (Math.log10(tickValue) % 1 == 0)
		{
			return true;
		}

		return false;
	}

	/**
	 * Returns the state indicating if there is a space to draw tick label.
	 * 
	 * @param previousPosition the previously drawn tick label position.
	 * @param tickLabelPosition the tick label position.
	 * @param tickLabel the tick label text
	 * @return true if there is a space to draw tick label
	 */
	private boolean hasSpaceToDraw(int previousPosition, int tickLabelPosition, String tickLabel)
	{
		Point p = Util.getExtentInGC(axis.getTick().getFont(), tickLabel);
		int interval = tickLabelPosition - previousPosition;
		int textLength = axis.isHorizontalAxis() ? p.x : p.y;

		return interval > textLength;
	}

	/**
	 * Gets max length of tick label.
	 */
	private void updateTickLabelMaxLength()
	{
		int maxLength = 0;
		for(int i = 0; i < tickLabels.size(); i++)
		{
			if (tickVisibilities.size() > i && tickVisibilities.get(i) == true)
			{
				Point p = Util.getExtentInGC(axis.getTick().getFont(), tickLabels.get(i));
				if (p.x > maxLength)
				{
					maxLength = p.x;
				}
			}
		}

		if (tickLabelMaxLength != maxLength)
		{
			tickLabelMaxLength = maxLength;
			if (axis.getTick().getTickLabelAngle() != 0)
			{
				chart.updateLayout();
			}
		}
	}

	/**
	 * Calculates the value of the first argument raised to the power of the second argument.
	 * 
	 * @param base the base
	 * @param expornent the exponent
	 * @return the value <tt>a<sup>b</sup></tt> in <tt>BigDecimal</tt>
	 */
	private BigDecimal pow(double base, int expornent)
	{
		BigDecimal value;
		if (expornent > 0)
		{
			value = new BigDecimal(new Double(base).toString()).pow(expornent);
		}
		else
		{
			value = BigDecimal.ONE.divide(new BigDecimal(new Double(base).toString()).pow(-expornent));
		}
		return value;
	}

	/**
	 * Gets the grid step.
	 * 
	 * @param lengthInPixels axis length in pixels
	 * @param min minimum value
	 * @param max maximum value
	 * @return rounded value.
	 */
	private BigDecimal getGridStep(int lengthInPixels, double min, double max)
	{
		if (lengthInPixels <= 0)
		{
			throw new IllegalArgumentException("lengthInPixels must be positive value.");
		}
		if (min >= max)
		{
			throw new IllegalArgumentException("min must be less than max.");
		}

		double length = Math.abs(max - min);
		double gridStepHint = length / lengthInPixels * axis.getTick().getTickMarkStepHint();

		// gridStepHint --> mantissa * 10 ** exponent
		// e.g. 724.1 --> 7.241 * 10 ** 2
		double mantissa = gridStepHint;
		int exponent = 0;
		if (mantissa < 1)
		{
			while(mantissa < 1)
			{
				mantissa *= 10.0;
				exponent--;
			}
		}
		else
		{
			while(mantissa >= 10)
			{
				mantissa /= 10.0;
				exponent++;
			}
		}

		// calculate the grid step with hint.
		BigDecimal gridStep;
		if (mantissa > 7.5)
		{
			// gridStep = 10.0 * 10 ** exponent
			gridStep = BigDecimal.TEN.multiply(pow(10, exponent));
		}
		else if (mantissa > 3.5)
		{
			// gridStep = 5.0 * 10 ** exponent
			gridStep = new BigDecimal(new Double(5).toString()).multiply(pow(10, exponent));
		}
		else if (mantissa > 1.5)
		{
			// gridStep = 2.0 * 10 ** exponent
			gridStep = new BigDecimal(new Double(2).toString()).multiply(pow(10, exponent));
		}
		else
		{
			// gridStep = 1.0 * 10 ** exponent
			gridStep = pow(10, exponent);
		}
		return gridStep;
	}

	/**
	 * Gets the tick label positions.
	 * 
	 * @return the tick label positions
	 */
	public ArrayList<Integer> getTickLabelPositions()
	{
		return tickLabelPositions;
	}

	/**
	 * Gets the tick label values.
	 * 
	 * @return the tick label values
	 */
	protected ArrayList<Double> getTickLabelValues()
	{
		return tickLabelValues;
	}

	/**
	 * Sets the font.
	 * 
	 * @param font the font
	 */
	protected void setFont(Font font)
	{
		if (font == null)
		{
			this.font = DEFAULT_FONT;
		}
		else
		{
			this.font = font;
		}
	}

	/**
	 * Gets the font.
	 * 
	 * @return the font
	 */
	protected Font getFont()
	{
		if (font.isDisposed())
		{
			font = DEFAULT_FONT;
		}
		return font;
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
	 * @param x the x coordinate
	 * @param y the y coordinate
	 * @param width the width
	 * @param height the height
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

	/**
	 * Updates the tick labels layout.
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
				double angle = axis.getTick().getTickLabelAngle();
				heightHint = Axis.MARGIN
						+ (int)(tickLabelMaxLength * Math.sin(Math.toRadians(angle)) + Util.getExtentInGC(getFont(), "dummy").y
								* Math.cos(Math.toRadians(angle)));
			}
			else
			{
				widthHint = tickLabelMaxLength + Axis.MARGIN;
			}
		}
	}

	/*
	 * @see PaintListener#paintControl(PaintEvent)
	 */
	public void paintControl(PaintEvent e)
	{
		if (!axis.getTick().isVisible())
		{
			return;
		}

		e.gc.setBackground(chart.getBackground());
		e.gc.setForeground(getForeground());
		if (axis.isHorizontalAxis())
		{
			drawXTick(e.gc);
		}
		else
		{
			drawYTick(e.gc);
		}
	}

	/**
	 * Draw the X tick.
	 * 
	 * @param gc the graphics context
	 */
	private void drawXTick(GC gc)
	{
		int offset = axis.getTick().getAxisTickMarks().getBounds().x;

		// draw tick labels
		gc.setFont(axis.getTick().getFont());
		int angle = axis.getTick().getTickLabelAngle();
		for(int i = 0; i < tickLabelPositions.size(); i++)
		{
			if (axis.isValidCategoryAxis() || tickVisibilities.get(i) == true)
			{
				String text = tickLabels.get(i);
				int textWidth = gc.textExtent(text).x;
				int textHeight = gc.textExtent(text).y;
				if (angle == 0)
				{
					int x = (int)(tickLabelPositions.get(i) - textWidth / 2d + offset);
					gc.drawText(text, bounds.x + x, bounds.y);
					continue;
				}

				float x, y;
				if (axis.getPosition() == Position.Primary)
				{
					x = (float)(offset + bounds.x + tickLabelPositions.get(i) - textWidth * Math.cos(Math.toRadians(angle)) - textHeight
							/ 2d * Math.sin(Math.toRadians(angle)));
					y = (float)(bounds.y + textWidth * Math.sin(Math.toRadians(angle)));
				}
				else
				{
					x = (float)(offset + bounds.x + tickLabelPositions.get(i) - textHeight / 2d * Math.sin(Math.toRadians(angle)));
					y = (float)(bounds.y + tickLabelMaxLength * Math.sin(Math.toRadians(angle)));
				}
				drawRotatedText(gc, text, x, y, angle);
			}
		}
	}

	/**
	 * Draws the rotated text.
	 * 
	 * @param gc the graphics context
	 * @param text the text
	 * @param x the x coordinate
	 * @param y the y coordinate
	 * @param angle the angle
	 */
	private void drawRotatedText(GC gc, String text, float x, float y, int angle)
	{
	}

	/**
	 * Draw the Y tick.
	 * 
	 * @param gc the graphics context
	 */
	private void drawYTick(GC gc)
	{
		int margin = Axis.MARGIN + AxisTickMarks.TICK_LENGTH;

		// draw tick labels
		gc.setFont(axis.getTick().getFont());
		int figureHeight = gc.textExtent("dummy").y;
		for(int i = 0; i < tickLabelPositions.size(); i++)
		{
			if (tickVisibilities.size() == 0 || tickLabels.size() == 0)
			{
				break;
			}

			if (tickVisibilities.get(i) == true)
			{
				String text = tickLabels.get(i);
				int x = 0;
				if (tickLabels.get(0).startsWith("-") && !text.startsWith("-"))
				{
					x += gc.textExtent("-").x;
				}
				int y = (int)(bounds.height - 1 - tickLabelPositions.get(i) - figureHeight / 2.0 - margin);
				gc.drawText(text, bounds.x + x, bounds.y + y);
			}
		}
	}

	/**
	 * Sets the format for axis tick label. <tt>DecimalFormat</tt> and <tt>DateFormat</tt> should be used for <tt>double[]</tt>
	 * series and <tt>Data[]</tt> series respectively.
	 * <p>
	 * If <tt>null</tt> is set, default format will be used.
	 * 
	 * @param format the format
	 */
	protected void setFormat(Format format)
	{
		this.format = format;
	}

	/**
	 * Gets the format for axis tick label.
	 * 
	 * @return the format
	 */
	protected Format getFormat()
	{
		return format;
	}
}
