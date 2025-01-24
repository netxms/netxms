/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2017 Raden Solutions
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
package org.netxms.client.maps.configs;

/**
 * Text box element config
 */
public class TextBoxConfig
{
   private String text;
   private int backgroundColor;
   private int textColor;
   private int borderColor;
   private int fontSize;
   private long drillDownObjectId;
   private boolean borderRequired;
   
   public String getText()
   {
      return text;
   }
   
   public void setText(String text)
   {
      this.text = text;
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
    * @return the textColor
    */
   public int getTextColor()
   {
      return textColor;
   }

   /**
    * @param textColor the textColor to set
    */
   public void setTextColor(int textColor)
   {
      this.textColor = textColor;
   }

   /**
    * @return the borderColor
    */
   public int getBorderColor()
   {
      return borderColor;
   }

   /**
    * @param borderColor the borderColor to set
    */
   public void setBorderColor(int borderColor)
   {
      this.borderColor = borderColor;
   }

   /**
    * @return the borderRequired
    */
   public boolean isBorderRequired()
   {
      return borderRequired;
   }

   /**
    * @param borderRequired the borderRequired to set
    */
   public void setBorderRequired(boolean borderRequired)
   {
      this.borderRequired = borderRequired;
   }
   
   /**
    * @return font size
    */
   public int getFontSize()
   {
      return fontSize;
   }
   
   /**
    * @param fontSize to set
    */
   public void setFontSize(int fontSize)
   {
      this.fontSize = fontSize;
   }
   
   /**
    * @return drill down object id
    */
   public long getDrillDownObjectId()
   {
      return drillDownObjectId;
   }
   
   /**
    * @param drillDownObjectId to set
    */
   public void setDrillDownObjectId(long drillDownObjectId)
   {
      this.drillDownObjectId = drillDownObjectId;
   }
}
