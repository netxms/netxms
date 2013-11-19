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
package org.netxms.ui.eclipse.tools;

import org.netxms.webui.core.Messages;

/**
 * Input validator for IP address entry fields
 */
public class IPAddressValidator implements TextFieldValidator
{
	private static final String IP_ADDRESS_PATTERN = "^([01]?\\d\\d?|2[0-4]\\d|25[0-5])\\.([01]?\\d\\d?|2[0-4]\\d|25[0-5])\\.([01]?\\d\\d?|2[0-4]\\d|25[0-5])\\.([01]?\\d\\d?|2[0-4]\\d|25[0-5])$"; //$NON-NLS-1$
	
	private boolean allowEmpty;
	
	/**
	 * Create new IP address validator.
	 * 
	 * @param allowEmpty if true, empty string is allowed
	 */
	public IPAddressValidator(boolean allowEmpty)
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
		return text.matches(IP_ADDRESS_PATTERN);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.tools.TextFieldValidator#getErrorMessage(java.lang.String, java.lang.String)
	 */
	@Override
	public String getErrorMessage(String text, String label)
	{
		return String.format(Messages.get().IPAddressValidator_ErrorMessage, label);
	}
}
