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
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Text;
import org.eclipse.ui.forms.widgets.FormToolkit;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Composite widget - text with label
 */
public class LabeledText extends Composite
{
	private Label label;
	private Text text;
	private FormToolkit toolkit;
	
	/**
	 * @param parent
	 * @param style
	 */
	public LabeledText(Composite parent, int style)
	{
		super(parent, style | SWT.NO_FOCUS);  // SWT.NO_FOCUS is a workaround for Eclipse/RAP bug 321274
		toolkit = null;
		createContent(SWT.SINGLE | SWT.BORDER);
	}

	/**
	 * @param parent
	 * @param style
	 * @param textStyle
	 */
	public LabeledText(Composite parent, int style, int textStyle)
	{
		super(parent, style | SWT.NO_FOCUS);  // SWT.NO_FOCUS is a workaround for Eclipse/RAP bug 321274
		toolkit = null;
		createContent(textStyle);
	}
	
	/**
	 * @param parent
	 * @param style
	 * @param textStyle
	 * @param toolkit
	 */
	public LabeledText(Composite parent, int style, int textStyle, FormToolkit toolkit)
	{
		super(parent, style | SWT.NO_FOCUS);  // SWT.NO_FOCUS is a workaround for Eclipse/RAP bug 321274
		this.toolkit = toolkit;
		createContent(textStyle);
      toolkit.adapt(this);
	}
	
	/**
	 * Do widget creation.
	 * 
	 * @param textStyle
	 */
	private void createContent(int textStyle)
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
		
		text = (toolkit != null) ? toolkit.createText(this, "", textStyle) : new Text(this, textStyle); //$NON-NLS-1$
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		if ((textStyle & SWT.MULTI) != 0)
		{
			gd.verticalAlignment = SWT.FILL;
			gd.grabExcessVerticalSpace = true;
		}
		else
		{
			gd.verticalAlignment = SWT.TOP;
		}
		text.setLayoutData(gd);
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
	 * Set text
	 * 
	 * @param newText
	 */
	public void setText(final String newText)
	{
		text.setText((newText != null) ? newText : ""); //$NON-NLS-1$
	}

	/**
	 * Get text
	 * 
	 * @return Text
	 */
	public String getText()
	{
		return text.getText();
	}
	
	/**
	 * Get text control
	 * 
	 * @return text control
	 */
	public Text getTextControl()
	{
		return text;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.widgets.Control#setEnabled(boolean)
	 */
	@Override
	public void setEnabled(boolean enabled)
	{
		super.setEnabled(enabled);
		text.setEnabled(enabled);
		label.setEnabled(enabled);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.widgets.Composite#setFocus()
	 */
	@Override
	public boolean setFocus()
	{
		return text.setFocus();
	}
	
	/**
    * Sets the editable state.
    * 
    * @param editable the new editable state
    */
	public void setEditable(boolean editable)
	{
		text.setEditable(editable);
	}
   
   /**
    * Returns the editable state.
    * 
    * @return whether or not the receiver is editable
    */
	public boolean getEditable()
	{
	   return text.getEditable();
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
