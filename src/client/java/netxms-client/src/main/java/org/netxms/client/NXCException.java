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
import org.netxms.client.constants.RCC;

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
    * List of identifiers of any related objects (for example, objects referencing entity being deleted).
    */
   protected long[] relatedObjects;

	/**
	 * Create new NXC exception.
	 * 
	 * @param errorCode Error code
	 */
	public NXCException(int errorCode)
	{
      this(errorCode, null, null, null);
	}

	/**
    * Create new NXC exception with additional information.
    * 
	 * @param errorCode Error code
	 * @param additionalInfo Additional info
	 */
	public NXCException(int errorCode, String additionalInfo)
	{
      this(errorCode, additionalInfo, null, null);
	}

   /**
    * Create new NXC exception with additional information.
    * 
    * @param errorCode Error code
    * @param additionalInfo Additional info
    * @param relatedObjects List of related object identifiers
    */
   public NXCException(int errorCode, String additionalInfo, long[] relatedObjects)
   {
      this(errorCode, additionalInfo, relatedObjects, null);
   }

   /**
    * Create new NXC exception with root cause reference.
    * 
    * @param errorCode Error code
    * @param cause root cause exception
    */
   public NXCException(int errorCode, Throwable cause)
   {
      this(errorCode, null, null, cause);
   }

   /**
    * Create new NXC exception with additional information and root cause reference.
    *
    * @param errorCode Error code
    * @param additionalInfo Additional info
    * @param relatedObjects List of related object identifiers
    * @param cause root cause exception
    */
   public NXCException(int errorCode, String additionalInfo, long[] relatedObjects, Throwable cause)
   {
      super(cause);
      this.errorCode = errorCode;
      this.additionalInfo = additionalInfo;
      this.relatedObjects = relatedObjects;
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
	   return RCC.getText(code, lang, additionalInfo);
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

   /**
    * Get additional information associated with exception (for example, NXSL error message)
    * 
    * @return additional information associated with exception
    */
   public String getAdditionalInfo()
   {
      return additionalInfo;
   }

   /**
    * Get list of identifiers of related objects.
    *
    * @return list of identifiers of related objects or null if not provided
    */
   public long[] getRelatedObjects()
   {
      return relatedObjects;
   }

   /**
    * @see java.lang.Throwable#getMessage()
    */
   @Override
   public String getMessage()
   {
      return getErrorMessage(errorCode, "en");
   }

   /**
    * @see java.lang.Throwable#getLocalizedMessage()
    */
   @Override
   public String getLocalizedMessage()
   {
      Locale locale = Locale.getDefault();
      return getErrorMessage(errorCode, locale.getLanguage());
   }
}
