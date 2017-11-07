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
package org.netxms.client.maps.elements;

import org.netxms.base.NXCPMessage;
import org.netxms.client.maps.configs.TextBoxConfig;

/**
 * Network map element representing a text box
 */
public class NetworkMapTextBox extends NetworkMapElement
{
   private String TextBoxXml = "";
   private String text;
   private int backgroundColor;
   private int textColor;
   private int borderColor;
   private int fontSize;
   private long drillDownObjectId;
   private boolean borderRequired;
   
   /**
    * Create new text box from NXCPMessgae
    * 
    * @param msg
    * @param baseId
    */
   protected NetworkMapTextBox(NXCPMessage msg, long baseId)
   {
      super(msg, baseId);
      TextBoxXml = msg.getFieldAsString(baseId+10);
      try
      {
         TextBoxConfig conf = TextBoxConfig.createFromXml(TextBoxXml);
         text = conf.getText();
         backgroundColor = conf.getBackgroundColor();
         textColor = conf.getTextColor();
         borderColor = conf.getBorderColor();
         borderRequired = conf.isBorderRequired();
         fontSize = conf.getFontSize();
         drillDownObjectId = conf.getDrillDownObjectId();
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
      type = MAP_ELEMENT_TEXT_BOX;
   }
   
   /* (non-Javadoc)
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
      try
      {
         TextBoxXml = config.createXml();
      }
      catch (Exception e)
      {
         TextBoxXml = "";
      }
      msg.setField(baseId + 10, TextBoxXml);
   }

   /**
    * @return text
    */
   public String getText()
   {
      return text;
   }
   
   /**
    * Set text
    * 
    * @param text to set
    */
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
