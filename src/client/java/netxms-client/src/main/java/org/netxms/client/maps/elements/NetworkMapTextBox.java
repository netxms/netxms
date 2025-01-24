/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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
package org.netxms.client.maps.elements;

import org.netxms.base.NXCPMessage;
import org.netxms.client.maps.configs.TextBoxConfig;
import com.google.gson.Gson;

/**
 * Network map element representing a text box
 */
public class NetworkMapTextBox extends NetworkMapElement
{
   private String text;
   private int backgroundColor;
   private int textColor;
   private int borderColor;
   private int fontSize;
   private long drillDownObjectId;
   private boolean borderRequired;

   /**
    * Create new text box from NXCP messgae
    * 
    * @param msg NXCP message
    * @param baseId base field ID
    */
   protected NetworkMapTextBox(NXCPMessage msg, long baseId)
   {
      super(msg, baseId);
      String json = msg.getFieldAsString(baseId + 10);
      try
      {
         TextBoxConfig config = new Gson().fromJson(json, TextBoxConfig.class);
         text = config.getText();
         backgroundColor = config.getBackgroundColor();
         textColor = config.getTextColor();
         borderColor = config.getBorderColor();
         borderRequired = config.isBorderRequired();
         fontSize = config.getFontSize();
         drillDownObjectId = config.getDrillDownObjectId();
      }
      catch(Exception e)
      {
         text = "";
         backgroundColor = Integer.MAX_VALUE;//white
         textColor = 0;//black
         borderColor = 0;//black
         borderRequired = true;
         fontSize = 10;
         drillDownObjectId = 0;
      }
      type = MAP_ELEMENT_TEXT_BOX;
   }

   /**
    * Create new text box element
    * 
    * @param id element ID
    */
   public NetworkMapTextBox(long id)
   {
      super(id);
      text = "";
      backgroundColor = Integer.MAX_VALUE;//white
      textColor = 0;//black
      borderColor = 0;//black
      borderRequired = true;
      fontSize = 10;
      type = MAP_ELEMENT_TEXT_BOX;
   }

   /**
    * @see org.netxms.client.maps.elements.NetworkMapElement#fillMessage(org.netxms.base.NXCPMessage, long)
    */
   @Override
   public void fillMessage(NXCPMessage msg, long baseId)
   {
      super.fillMessage(msg, baseId);
      TextBoxConfig config = new TextBoxConfig();
      config.setText(text);
      config.setBackgroundColor(backgroundColor);
      config.setTextColor(textColor);
      config.setBorderColor(borderColor);
      config.setBorderRequired(borderRequired);
      config.setFontSize(fontSize);
      config.setDrillDownObjectId(drillDownObjectId);
      msg.setFieldJson(baseId + 10, config);
   }

   /**
    * Get text.
    *
    * @return text
    */
   public String getText()
   {
      return text;
   }
   
   /**
    * Set text.
    * 
    * @param text new text
    */
   public void setText(String text)
   {
      this.text = text;
   }
   
   /**
    * Get background color.
    *
    * @return background color in BGR format
    */
   public int getBackgroundColor()
   {
      return backgroundColor;
   }

   /**
    * Set background color.
    *
    * @param backgroundColor new background color in BGR format
    */
   public void setBackgroundColor(int backgroundColor)
   {
      this.backgroundColor = backgroundColor;
   }

   /**
    * Get text color.
    *
    * @return text color in BGR format
    */
   public int getTextColor()
   {
      return textColor;
   }

   /**
    * Set text color.
    *
    * @param textColor new text color in BGR format
    */
   public void setTextColor(int textColor)
   {
      this.textColor = textColor;
   }

   /**
    * Get border color.
    *
    * @return border color in BGR format
    */
   public int getBorderColor()
   {
      return borderColor;
   }

   /**
    * Set border color.
    *
    * @param borderColor new border color in BGR format
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
