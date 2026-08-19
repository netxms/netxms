/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Victor Kirhenshtein
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

import org.netxms.base.Duration;
import org.netxms.base.DurationFormatException;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Implementation of TextFieldValidator interface for validating duration fields (like "2h 30m")
 */
public class DurationValidator implements TextFieldValidator
{
   private final I18n i18n = LocalizationHelper.getI18n(DurationValidator.class);

   private int min;
   private int max;

   /**
    * @param min minimal allowed value in seconds
    * @param max maximal allowed value in seconds
    */
   public DurationValidator(int min, int max)
   {
      this.min = min;
      this.max = max;
   }

   /**
    * @see org.netxms.nxmc.tools.TextFieldValidator#validate(java.lang.String)
    */
   @Override
   public boolean validate(String text)
   {
      try
      {
         long value = Duration.parse(text);
         return (value >= min) && (value <= max);
      }
      catch(DurationFormatException e)
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
      if (Duration.parse(text, -1) == -1)
         return i18n.tr("Must be a duration (for example 90, 5m, or 2h 30m)");
      return i18n.tr("Must be a duration in range {0}..{1}", Duration.format(min), Duration.format(max));
   }
}
