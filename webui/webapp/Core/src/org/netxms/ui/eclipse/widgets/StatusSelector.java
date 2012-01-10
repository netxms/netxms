/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.netxms.client.constants.Severity;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Abstract selector widget
 *
 */
public class StatusSelector extends Composite
{
	private Label label;
	private Combo combo;
	
	/**
	 * Create abstract selector.
	 * 
	 * @param parent
	 * @param style
	 */
	public StatusSelector(Composite parent, int style, int maxSeverity)
	{
		super(parent, style);
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.INNER_SPACING;
		layout.horizontalSpacing = WidgetHelper.INNER_SPACING;
		layout.marginTop = 0;
		layout.marginBottom = 0;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		setLayout(layout);
		
		label = new Label(this, SWT.NONE);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.horizontalSpan = 2;
		label.setLayoutData(gd);
		
		combo = new Combo(this, SWT.DROP_DOWN | SWT.READ_ONLY | SWT.BORDER);
		for(int i = 0; i <= Math.min(maxSeverity, Severity.TESTING); i++)
			combo.add(StatusDisplayInfo.getStatusText(i));
		combo.select(0);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.TOP;
		combo.setLayoutData(gd);
	}
	
	/**
	 * @return
	 */
	public int getSelection()
	{
		return combo.getSelectionIndex();
	}
	
	/**
	 * @param selection
	 */
	public void setSelection(int selection)
	{
		combo.select(selection);
	}

	/**
	 * Returns tooltip text for text area. Can be overridden by subclasses.
	 * @return tooltip text for selection button
	 */
	protected String getTextToolTip()
	{
		return null;
	}

	/**
	 * Set selector's label
	 * 
	 * @param newLabel New label
	 */
	public void setLabel(final String newLabel)
	{
		label.setText(newLabel);
	}
	
	/**
	 * Get selector's label
	 * 
	 * @return Current selector's label
	 */
	public String getLabel()
	{
		return label.getText();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.widgets.Control#setEnabled(boolean)
	 */
	@Override
	public void setEnabled(boolean enabled)
	{
		combo.setEnabled(enabled);
		super.setEnabled(enabled);
	}
	
	/**
	 * Get text control
	 * @return text control
	 */
	protected Control getComboControl()
	{
		return combo;
	}
}
