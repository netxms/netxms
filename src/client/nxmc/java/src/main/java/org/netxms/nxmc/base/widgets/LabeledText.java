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
package org.netxms.nxmc.base.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Text;

/**
 * Composite widget - text with label
 */
public class LabeledText extends LabeledControl
{
	/**
	 * @param parent
	 * @param style
	 */
	public LabeledText(Composite parent, int style)
	{
		super(parent, style);
	}

	/**
	 * @param parent
	 * @param style
	 * @param textStyle
	 */
	public LabeledText(Composite parent, int style, int textStyle)
	{
		super(parent, style, textStyle);
	}

   /**
    * @param parent
    * @param style
    * @param textStyle
    * @param widthHint
    */
   public LabeledText(Composite parent, int style, int textStyle, int widthHint)
   {
      super(parent, style, textStyle, widthHint);
   }

   /**
    * @see org.netxms.nxmc.base.widgets.LabeledControl#createControl(int, java.lang.Object)
    */
   @Override
   protected Control createControl(int controlStyle, Object parameters)
   {
      return new Text(this, controlStyle);
   }

   /**
    * @see org.netxms.nxmc.base.widgets.LabeledControl#getDefaultControlStyle()
    */
   @Override
   protected int getDefaultControlStyle()
   {
      return SWT.BORDER | SWT.SINGLE;
   }

   /**
    * @see org.netxms.nxmc.base.widgets.LabeledControl#isExtraVerticalSpaceNeeded(int)
    */
   @Override
   protected boolean isExtraVerticalSpaceNeeded(int controlStyle)
   {
      return (controlStyle & SWT.MULTI) != 0;
   }

   /**
    * Sets the editable state.
    * 
    * @param editable the new editable state
    * @see org.eclipse.swt.widgets.Text#setEditable(boolean)
    */
   public void setEditable(boolean editable)
   {
      ((Text)control).setEditable(editable);
   }
   
   /**
    * Returns the editable state.
    * 
    * @return whether or not the receiver is editable
    * @see org.eclipse.swt.widgets.Text#getEditable()
    */
   public boolean isEditable()
   {
      return ((Text)control).getEditable();
   }

   /**
    * @see org.netxms.nxmc.base.widgets.LabeledControl#setText(java.lang.String)
    */
   @Override
	public void setText(final String newText)
	{
      ((Text)control).setText((newText != null) ? newText : ""); //$NON-NLS-1$
	}

   /**
    * @see org.netxms.nxmc.base.widgets.LabeledControl#getText()
    */
   @Override
	public String getText()
	{
		return ((Text)control).getText();
	}

   /**
    * Set text length limit.
    *
    * @param limit max text length
    * @see org.eclipse.swt.widgets.Text#setTextLimit(int)
    */
   public void setTextLimit(int limit)
   {
      ((Text)control).setTextLimit(limit);
   }

	/**
	 * Get text control
	 * 
	 * @return text control
	 */
	public Text getTextControl()
	{
		return (Text)control;
	}
}
