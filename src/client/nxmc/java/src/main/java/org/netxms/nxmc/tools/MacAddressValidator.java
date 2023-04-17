/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
package org.netxms.nxmc.tools;

import org.netxms.base.MacAddress;
import org.netxms.base.MacAddressFormatException;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Input validator for MAC address entry fields
 */
public class MacAddressValidator implements TextFieldValidator
{
   private I18n i18n = LocalizationHelper.getI18n(MacAddressValidator.class);
	private boolean allowEmpty;
	
	/**
	 * Create new MAC address validator.
	 * 
	 * @param allowEmpty if true, empty string is allowed
	 */
	public MacAddressValidator(boolean allowEmpty)
	{
		this.allowEmpty = allowEmpty;
	}

   /**
    * @see org.netxms.ui.eclipse.tools.TextFieldValidator#validate(java.lang.String)
    */
	@Override
	public boolean validate(String text)
	{
		if (allowEmpty && text.trim().isEmpty())
			return true;
		
		try
		{
			MacAddress.parseMacAddress(text);
			return true;
		}
		catch(MacAddressFormatException e)
		{
			return false;
		}
	}

   /**
    * @see org.netxms.nxmc.tools.TextFieldValidator#getErrorMessage(java.lang.String)
    */
	@Override
   public String getErrorMessage(String text)
	{
      return i18n.tr("Invalid MAC address");
	}
}
