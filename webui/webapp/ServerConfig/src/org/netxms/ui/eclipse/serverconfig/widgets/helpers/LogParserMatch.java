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
package org.netxms.ui.eclipse.serverconfig.widgets.helpers;

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
	public LogParserMatch(String match, boolean invert)
	{
		this.match = match;
		setInvert(invert);
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
}
