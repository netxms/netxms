/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
package org.netxms.client;

import java.util.Locale;
import java.util.MissingResourceException;
import java.util.PropertyResourceBundle;
import java.util.ResourceBundle;

/**
 * NetXMS client library exception. Used to report API call errors.
 */
public class NXCException extends Exception
{
   private static final long serialVersionUID = 1453981595988661915L;

   /**
    * Application-specific error code
    */
   protected int errorCode;
   
   /**
    * Additional information about this error
    */
   protected String additionalInfo;

   /**
	 * @param errorCode
	 */
	public NXCException(int errorCode)
	{
      super();
      this.errorCode = errorCode;
      this.additionalInfo = null;
	}

	/**
	 * @param errorCode
	 * @param additionalInfo
	 */
	public NXCException(int errorCode, String additionalInfo)
	{
      super();
      this.errorCode = errorCode;
      this.additionalInfo = additionalInfo;
	}

   /**
    * Get error message text for given error code. Must not return null.
    * 
    * @param code error code
    * @param lang language code
    * @return error message for given code
    */
	protected String getErrorMessage(int code, String lang)
	{
	   try
	   {
   	   ResourceBundle bundle = PropertyResourceBundle.getBundle("messages", new Locale(lang));
   	   try
   	   {
   	      return String.format(bundle.getString(String.format("RCC_%04d", code)), additionalInfo);
   	   }
   	   catch(MissingResourceException e)
   	   {
   	      return String.format(bundle.getString("RCC_UNKNOWN"), code);
   	   }
	   }
	   catch(Exception e)
	   {
         return "Error " + Integer.toString(code);
	   }
	}

   /**
    * Get exception's error code.
    * 
    * @return the errorCode
    */
   public int getErrorCode()
   {
      return errorCode;
   }

   /* (non-Javadoc)
    * @see java.lang.Throwable#getMessage()
    */
   @Override
   public String getMessage()
   {
      return getErrorMessage(errorCode, "en");
   }

   /* (non-Javadoc)
    * @see java.lang.Throwable#getLocalizedMessage()
    */
   @Override
   public String getLocalizedMessage()
   {
      Locale locale = Locale.getDefault();
      return getErrorMessage(errorCode, locale.getLanguage());
   }
}
