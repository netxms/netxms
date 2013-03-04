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

import java.net.InetAddress;
import java.net.UnknownHostException;
import org.netxms.ui.eclipse.console.Messages;
import org.netxms.ui.eclipse.tools.TextFieldValidator;

/**
 * Input validator for IP network mask entry fields
 */
public class IPNetMaskValidator implements TextFieldValidator
{
	private static final String IP_ADDRESS_PATTERN = "^([01]?\\d\\d?|2[0-4]\\d|25[0-5])\\.([01]?\\d\\d?|2[0-4]\\d|25[0-5])\\.([01]?\\d\\d?|2[0-4]\\d|25[0-5])\\.([01]?\\d\\d?|2[0-4]\\d|25[0-5])$"; //$NON-NLS-1$
	
	private boolean allowEmpty;
	
	/**
	 * Create new IP network mask validator.
	 * 
	 * @param allowEmpty if true, empty string is allowed
	 */
	public IPNetMaskValidator(boolean allowEmpty)
	{
		this.allowEmpty = allowEmpty;
	}
	
	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.tools.TextFieldValidator#validate(java.lang.String)
	 */
	@Override
	public boolean validate(String text)
	{
		if (allowEmpty && text.trim().isEmpty())
			return true;
		
		if (!text.matches(IP_ADDRESS_PATTERN))
			return false;
		
		try
		{
			byte[] bytes = InetAddress.getByName(text).getAddress();
			for(int i = 0, state = 0; i < bytes.length; i++)
			{
				if (bytes[i] == (byte)0xFF)
					continue;
				if ((state != 0) && (bytes[i] != 0))
					return false;
				if ((bytes[i] != 0) && (bytes[i] != (byte)0x80) && (bytes[i] != (byte)0xC0) && (bytes[i] != (byte)0xE0) && (bytes[i] != (byte)0xF0) && (bytes[i] != (byte)0xF8) && (bytes[i] != (byte)0xFC) && (bytes[i] != (byte)0xFE))
					return false;
				state = 1;
			}
			return true;
		}
		catch(UnknownHostException e)
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
		return Messages.IPNetMaskValidator_ErrorPrefix + label + Messages.IPNetMaskValidator_ErrorSuffix;
	}
}
