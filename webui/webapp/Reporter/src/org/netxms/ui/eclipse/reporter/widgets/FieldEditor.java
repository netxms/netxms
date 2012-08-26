/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.reporter.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.ui.forms.widgets.FormToolkit;
import org.netxms.ui.eclipse.reporter.widgets.helpers.ReportParameter;
import org.netxms.ui.eclipse.tools.WidgetHelper;


/**
 * Abstract field editor for report
 */
public abstract class FieldEditor extends Composite
{
	private static final long serialVersionUID = 1L;

	protected ReportParameter parameter;
	protected FormToolkit toolkit;

	private Label label;
	
	/**
	 * @param parameter
	 * @param toolkit
	 * @param parent
	 */
	public FieldEditor(ReportParameter parameter, FormToolkit toolkit, Composite parent)
	{
		super(parent, SWT.NONE);
		this.parameter = parameter;
		this.toolkit = toolkit;

		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.INNER_SPACING;
		layout.marginTop = 0;
		layout.marginBottom = 0;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		setLayout(layout);
		
		label = toolkit.createLabel(this, parameter.getDisplayName());
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		label.setLayoutData(gd);
		
		createContent(this);
	}
	
	/**
	 * Create editor's content.
	 * 
	 * @param parent parent composite
	 */
	abstract protected void createContent(Composite parent);
	
	/**
	 * Get current value for parameter being edited.
	 * 
	 * @return current value for parameter
	 */
	abstract public String getValue();
}
