/**
 * 
 */
package org.netxms.ui.android.main.views;

import java.text.DecimalFormat;
import java.util.ArrayList;
import java.util.List;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.ui.android.main.views.helpers.ChartItem;
import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Paint.Style;
import android.graphics.Path;
import android.graphics.Point;
import android.graphics.Rect;
import android.graphics.RectF;
import android.view.View;

/**
 * Custom dial chart
 */
public class DialChart extends View
{
	private static final int OUTER_MARGIN_WIDTH = 5;
	private static final int OUTER_MARGIN_HEIGHT = 5;
	private static final int INNER_MARGIN_WIDTH = 5;
	private static final int INNER_MARGIN_HEIGHT = 5;
	private static final int NEEDLE_PIN_RADIUS = 8;
	private static final int SCALE_OFFSET = 30;	// In percents
	private static final int SCALE_WIDTH = 10;	// In percents
	
	private static final int GREEN_ZONE_COLOR = 0xFF00E000;
	private static final int YELLOW_ZONE_COLOR = 0xFFFFF200;
	private static final int RED_ZONE_COLOR = 0xFFE00000;

	private int backgroundColor = 0xFFF0F0F0;
	private int plotAreaColor = 0xFFFFFFFF;
	private int borderColor = 0xFF000000;
	private int scaleColor = 0xFF161616;
	private int needleColor = 0xFF334E71;
	private int needlePinColor = 0xFFEFE4B0;
	private int valueColor = 0xFFFFFFFF;
	
	private boolean titleVisible = true;
	private String title = "";
	private double minValue = 0.0;
	private double maxValue = 100.0;
	private double leftRedZone = 0.0;
	private double leftYellowZone = 0.0;
	private double rightYellowZone = 70.0;
	private double rightRedZone = 90.0;
	private boolean legendVisible;
	private boolean legendInside = true;
	private boolean gridVisible = true;
	private List<ChartItem> parameters = new ArrayList<ChartItem>();
	
	/**
	 * @param context
	 */
	public DialChart(Context context)
	{
		super(context);
	}
	
	/**
	 * Add new parameter
	 * 
	 * @param item
	 * @return
	 */
	public int addParameter(ChartItem item)
	{
		parameters.add(item);
		return parameters.size() - 1;
	}
	
	/**
	 * Update parameter's value
	 * 
	 * @param index
	 * @param value
	 */
	public void updateParameter(int index, int dataType, double value)
	{
		if ((index >= 0) && (index < parameters.size()))
		{
			ChartItem p = parameters.get(index);
			p.dataType = dataType;
			p.value = value;
		}
	}

	/* (non-Javadoc)
	 * @see android.view.View#onDraw(android.graphics.Canvas)
	 */
	@Override
	protected void onDraw(Canvas canvas)
	{
		canvas.drawRGB((backgroundColor >> 16) & 0xFF, (backgroundColor >> 8) & 0xFF, backgroundColor & 0xFF);
		Paint paint = new Paint();
		paint.setAntiAlias(true);

		int top = OUTER_MARGIN_HEIGHT;
		
		// Draw title
		if (titleVisible && (title != null))
		{
			paint.setTextSize(24);
			Rect ext = new Rect();
			paint.getTextBounds(title, 0, title.length(), ext);
			int x = (ext.right < getWidth()) ? (getWidth() - ext.right) / 2 : 0;
			canvas.drawText(title, x, top + ext.height(), paint);
			top += ext.height() + INNER_MARGIN_HEIGHT;
		}
		
		if ((parameters.size() == 0) || (canvas.getWidth() < OUTER_MARGIN_WIDTH * 2) || (canvas.getHeight() < OUTER_MARGIN_HEIGHT * 2))
			return;
		
		int w = (getWidth() - OUTER_MARGIN_WIDTH * 2) / parameters.size();
		int h = getHeight() - OUTER_MARGIN_HEIGHT - top;
		if ((w > 40 * parameters.size()) && (h > 40))
		{
			for(int i = 0; i < parameters.size(); i++)
			{
				renderElement(canvas, paint, parameters.get(i), i * w + getPaddingLeft(), top, w, h);
			}
		}
	}

