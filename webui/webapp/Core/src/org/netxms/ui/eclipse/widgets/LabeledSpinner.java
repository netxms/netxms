/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2015 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.widgets;

import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Spinner;
import org.eclipse.ui.forms.widgets.FormToolkit;

/**
 * Composite widget - spinner with label
 */
public class LabeledSpinner extends LabeledControl
{
	/**
	 * @param parent
	 * @param style
	 */
	public LabeledSpinner(Composite parent, int style)
	{
		super(parent, style);
	}

	/**
	 * @param parent
	 * @param style
	 * @param spinnerStyle
	 */
	public LabeledSpinner(Composite parent, int style, int spinnerStyle)
	{
		super(parent, style, spinnerStyle);
	}
	
	/**
	 * @param parent
	 * @param style
	 * @param spinnerStyle
	 * @param toolkit
	 */
	public LabeledSpinner(Composite parent, int style, int spinnerStyle, FormToolkit toolkit)
	{
		super(parent, style, spinnerStyle, toolkit);
	}
	
	/* (non-Javadoc)
    * @see org.netxms.ui.eclipse.widgets.LabeledControl#createControl(int)
	 */
   @Override
   protected Control createControl(int controlStyle)
	{
      return new Spinner(this, controlStyle);
	}
	
	/* (non-Javadoc)
    * @see org.netxms.ui.eclipse.widgets.LabeledControl#setText(java.lang.String)
	 */
   @Override
   public void setText(String newText)
   {
      try
      {
         ((Spinner)control).setSelection(Integer.parseInt(newText));
      }
      catch(NumberFormatException e)
	{
      }
	}
	
   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.widgets.LabeledControl#getText()
	 */
   @Override
   public String getText()
	{
      return ((Spinner)control).getText();
	}
	
	/**
	 * Set spinner range
	 * 
	 * @param minValue
	 * @param maxValue
	 */
	public void setRange(int minValue, int maxValue)
	{
      ((Spinner)control).setMinimum(minValue);
      ((Spinner)control).setMaximum(maxValue);
	}
	
	/**
	 * Set selection.
	 * 
	 * @param value new value
	 */
	public void setSelection(int value)
	{
	   ((Spinner)control).setSelection(value);
	}
	
	/**
	 * Get current selection
	 * 
	 * @return
	 */
	public int getSelection()
	{
	   return ((Spinner)control).getSelection();
	}
	
	/**
	 * Get spinner control
	 * 
	 * @return spinner control
	 */
	public Spinner getSpinnerControl()
	{
		return (Spinner)control;
   }
}
