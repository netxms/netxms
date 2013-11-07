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
package org.netxms.ui.eclipse.console.tools;

import org.netxms.ui.eclipse.console.Messages;
import org.netxms.ui.eclipse.tools.TextFieldValidator;

/**
 * Object name field validator
 */
public class ObjectNameValidator implements TextFieldValidator
{
	private static char[] INVALID_CHARACTERS = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,
	                                             0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	                                             0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
	                                             '|', '"', '\'', '*', '%', '#', '\\', '`', ';', '?', '<', '>', '=' };
	
	private boolean isEmpty = false;
	
	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.tools.TextFieldValidator#validate(java.lang.String)
	 */
	@Override
	public boolean validate(String text)
	{
		isEmpty = text.trim().isEmpty();
		if (isEmpty)
			return false;
		
		for(char c : text.toCharArray())
		{
			for(char tc : INVALID_CHARACTERS)
			{
				if (c == tc)
					return false;
			}
		}
		return true;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.tools.TextFieldValidator#getErrorMessage(java.lang.String, java.lang.String)
	 */
	@Override
	public String getErrorMessage(String text, String label)
	{
		return isEmpty ? 
		      String.format(Messages.ObjectNameValidator_ErrorMessage1, label) : 
		         String.format(Messages.ObjectNameValidator_ErrorMessage2, label);
	}
}
