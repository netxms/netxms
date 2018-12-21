/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.datacollection.widgets.helpers;

import org.simpleframework.xml.Attribute;
import org.simpleframework.xml.Root;
import org.simpleframework.xml.Text;

/**
 * Event in log parser rule
 */
@Root(name="match", strict=false)
public class LogParserMatch
{
	@Text
	private String match = ".*"; //$NON-NLS-1$

	@Attribute(required=false)
	private String invert = null;

   @Attribute(required=false)
   private Integer repeatCount = null;

   @Attribute(required=false)
   private Integer repeatInterval = null;

   @Attribute(required=false)
   private String reset = null;
	
	/**
	 * Protected constructor for XML parser
	 */
	protected LogParserMatch()
	{
	}
	
	/**
	 * @param event
	 * @param parameterCount
	 */
	public LogParserMatch(String match, boolean invert, Integer repeatCount, Integer repeatInterval, boolean reset)
	{
		this.match = match;
		setInvert(invert);
		this.repeatCount = repeatCount;
		this.repeatInterval = repeatInterval;
		setReset(reset);
	}

   /**
    * @return the repeatCount
    */
   public Integer getRepeatCount()
   {
      return repeatCount == null ? 0 : repeatCount;
   }

   /**
    * @param repeatCount the repeatCount to set
    */
   public void setRepeatCount(Integer repeatCount)
   {
      this.repeatCount = repeatCount;
   }

   /**
    * @return the repeatInterval
    */
   public Integer getRepeatInterval()
   {
      return repeatInterval == null ? 0 : repeatInterval;
   }

   /**
    * @param repeatInterval the repeatInterval to set
    */
   public void setRepeatInterval(Integer repeatInterval)
   {
      this.repeatInterval = repeatInterval;
   }

   /**
    * @return the reset
    */
   public boolean getReset()
   {
      return reset == null ? true : LogParser.stringToBoolean(reset);
   }

   /**
    * @param reset the reset to set
    */
   public void setReset(boolean reset)
   {
      this.reset = reset ? null : "false";
   }

   /**
    * @return the match
    */
   public String getMatch()
   {
      return match;
   }

   /**
    * @param match the match to set
    */
   public void setMatch(String match)
   {
      this.match = match;
   }

   /**
    * @return the invert
    */
   public boolean getInvert()
   {
      return LogParser.stringToBoolean(invert);
   }

   /**
    * @param invert the invert to set
    */
   public void setInvert(boolean invert)
   {
      this.invert = LogParser.booleanToString(invert);
   }
   
   public int getTimeRagne()
   {
      if(repeatInterval == null)
         return 0;
      int interval = repeatInterval;
      if((interval % 60) == 0)
      {
         interval = interval/60;
         if((interval % 60) == 0)
         {
            interval = interval/60;
         }
      }
      return interval;
   }
   
   public int getTimeUnit()
   {
      if (repeatInterval == null)
         return 0;
      int unit = 0;
      if((repeatInterval % 60) == 0)
      {
         unit++;
         if((repeatInterval % 3600) == 0)
         {
            unit++;
         }
      }      
      return unit;
   }   
}
