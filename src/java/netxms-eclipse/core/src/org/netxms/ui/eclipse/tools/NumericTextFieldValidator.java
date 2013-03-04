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
package org.netxms.ui.eclipse.tools;

import org.netxms.ui.eclipse.console.Messages;


/**
 * Concrete implementation of TextFieldValidator interface
 * for validating numeric fields 
 */
public class NumericTextFieldValidator implements TextFieldValidator
{
	private long min;
	private long max;
	private String range;
	
	/**
	 * @param min minimal allowed value
	 * @param max maximal allowed value
	 */
	public NumericTextFieldValidator(long min, long max)
	{
		this.min = min;
		this.max = max;
		range = Long.toString(min) + Messages.NumericTextFieldValidator_RangeSeparator + Long.toString(max);
	}
	
	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.tools.TextFieldValidator#validate(java.lang.String)
	 */
	@Override
	public boolean validate(String text)
	{
		try
		{
			long value = Long.parseLong(text);
			return ((value >= min) && (value <= max));
		}
		catch(NumberFormatException e)
		{
			return false;
		}
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.tools.TextFieldValidator#getErrorMessage(java.lang.String, java.lang.String)
	 */
	@Override
	public String getErrorMessage(String text, String label)
	{
		return Messages.NumericTextFieldValidator_ErrorMessage_Part1 + range + Messages.NumericTextFieldValidator_ErrorMessage_Part2 + label + Messages.NumericTextFieldValidator_ErrorMessage_Part3;
	}
}