	/**
	 * @param dataComparisonElement
	 * @param i
	 * @param w
	 */
	private void renderElement(Canvas canvas, Paint paint, ChartItem dci, int x, int y, int w, int h)
	{
		Rect rect = new Rect(x + INNER_MARGIN_WIDTH, y + INNER_MARGIN_HEIGHT, x + w - INNER_MARGIN_WIDTH, y + h - INNER_MARGIN_HEIGHT);
		if (legendVisible && !legendInside)
		{
			Rect ext = new Rect();
			paint.getTextBounds("MMM", 0, 3, ext);
			rect.bottom -= ext.bottom - 4;
		}
		if (rect.height() > rect.width())
		{
			int delta = (rect.height() - rect.width()) / 2;
			rect.top += delta;
			rect.bottom -= delta;
		}
		else
		{
			int delta = (rect.width() - rect.height()) / 2;
			rect.left += delta;
			rect.right -= delta;
		}
		
		double angleValue = (maxValue - minValue) / 270;
		int outerRadius = (rect.width() + 1) / 2;
		int scaleOuterOffset = ((rect.width() / 2) * SCALE_OFFSET / 100);
		int scaleInnerOffset = ((rect.width() / 2) * (SCALE_OFFSET + SCALE_WIDTH) / 100);

		paint.setColor(plotAreaColor);
		canvas.drawArc(new RectF(rect), 0, 360, false, paint);
		
		// Draw zones
		int startAngle = 135;
		startAngle = drawZone(canvas, paint, rect, startAngle, minValue, leftRedZone, angleValue, RED_ZONE_COLOR);
		startAngle = drawZone(canvas, paint, rect, startAngle, leftRedZone, leftYellowZone, angleValue, YELLOW_ZONE_COLOR);
		startAngle = drawZone(canvas, paint, rect, startAngle, leftYellowZone, rightYellowZone, angleValue, GREEN_ZONE_COLOR);
		startAngle = drawZone(canvas, paint, rect, startAngle, rightYellowZone, rightRedZone, angleValue, YELLOW_ZONE_COLOR);
		startAngle = drawZone(canvas, paint, rect, startAngle, rightRedZone, maxValue, angleValue, RED_ZONE_COLOR);
		
		// Draw center part and border
		paint.setColor(plotAreaColor);
		RectF centerRect = new RectF(rect);
		centerRect.inset(scaleInnerOffset, scaleInnerOffset);
		canvas.drawArc(centerRect, 0, 360, false, paint);
		paint.setColor(borderColor);
		paint.setStyle(Style.STROKE);
		paint.setStrokeWidth(2);
		canvas.drawArc(new RectF(rect), 0, 360, false, paint);
		
		// Draw scale
		paint.setColor(scaleColor);
		paint.setStrokeWidth(1);
		int textOffset = ((rect.width() / 2) * SCALE_OFFSET / 200);
		double arcLength = (outerRadius - scaleOuterOffset) * 4.7123889803846898576939650749193;	// r * (270 degrees angle in radians)
		int step = (arcLength >= 200) ? 27 : 54;
		double valueStep = Math.abs((maxValue - minValue) / ((arcLength >= 200) ? 10 : 20)); 
		int textWidth = (int)(Math.sqrt((outerRadius - scaleOuterOffset) * (outerRadius - scaleOuterOffset) / 2) * 0.7);
		setBestFittingFont(paint, "900MM", textWidth, outerRadius - scaleOuterOffset);
		final int cx = rect.centerX();
		final int cy = rect.centerY();
		Rect ext = new Rect();
		for(int i = 135; i <= 405; i += step)
		{
			if (gridVisible)
			{
				Point l1 = positionOnArc(cx, cy, outerRadius - scaleOuterOffset, i);
				Point l2 = positionOnArc(cx, cy, outerRadius - scaleInnerOffset, i);
				canvas.drawLine(l1.x, l1.y, l2.x, l2.y, paint);
			}

			paint.setStyle(Style.FILL);
			String value = roundedMarkValue(i, angleValue, valueStep);
			Point t = positionOnArc(cx, cy, outerRadius - textOffset, i);
			paint.getTextBounds(value, 0, value.length(), ext);
			canvas.drawText(value, t.x - ext.width() / 2, t.y + ext.width() / 2, paint);
			paint.setStyle(Style.STROKE);
		}
		
		centerRect = new RectF(rect);
		centerRect.inset(scaleOuterOffset, scaleOuterOffset);
		canvas.drawArc(centerRect, 135, 270, false, paint);
		
		centerRect = new RectF(rect);
		centerRect.inset(scaleInnerOffset, scaleInnerOffset);
		canvas.drawArc(centerRect, 135, 270, false, paint);
		
		// Draw needle
		double dciValue = dci.value;
		if (dciValue < minValue)
			dciValue = minValue;
		if (dciValue > maxValue)
			dciValue = maxValue;
		int angle = (int)(135 + (dciValue - minValue) / angleValue);
		Point needleEnd = positionOnArc(cx, cy, outerRadius - ((rect.width() / 2) * (SCALE_WIDTH / 2) / 100), angle);
		Point np1 = positionOnArc(cx, cy, NEEDLE_PIN_RADIUS / 2, angle - 90);
		Point np2 = positionOnArc(cx, cy, NEEDLE_PIN_RADIUS / 2, angle + 90);
		paint.setColor(needleColor);
		paint.setStyle(Style.FILL);
		Path path = new Path();
		path.moveTo(np1.x, np1.y);
		path.lineTo(needleEnd.x, needleEnd.y);
		path.lineTo(np2.x, np2.y);
		canvas.drawPath(path, paint);
		canvas.drawArc(new RectF(cx - NEEDLE_PIN_RADIUS, cy - NEEDLE_PIN_RADIUS, cx + NEEDLE_PIN_RADIUS, cy + NEEDLE_PIN_RADIUS), 0, 360, false, paint);
		paint.setColor(needlePinColor);
		canvas.drawArc(new RectF(cx - NEEDLE_PIN_RADIUS / 2, cy - NEEDLE_PIN_RADIUS / 2, cx + NEEDLE_PIN_RADIUS / 2, cy + NEEDLE_PIN_RADIUS / 2), 0, 360, false, paint);
		
		// Draw current value
		String value = getValueAsDisplayString(dci);
		paint.getTextBounds(value, 0, value.length(), ext);
		paint.setColor(needleColor);
		int boxW = Math.max(outerRadius - scaleInnerOffset - 6, ext.width() + 8);
		canvas.drawRoundRect(new RectF(cx - boxW / 2, cy + rect.height() / 4, cx + boxW / 2, cy + rect.height() / 4 + ext.height() + 8), 3, 3, paint);
		paint.setColor(valueColor);
		canvas.drawText(value, cx - ext.width() / 2, cy + rect.height() / 4 + 4 + ext.height(), paint);
		
		// Draw legend, ignore legend position
		if (legendVisible)
		{
			paint.setTextSize(16);
			paint.getTextBounds(dci.name, 0, dci.name.length(), ext);
			paint.setColor(scaleColor);
			if (legendInside)
			{
				canvas.drawText(dci.name, rect.left + ((rect.width() - ext.width()) / 2), rect.top + scaleInnerOffset / 2 + rect.height() / 4 + ext.height(), paint);
			}
			else
			{
				canvas.drawText(dci.name,  rect.left + ((rect.width() - ext.width()) / 2), rect.top + rect.height() + 4 + ext.height(), paint);
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
	private int drawZone(Canvas canvas, Paint paint, Rect outerRect, int startAngle, double minValue, double maxValue, double angleValue, int color)
	{
		if (minValue >= maxValue)
			return startAngle;	// Ignore incorrect zone settings
		
		int angle = (int)((maxValue - minValue) / angleValue);
		if (angle <= 0)
			return startAngle;
		
		RectF rect = new RectF(outerRect);
		int offset = ((outerRect.width() / 2) * SCALE_OFFSET / 100);
		rect.inset(offset, offset);
		
		paint.setColor(color);
		canvas.drawArc(new RectF(rect), startAngle, angle, true, paint);
		return startAngle + angle;
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
		return new Point((int)(radius * Math.cos(Math.toRadians(360 - angle)) + cx), (int)(radius * -Math.sin(Math.toRadians(360 - angle)) + cy));
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
		double value = (angle - 135) * angleValue + minValue;
		double absValue = Math.abs(value);
		if (absValue >= 10000000000L)
		{
			return Long.toString(Math.round(value / 1000000000)) + "G";
		}
		else if (absValue >= 1000000000)
		{
			return new DecimalFormat("#.#").format(value / 1000000000) + "G";
		}
		else if (absValue >= 10000000)
		{
			return Long.toString(Math.round(value / 1000000)) + "M";
		}
		else if (absValue >= 1000000)
		{
			return new DecimalFormat("#.#").format(value / 1000000) + "M";
		}
		else if (absValue >= 10000)
		{
			return Long.toString(Math.round(value / 1000)) + "K";
		}
		else if (absValue >= 1000)
		{
			return new DecimalFormat("#.#").format(value / 1000) + "K";
		}
		else if ((absValue >= 1) && (step >= 1))
		{
			return Long.toString(Math.round(value));
		}
		else if (absValue == 0)
		{
			return "0"; //$NON-NLS-1$
		}
		else
		{
			if (step < 0.00001)
				return Double.toString(value);
			if (step < 0.0001)
				return new DecimalFormat("#.#####").format(value); //$NON-NLS-1$
			if (step < 0.001)
				return new DecimalFormat("#.####").format(value); //$NON-NLS-1$
			if (step < 0.01)
				return new DecimalFormat("#.###").format(value); //$NON-NLS-1$
			return new DecimalFormat("#.##").format(value); //$NON-NLS-1$
		}
	}

	/**
	 * @param dci
	 * @return
	 */
	private String getValueAsDisplayString(ChartItem dci)
	{
		switch(dci.dataType)
		{
			case DataCollectionItem.DT_INT:
				return Integer.toString((int)dci.value);
			case DataCollectionItem.DT_UINT:
			case DataCollectionItem.DT_INT64:
			case DataCollectionItem.DT_UINT64:
				return Long.toString((long)dci.value);
			default:
				return Double.toString(dci.value);
		}
	}
	
	/**
	 * Set best fitting font for given string and bounding rectangle.
	 *
	 * @param paint paint object
	 * @param text text to fit
	 * @param width width of bounding rectangle
	 * @param height height of bounding rectangle
	 */
	private void setBestFittingFont(Paint paint, String text, int width, int height)
	{
		int first = 6;
		int last = 24;
		int curr = last / 2;
		Rect ext = new Rect();
		while(last > first)
		{
			paint.setTextSize(curr);
			paint.getTextBounds(text, 0, text.length(), ext);
			if ((ext.width() <= width) && (ext.height() <= height))
			{
				first = curr + 1;
				curr = first + (last - first) / 2;
			}
			else
			{
				last = curr - 1;
				curr = first + (last - first) / 2;
			}
		}
	}
	
	/**
	 * @return the backgroundColor
	 */
	public int getBackgroundColor()
	{
		return backgroundColor;
	}

	/**
	 * @param backgroundColor the backgroundColor to set
	 */
	public void setBackgroundColor(int backgroundColor)
	{
		this.backgroundColor = backgroundColor;
	}

	/**
	 * @return the plotAreaColor
	 */
	public int getPlotAreaColor()
	{
		return plotAreaColor;
	}

	/**
	 * @param plotAreaColor the plotAreaColor to set
	 */
	public void setPlotAreaColor(int plotAreaColor)
	{
		this.plotAreaColor = plotAreaColor;
	}

	/**
	 * @return the titleVisible
	 */
	public boolean isTitleVisible()
	{
		return titleVisible;
	}

	/**
	 * @param titleVisible the titleVisible to set
	 */
	public void setTitleVisible(boolean titleVisible)
	{
		this.titleVisible = titleVisible;
	}

	/**
	 * @return the title
	 */
	public String getTitle()
	{
		return title;
	}

	/**
	 * @param title the title to set
	 */
	public void setTitle(String title)
	{
		this.title = title;
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
	 * @return the leftYellowZone
	 */
	public double getLeftYellowZone()
	{
		return leftYellowZone;
	}

	/**
	 * @param leftYellowZone the leftYellowZone to set
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

	/**
	 * @return the legendVisible
	 */
	public boolean isLegendVisible()
	{
		return legendVisible;
	}

	/**
	 * @param legendVisible the legendVisible to set
	 */
	public void setLegendVisible(boolean legendVisible)
	{
		this.legendVisible = legendVisible;
	}

	/**
	 * @return the legendInside
	 */
	public boolean isLegendInside()
	{
		return legendInside;
	}

	/**
	 * @param legendInside the legendInside to set
	 */
	public void setLegendInside(boolean legendInside)
	{
		this.legendInside = legendInside;
	}

	/**
	 * @return the gridVisible
	 */
	public boolean isGridVisible()
	{
		return gridVisible;
	}

	/**
	 * @param gridVisible the gridVisible to set
	 */
	public void setGridVisible(boolean gridVisible)
	{
		this.gridVisible = gridVisible;
	}
}
