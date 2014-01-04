/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
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
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Spinner;
import org.eclipse.ui.forms.widgets.FormToolkit;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Composite widget - spinner with label
 */
public class LabeledSpinner extends Composite
{
	private Label label;
	private Spinner spinner;
	private FormToolkit toolkit;
	
	/**
	 * @param parent
	 * @param style
	 */
	public LabeledSpinner(Composite parent, int style)
	{
		super(parent, style);
		toolkit = null;
		createContent(SWT.BORDER);
	}

	/**
	 * @param parent
	 * @param style
	 * @param spinnerStyle
	 */
	public LabeledSpinner(Composite parent, int style, int spinnerStyle)
	{
		super(parent, style);
		toolkit = null;
		createContent(spinnerStyle);
	}
	
	/**
	 * @param parent
	 * @param style
	 * @param spinnerStyle
	 * @param toolkit
	 */
	public LabeledSpinner(Composite parent, int style, int spinnerStyle, FormToolkit toolkit)
	{
		super(parent, style);
		this.toolkit = toolkit;
		toolkit.adapt(this);
		createContent(spinnerStyle);
	}
	
	/**
	 * Do widget creation.
	 * 
	 * @param spinnerStyle
	 */
	private void createContent(int spinnerStyle)
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
		
		spinner = new Spinner(this, spinnerStyle);
		if (toolkit != null)
		   toolkit.adapt(spinner);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.TOP;
		spinner.setLayoutData(gd);
	}
	
	/**
	 * Set spinner range
	 * 
	 * @param minValue
	 * @param maxValue
	 */
	public void setRange(int minValue, int maxValue)
	{
      spinner.setMinimum(minValue);
      spinner.setMaximum(maxValue);
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
	 * Set selection.
	 * 
	 * @param value new value
	 */
	public void setSelection(int value)
	{
	   spinner.setSelection(value);
	}
	
	/**
	 * Get current selection
	 * 
	 * @return
	 */
	public int getSelection()
	{
	   return spinner.getSelection();
	}
	
	/**
	 * Get spinner control
	 * 
	 * @return spinner control
	 */
	public Spinner getSpinnerControl()
	{
		return spinner;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.widgets.Control#setEnabled(boolean)
	 */
	@Override
	public void setEnabled(boolean enabled)
	{
		super.setEnabled(enabled);
		spinner.setEnabled(enabled);
		label.setEnabled(enabled);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.widgets.Composite#setFocus()
	 */
	@Override
	public boolean setFocus()
	{
		return spinner.setFocus();
	}
}
