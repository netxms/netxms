package org.netxms.ui.eclipse.charts.widgets.internal;

import org.eclipse.swt.SWTException;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Point;

/**
 * Selection rectangle with mouse to zoom in/out.
 */
public class SelectionRectangle
{
	/** the start point of selection */
	private Point startPoint;

	/** the end point of selection */
	private Point endPoint;
	
	private Color color;
	
	/**
	 * Set selection color
	 * 
	 * @param color New selection color
	 */
	public void setColor(Color color)
	{
		this.color = color;
	}

	/**
	 * Sets the start point.
	 * 
	 * @param x
	 *           the X coordinate of start point in pixels
	 * @param y
	 *           the Y coordinate of start point in pixels
	 */
	public void setStartPoint(int x, int y)
	{
		startPoint = new Point(x, y);
	}

	/**
	 * Sets the end point.
	 * 
	 * @param x
	 *           the X coordinate of end point in pixels
	 * @param y
	 *           the X coordinate of end point in pixels
	 */
	public void setEndPoint(int x, int y)
	{
		endPoint = new Point(x, y);
	}

	/**
	 * Gets the horizontal selected range.
	 * 
	 * @return the horizontal selected range in pixels
	 */
	public Point getHorizontalRange()
	{
		if (startPoint == null || endPoint == null)
		{
			return null;
		}

		return new Point(startPoint.x, endPoint.x);
	}

	/**
	 * Gets the vertical selected range.
	 * 
	 * @return the vertical selected range in pixels
	 */
	public Point getVerticalRange()
	{
		if (startPoint == null || endPoint == null)
		{
			return null;
		}

		return new Point(startPoint.y, endPoint.y);
	}

	/**
	 * Check if selection is disposed.
	 * 
	 * @return true if selection is disposed.
	 */
	public boolean isDisposed()
	{
		return startPoint == null;
	}

	/**
	 * Disposes the resource.
	 */
	public void dispose()
	{
		startPoint = null;
		endPoint = null;
	}

	/**
	 * Draws the selection rectangle on chart panel.
	 * 
	 * @param gc
	 *           the graphics context
	 */
	public void draw(GC gc)
	{
		if (startPoint == null || endPoint == null)
		{
			return;
		}

		int minX;
		int maxX;
		if (startPoint.x > endPoint.x)
		{
			minX = endPoint.x;
			maxX = startPoint.x;
		}
		else
		{
			minX = startPoint.x;
			maxX = endPoint.x;
		}

		int minY;
		int maxY;
		if (startPoint.y > endPoint.y)
		{
			minY = endPoint.y;
			maxY = startPoint.y;
		}
		else
		{
			minY = startPoint.y;
			maxY = endPoint.y;
		}

		// Try to paint alpha-blended rectangle
		try
		{
			Color bg = gc.getBackground();
			gc.setBackground(color);
			int alpha = gc.getAlpha();
			gc.setAlpha(32);
			gc.fillRectangle(minX, minY, maxX - minX, maxY - minY);
			gc.setAlpha(alpha);
			gc.setBackground(bg);
		}
		catch(SWTException e)
		{
		}
		
		// Draw bounding rectangle
		Color fg = gc.getBackground();	
		gc.setForeground(color);
		gc.drawRectangle(minX, minY, maxX - minX, maxY - minY);
		gc.setForeground(fg);
	}
}
