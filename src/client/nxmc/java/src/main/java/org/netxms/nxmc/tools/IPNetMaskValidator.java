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

import java.net.Inet4Address;
import java.net.InetAddress;
import java.net.UnknownHostException;
import org.netxms.base.InetAddressEx;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Input validator for IP network mask entry fields
 */
public class IPNetMaskValidator implements TextFieldValidator
{
   private static final String IP_ADDRESS_PATTERN = "^([0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}|[A-Fa-f0-9:]+)$";

   private I18n i18n = LocalizationHelper.getI18n(IPNetMaskValidator.class);
   private boolean allowEmpty;
   private int maxBits;

	/**
	 * Create new IP network mask validator.
	 * 
	 * @param allowEmpty if true, empty string is allowed
	 */
   public IPNetMaskValidator(boolean allowEmpty, String ipAddress)
	{
		this.allowEmpty = allowEmpty;
      if (ipAddress.isEmpty())
      {
         maxBits = 32;
      }
      else
      {
         try
         {
            maxBits = (InetAddress.getByName(ipAddress) instanceof Inet4Address) ? 32 : 128;
         }
         catch(UnknownHostException e)
         {
            maxBits = 32;
         }
      }
	}

   /**
    * @see org.netxms.ui.eclipse.tools.TextFieldValidator#validate(java.lang.String)
    */
	@Override
	public boolean validate(String text)
	{
		if (allowEmpty && text.trim().isEmpty())
			return true;

		if (!text.matches(IP_ADDRESS_PATTERN))
			return false;

		if (text.length() <= 2)
		{
	      try
	      {
	         int bits = Integer.parseInt(text);
            return ((bits >= 0) && (bits < maxBits));
	      }
	      catch(NumberFormatException e)
	      {
	      }
		}		   

		try
		{
         InetAddress mask = InetAddress.getByName(text);
			byte[] bytes = mask.getAddress();
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
         int bits = InetAddressEx.bitsInMask(mask);
         return ((bits >= 0) && (bits < maxBits));
		}
		catch(UnknownHostException e)
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
      return i18n.tr("Invalid network mask");
	}
}
