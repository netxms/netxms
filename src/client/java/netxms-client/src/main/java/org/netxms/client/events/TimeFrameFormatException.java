/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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
package org.netxms.client.events;

import java.util.Locale;
import org.netxms.client.ClientLocalizationHelper;

/**
 * Exception in case of time frame data parsing
 */
public class TimeFrameFormatException extends Exception
{
   private static final long serialVersionUID = 5935877340735559718L;

   public static final int TIME_VALIDATION_FAILURE = 1;
   public static final int TIME_INCORRECT_ORDER = 2;
   public static final int DAY_OUT_OF_RANGE = 3;
   public static final int DAY_NOT_A_NUMBER = 4;

   /**
    * Specific error code
    */
   protected int errorCode;

   /**
    * Translation key
    */
   private String key;

   /**
    * Exception constructor
    * 
    * @param errorCode one of error codes
    */
   public TimeFrameFormatException(int errorCode)
   {
      this.errorCode = errorCode;
      key = String.format("TimeFrameFormatException_%d", errorCode);
   }

   /**
    * @return the errorCode
    */
   public int getErrorCode()
   {
      return errorCode;
   }

   /**
    * @see java.lang.Throwable#getMessage()
    */
   @Override
   public String getMessage()
   {
      return ClientLocalizationHelper.getText(key, "en");
   }

   /**
    * @see java.lang.Throwable#getLocalizedMessage()
    */
   @Override
   public String getLocalizedMessage()
   {
      return ClientLocalizationHelper.getText(key, Locale.getDefault());
   }
}
