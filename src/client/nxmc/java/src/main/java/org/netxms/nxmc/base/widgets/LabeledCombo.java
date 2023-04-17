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

import java.util.Collection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;

/**
 * Composite widget - combo with label
 */
public class LabeledCombo extends LabeledControl
{
	/**
    * Create labeled combo with default combo styling (read-only dropdown).
    *
    * @param parent parent composite
    * @param style control style
    */
	public LabeledCombo(Composite parent, int style)
	{
      this(parent, style, SWT.BORDER | SWT.DROP_DOWN | SWT.READ_ONLY);
	}

	/**
    * Create labeled combo with given combo styling
    *
    * @param parent parent composite
    * @param style control style
    * @param comboStyle combo control style
    */
	public LabeledCombo(Composite parent, int style, int comboStyle)
	{
		super(parent, style, comboStyle);
	}

   /**
    * Create labeled combo with given combo styling and width hint
    *
    * @param parent parent composite
    * @param style control style
    * @param comboStyle combo control style
    * @param widthHint width hint
    */
   public LabeledCombo(Composite parent, int style, int comboStyle, int widthHint)
   {
      super(parent, style, comboStyle, widthHint);
   }

   /**
    * @see org.netxms.nxmc.base.widgets.LabeledControl#createControl(int, java.lang.Object)
    */
   @Override
   protected Control createControl(int controlStyle, Object parameters)
   {
      return new Combo(this, controlStyle);
   }

   /**
    * @see org.netxms.nxmc.base.widgets.LabeledControl#setText(java.lang.String)
    */
   @Override
   public void setText(String newText)
   {
      ((Combo)control).setText(newText);
   }

   /**
    * Set text length limit.
    *
    * @param limit max text length
    * @see org.eclipse.swt.widgets.Combo#setTextLimit(int)
    */
   public void setTextLimit(int limit)
   {
      ((Combo)control).setTextLimit(limit);
   }

   /**
    * @see org.netxms.nxmc.base.widgets.LabeledControl#getText()
    */
   @Override
   public String getText()
   {
      return ((Combo)control).getText();
   }

	/**
    * Select element in combo.
    * 
    * @param index selection index
    * @see org.eclipse.swt.widgets.Combo#select(int)
    */
   public void select(int index)
	{
      ((Combo)control).select(index);
	}

	/**
    * Get current selection index.
    * 
    * @return current selection index
    * @see org.eclipse.swt.widgets.Combo#getSelectionIndex()
    */
   public int getSelectionIndex()
	{
      return ((Combo)control).getSelectionIndex();
	}

   /**
    * Set content of the combo (will remove any existing entries).
    *
    * @param elements new set of elements
    */
   public void setContent(String[] elements)
   {
      ((Combo)control).removeAll();
      for(String e : elements)
         ((Combo)control).add(e);
   }
	
   /**
    * Set content of the combo (will remove any existing entries).
    *
    * @param elements new set of elements
    */
   public void setContent(Collection<String> elements)
   {
      ((Combo)control).removeAll();
      for(String e : elements)
         ((Combo)control).add(e);
   }

   /**
    * Add element to combo.
    *
    * @param element element to add
    * @see org.eclipse.swt.widgets.Combo#add(String)
    */
   public void add(String element)
   {
      ((Combo)control).add(element);
   }

   /**
    * Remove all elements from combo
    *
    * @see org.eclipse.swt.widgets.Combo#removeAll()
    */
   public void removeAll()
   {
      ((Combo)control).removeAll();
   }

   /**
    * Add selection listener.
    *
    * @param listener listener to add
    * @see org.eclipse.swt.widgets.Combo#addSelectionListener(SelectionListener)
    */
   public void addSelectionListener(SelectionListener listener)
   {
      ((Combo)control).addSelectionListener(listener);
   }

   /**
    * Remove selection listener.
    *
    * @param listener listener to remove
    * @see org.eclipse.swt.widgets.Combo#removeSelectionListener(SelectionListener)
    */
   public void removeSelectionListener(SelectionListener listener)
   {
      ((Combo)control).removeSelectionListener(listener);
   }

	/**
    * Get combo control
    * 
    * @return combo control
    */
   public Combo getComboControl()
	{
      return (Combo)control;
	}
}
