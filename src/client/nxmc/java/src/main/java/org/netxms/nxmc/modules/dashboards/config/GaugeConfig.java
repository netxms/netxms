/**
 * NetXMS - open source network management system
 * Copyright (C) 2016-2023 RadenSolutions
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
package org.netxms.nxmc.modules.dashboards.config;

import org.netxms.nxmc.modules.charts.api.GaugeColorMode;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.Root;

/**
 * Configuration for "dial chart" dashboard element
 */
@Root(name = "element", strict = false)
public class GaugeConfig extends AbstractChartConfig
{
	// gauge types
	public static final int DIAL = 0;
	public static final int BAR = 1;
	public static final int TEXT = 2;
   public static final int CIRCULAR = 3;

	@Element(required = false)
	private int gaugeType = DIAL;

	@Element(required = false)
	private boolean legendInside = false;

	@Element(required = false)
	private boolean vertical = false;

	@Element(required = false)
	private double minValue = 0.0;

	@Element(required = false)
	private double maxValue = 100.0;

	@Element(required = false)
	private double leftYellowZone = 0.0;

	@Element(required = false)
	private double leftRedZone = 0.0;

	@Element(required = false)
	private double rightYellowZone = 70.0;

	@Element(required = false)
	private double rightRedZone = 90.0;

	@Element(required = false)
   private String fontName = "";

   @Element(required = false)
   private int fontSize = 0;

   @Element(required = false)
   private int expectedTextWidth = 0;

   @Element(required = false)
	private boolean elementBordersVisible = false;

   @Element(required = false)
   private int colorMode = GaugeColorMode.ZONE.getValue();

   @Element(required = false)
   private int customColor = 0;

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
	 * @return the yellowZone
	 */
	public double getRightYellowZone()
	{
		return rightYellowZone;
	}

	/**
	 * @param yellowZone the yellowZone to set
	 */
	public void setRightYellowZone(double yellowZone)
	{
		this.rightYellowZone = yellowZone;
	}

	/**
	 * @return the redZone
	 */
	public double getRightRedZone()
	{
		return rightRedZone;
	}

	/**
	 * @param redZone the redZone to set
	 */
	public void setRightRedZone(double redZone)
	{
		this.rightRedZone = redZone;
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
	 * @return the vertical
	 */
	public boolean isVertical()
	{
		return vertical;
	}

	/**
	 * @param vertical the vertical to set
	 */
	public void setVertical(boolean vertical)
	{
		this.vertical = vertical;
	}

	/**
	 * @return
	 */
	public int getGaugeType()
	{
		return gaugeType;
	}

	/**
	 * @param gaugeType
	 */
	public void setGaugeType(int gaugeType)
	{
		this.gaugeType = gaugeType;
	}

	/**
	 * @return
	 */
	public String getFontName()
	{
		return (fontName != null) ? fontName : ""; //$NON-NLS-1$
	}

	/**
	 * @param fontName
	 */
	public void setFontName(String fontName)
	{
		this.fontName = fontName;
	}

   /**
    * @return the fontSize
    */
   public int getFontSize()
   {
      return fontSize;
   }

   /**
    * @param fontSize the fontSize to set
    */
   public void setFontSize(int fontSize)
   {
      this.fontSize = fontSize;
   }

   /**
    * @return the expectedTextWidth
    */
   public int getExpectedTextWidth()
   {
      return expectedTextWidth;
   }

   /**
    * @param expectedTextWidth the expectedTextWidth to set
    */
   public void setExpectedTextWidth(int expectedTextWidth)
   {
      this.expectedTextWidth = expectedTextWidth;
   }

   /**
    * @return the elementBordersVisible
    */
   public boolean isElementBordersVisible()
   {
      return elementBordersVisible;
   }

   /**
    * @param elementBordersVisible the elementBordersVisible to set
    */
   public void setElementBordersVisible(boolean elementBordersVisible)
   {
      this.elementBordersVisible = elementBordersVisible;
   }

   /**
    * @return the colorMode
    */
   public int getColorMode()
   {
      return colorMode;
   }

   /**
    * @param colorMode the colorMode to set
    */
   public void setColorMode(int colorMode)
   {
      this.colorMode = colorMode;
   }

   /**
    * @return the customColor
    */
   public int getCustomColor()
   {
      return customColor;
   }

   /**
    * @param customColor the customColor to set
    */
   public void setCustomColor(int customColor)
   {
      this.customColor = customColor;
   }
}
