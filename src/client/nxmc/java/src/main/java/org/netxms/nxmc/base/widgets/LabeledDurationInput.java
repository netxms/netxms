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
package org.netxms.nxmc.base.widgets;

import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.widgets.Composite;
import org.netxms.base.Duration;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.DurationValidator;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Text input field with label for entering durations in human readable form (for example "2h 30m"). Value is always exchanged
 * with the calling code as number of seconds.
 */
public class LabeledDurationInput extends LabeledText
{
   private final I18n i18n = LocalizationHelper.getI18n(LabeledDurationInput.class);

   private int min = 0;
   private int max = Integer.MAX_VALUE;

   /**
    * @param parent parent composite
    * @param style widget style
    */
   public LabeledDurationInput(Composite parent, int style)
   {
      super(parent, style);
      getTextControl().setToolTipText(i18n.tr("Duration in seconds or with unit suffix: s, m, h, d, w (for example 90, 5m, 2h 30m)"));
   }

   /**
    * Set allowed value range.
    *
    * @param min minimal allowed value in seconds
    * @param max maximal allowed value in seconds
    */
   public void setRange(int min, int max)
   {
      this.min = min;
      this.max = max;
   }

   /**
    * Set value.
    *
    * @param seconds value in seconds
    */
   public void setValue(int seconds)
   {
      setText(Duration.format(seconds));
   }

   /**
    * Get value. If current content of the field is not a valid duration, minimal allowed value will be returned, and values
    * above the allowed range are capped at maximum. Call <code>validate()</code> before reading the value to check user input.
    *
    * @return value in seconds
    */
   public int getValue()
   {
      return (int)Math.min(Duration.parse(getText(), min), max);
   }

   /**
    * Validate user input. Will update control error message accordingly.
    *
    * @return true if input is valid
    */
   public boolean validate()
   {
      return WidgetHelper.validateTextInput(this, new DurationValidator(min, max));
   }

   /**
    * Add modify listener to underlying text control.
    *
    * @param listener listener to add
    */
   public void addModifyListener(ModifyListener listener)
   {
      getTextControl().addModifyListener(listener);
   }
}
