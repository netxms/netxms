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
package org.netxms.ui.eclipse.osm.tools;

/**
 * Represents rectangular area
 */
public class Area
{
	private double xLow;
	private double yLow;
	private double xHigh;
	private double yHigh;
	
	/**
	 * @param xLow
	 * @param yLow
	 * @param xHigh
	 * @param yHigh
	 */
	public Area(double xLow, double yLow, double xHigh, double yHigh)
	{
		if (xLow < xHigh)
		{
			this.xLow = xLow;
			this.xHigh = xHigh;
		}
		else
		{
			this.xLow = xHigh;
			this.xHigh = xLow;
		}
		if (yLow < yHigh)
		{
			this.yLow = yLow;
			this.yHigh = yHigh;
		}
		else
		{
			this.yLow = yHigh;
			this.yHigh = yLow;
		}
	}

	/**
	 * @return the xLow
	 */
	public double getxLow()
	{
		return xLow;
	}

	/**
	 * @return the yLow
	 */
	public double getyLow()
	{
		return yLow;
	}

	/**
	 * @return the xHigh
	 */
	public double getxHigh()
	{
		return xHigh;
	}

	/**
	 * @return the yHigh
	 */
	public double getyHigh()
	{
		return yHigh;
	}
	
	/**
	 * Check if given point is inside area.
	 * 
	 * @param x
	 * @param y
	 * @return true if given point is inside area
	 */
	public boolean contains(double x, double y)
	{
		return (x >= xLow) && (x <= xHigh) && (y >= yLow) && (y <= yHigh);
	}

	/* (non-Javadoc)
	 * @see java.lang.Object#toString()
	 */
	@Override
	public String toString()
	{
		StringBuilder sb = new StringBuilder("Area={");
		sb.append(xLow);
		sb.append(',');
		sb.append(yLow);
		sb.append('-');
		sb.append(xHigh);
		sb.append(',');
		sb.append(yHigh);
		sb.append('}');
		return sb.toString();
	}
}
