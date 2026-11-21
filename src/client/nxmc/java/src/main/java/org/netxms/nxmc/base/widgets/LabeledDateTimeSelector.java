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

import java.util.Date;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;

/**
 * Composite widget - date/time selector with label
 */
public class LabeledDateTimeSelector extends LabeledControl
{
   /**
    * @param parent parent composite
    * @param style widget style
    */
   public LabeledDateTimeSelector(Composite parent, int style)
   {
      super(parent, style);
   }

   /**
    * @see org.netxms.nxmc.base.widgets.LabeledControl#getDefaultControlStyle()
    */
   @Override
   protected int getDefaultControlStyle()
   {
      return SWT.NONE;
   }

   /**
    * @see org.netxms.nxmc.base.widgets.LabeledControl#createControl(int, java.lang.Object)
    */
   @Override
   protected Control createControl(int controlStyle, Object parameters)
   {
      DateTimeSelector selector = new DateTimeSelector(this, controlStyle);
      selector.setValue(new Date());
      return selector;
   }

   /**
    * Set current value.
    *
    * @param date new date value
    */
   public void setValue(Date date)
   {
      ((DateTimeSelector)control).setValue(date);
   }

   /**
    * Get currently selected date and time.
    * 
    * @return currently selected date and time
    */
   public Date getValue()
   {
      return ((DateTimeSelector)control).getValue();
   }

   /**
    * Set text as unix timestamp in seconds.
    *
    * @see org.netxms.nxmc.base.widgets.LabeledControl#setText(java.lang.String)
    */
   @Override
   public void setText(String newText)
   {
      try
      {
         long timestamp = Long.parseLong(newText);
         ((DateTimeSelector)control).setValue(new Date(timestamp * 1000));
      }
      catch(NumberFormatException e)
      {
      }
   }

   /**
    * Get text as unix timestamp in seconds.
    *
    * @see org.netxms.nxmc.base.widgets.LabeledControl#getText()
    */
   @Override
   public String getText()
   {
      return Long.toString(((DateTimeSelector)control).getValue().getTime() / 1000);
   }
}
