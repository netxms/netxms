/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.charts.api;

/**
 * Dial chart interface
 */
public interface DialChart extends DataComparisonChart
{
	/**
	 * @return the minValue
	 */
	public double getMinValue();

	/**
	 * @param minValue the minValue to set
	 */
	public void setMinValue(double minValue);

	/**
	 * @return the maxValue
	 */
	public double getMaxValue();

	/**
	 * @param maxValue the maxValue to set
	 */
	public void setMaxValue(double maxValue);

	/**
	 * @return the leftRedZone
	 */
	public double getLeftRedZone();

	/**
	 * @param leftRedZone the leftRedZone to set
	 */
	public void setLeftRedZone(double leftRedZone);

	/**
	 * @return the leftYelowZone
	 */
	public double getLeftYellowZone();

	/**
	 * @param leftYelowZone the leftYelowZone to set
	 */
	public void setLeftYellowZone(double leftYellowZone);

	/**
	 * @return the rightYellowZone
	 */
	public double getRightYellowZone();

	/**
	 * @param rightYellowZone the rightYellowZone to set
	 */
	public void setRightYellowZone(double rightYellowZone);

	/**
	 * @return the rightRedZone
	 */
	public double getRightRedZone();

	/**
	 * @param rightRedZone the rightRedZone to set
	 */
	public void setRightRedZone(double rightRedZone);

	/**
	 * @return the legendInside
	 */
	public boolean isLegendInside();

	/**
	 * @param legendInside the legendInside to set
	 */
	public void setLegendInside(boolean legendInside);

	/**
	 * @return the vertical
	 */
	public boolean isVertical();

	/**
	 * @param vertical the vertical to set
	 */
	public void setVertical(boolean vertical);
}
