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
package org.netxms.ui.eclipse.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.ui.forms.widgets.FormToolkit;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Composite widget - text with label
 */
public abstract class LabeledControl extends Composite
{
	protected Label label;
	protected Control control;
	protected FormToolkit toolkit;
	
	/**
	 * @param parent
	 * @param style
	 */
	public LabeledControl(Composite parent, int style)
	{
		super(parent, style);
		toolkit = null;
		createContent(getDefaultControlStyle());
	}

	/**
	 * @param parent
	 * @param style
	 * @param textStyle
	 */
	public LabeledControl(Composite parent, int style, int textStyle)
	{
		super(parent, style);
		toolkit = null;
		createContent(textStyle);
	}
	
	/**
	 * @param parent
	 * @param style
	 * @param textStyle
	 * @param toolkit
	 */
	public LabeledControl(Composite parent, int style, int textStyle, FormToolkit toolkit)
	{
		super(parent, style);
		this.toolkit = toolkit;
		createContent(textStyle);
      toolkit.adapt(this);
	}
	
   /**
    * Create enclosed control
    * 
    * @param controlStyle
    * @return
    */
   protected abstract Control createControl(int controlStyle);
   
   /**
    * Get default style for control
    * 
    * @return
    */
   protected int getDefaultControlStyle()
   {
      return SWT.BORDER;
   }
   
   /**
    * Should return true if extra vertical space is needed by control
    * 
    * @param controlStyle style of control being created
    * @return
    */
   protected boolean isExtraVerticalSpaceNeeded(int controlStyle)
   {
      return false;
   }
   
	/**
	 * Do widget creation.
	 * 
	 * @param textStyle
	 */
   private void createContent(int controlStyle)
	{
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.INNER_SPACING;
		layout.marginTop = 0;
		layout.marginBottom = 0;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		setLayout(layout);
		
		label = (toolkit != null) ? toolkit.createLabel(this, "") : new Label(this, SWT.NONE); //$NON-NLS-1$
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		label.setLayoutData(gd);
		
		control = createControl(controlStyle);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		if (isExtraVerticalSpaceNeeded(controlStyle))
		{
			gd.verticalAlignment = SWT.FILL;
			gd.grabExcessVerticalSpace = true;
		}
		else
		{
			gd.verticalAlignment = SWT.TOP;
		}
		control.setLayoutData(gd);
	}
	
	/**
	 * Set label
	 * 
	 * @param newLabel New label
	 */
	public void setLabel(final String newLabel)
	{
		label.setText(newLabel);
	}
	
	/**
	 * Get label
	 * 
	 * @return Current label
	 */
	public String getLabel()
	{
		return label.getText();
	}
	
	/**
	 * Set text in control
	 * 
	 * @param newText
	 */
	public abstract void setText(final String newText);

	/**
	 * Get text from control
	 * 
	 * @return Text
	 */
	public abstract String getText();
	
	/**
	 * Get encapsulated control
	 * 
	 * @return text control
	 */
	public Control getControl()
	{
		return control;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.widgets.Control#setEnabled(boolean)
	 */
	@Override
	public void setEnabled(boolean enabled)
	{
		super.setEnabled(enabled);
		control.setEnabled(enabled);
		label.setEnabled(enabled);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.widgets.Composite#setFocus()
	 */
	@Override
	public boolean setFocus()
	{
		return control.setFocus();
	}

   /* (non-Javadoc)
    * @see org.eclipse.swt.widgets.Control#setBackground(org.eclipse.swt.graphics.Color)
    */
   @Override
   public void setBackground(Color color)
   {
      super.setBackground(color);
      label.setBackground(color);
   }
}
