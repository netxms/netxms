/**
 * 
 */
package org.netxms.ui.android.main.views;

import java.util.ArrayList;
import java.util.List;
import org.netxms.ui.android.main.views.helpers.ChartItem;
import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
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
	private static final int NEEDLE_COLOR = 0xFF334E71;
	private static final int NEEDLE_PIN_COLOR = 0xFFEFE4B0;

	private int backgroundColor = 0xFFF0F0F0;
	private int plotAreaColor = 0xFFFFFFFF;
	
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
	private List<Double> values = new ArrayList<Double>();
	
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
		values.add(0.0);
		return parameters.size() - 1;
	}
	
	/**
	 * Update parameter's value
	 * 
	 * @param index
	 * @param value
	 */
	public void updateParameter(int index, double value)
	{
		if ((index >= 0) && (index < values.size()))
			values.set(index, value);
	}

	/* (non-Javadoc)
	 * @see android.view.View#onSizeChanged(int, int, int, int)
	 */
	@Override
	protected void onSizeChanged(int w, int h, int oldw, int oldh)
	{
		// TODO Auto-generated method stub
		super.onSizeChanged(w, h, oldw, oldh);
	}

	/* (non-Javadoc)
	 * @see android.view.View#onDraw(android.graphics.Canvas)
	 */
	@Override
	protected void onDraw(Canvas canvas)
	{
		canvas.drawRGB((backgroundColor >> 16) & 0xFF, (backgroundColor >> 8) & 0xFF, backgroundColor & 0xFF);
		Paint paint = new Paint();
		
		int top = OUTER_MARGIN_HEIGHT;
		
		// Draw title
		if (titleVisible && (title != null))
		{
			Rect ext = new Rect();
			paint.getTextBounds(title, 0, title.length(), ext);
			int x = (ext.right < canvas.getWidth()) ? (canvas.getWidth() - ext.right) / 2 : 0;
			canvas.drawText(title, x, top, paint);
			top += ext.bottom + INNER_MARGIN_HEIGHT;
		}
		
		if ((parameters.size() == 0) || (canvas.getWidth() < OUTER_MARGIN_WIDTH * 2) || (canvas.getHeight() < OUTER_MARGIN_HEIGHT * 2))
			return;
		
		int w = (canvas.getWidth() - OUTER_MARGIN_WIDTH * 2) / parameters.size();
		int h = canvas.getHeight() - OUTER_MARGIN_HEIGHT - top;
		if ((w > 40 * parameters.size()) && (h > 40))
		{
			for(int i = 0; i < parameters.size(); i++)
			{
				renderElement(canvas, paint, parameters.get(i), i * w, top, w, h);
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
		
		/*
		// Draw zones
		int startAngle = 225;
		startAngle = drawZone(gc, rect, startAngle, minValue, leftRedZone, angleValue, RED_ZONE_COLOR);
		startAngle = drawZone(gc, rect, startAngle, leftRedZone, leftYellowZone, angleValue, YELLOW_ZONE_COLOR);
		startAngle = drawZone(gc, rect, startAngle, leftYellowZone, rightYellowZone, angleValue, GREEN_ZONE_COLOR);
		startAngle = drawZone(gc, rect, startAngle, rightYellowZone, rightRedZone, angleValue, YELLOW_ZONE_COLOR);
		startAngle = drawZone(gc, rect, startAngle, rightRedZone, maxValue, angleValue, RED_ZONE_COLOR);
		
		// Draw center part and border
		gc.setBackground(getColorFromPreferences("Chart.Colors.PlotArea")); //$NON-NLS-1$
		gc.setForeground(SharedColors.BLACK);
		gc.fillArc(rect.x + scaleInnerOffset, rect.y + scaleInnerOffset, rect.width - scaleInnerOffset * 2, rect.height - scaleInnerOffset * 2, 0, 360);
		gc.setLineWidth(2);
		gc.drawArc(rect.x, rect.y, rect.width, rect.height, 0, 360);
		gc.setLineWidth(1);
		
		// Draw scale
		gc.setForeground(getColorFromPreferences("Chart.Axis.X.Color")); //$NON-NLS-1$
		int textOffset = ((rect.width / 2) * SCALE_OFFSET / 200);
		double arcLength = (outerRadius - scaleOuterOffset) * 4.7123889803846898576939650749193;	// r * (270 degrees angle in radians)
		int step = (arcLength >= 200) ? 27 : 54;
		double valueStep = Math.abs((maxValue - minValue) / ((arcLength >= 200) ? 10 : 20)); 
		int textWidth = (int)(Math.sqrt((outerRadius - scaleOuterOffset) * (outerRadius - scaleOuterOffset) / 2) * 0.7);
		final Font markFont = WidgetHelper.getBestFittingFont(gc, scaleFonts, "900MM", textWidth, outerRadius - scaleOuterOffset); //$NON-NLS-1$
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
		*/
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
