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
package org.netxms.nxmc.modules.reporting.widgets;

import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.netxms.client.reporting.ReportParameter;
import org.netxms.nxmc.tools.WidgetHelper;

/**
 * Abstract field editor for report
 */
public abstract class ReportFieldEditor extends Composite
{
	protected ReportParameter parameter;
	protected ReportFieldEditor dependantEditor;

	private Label label;

	/**
	 * @param parameter
	 * @param toolkit
	 * @param parent
	 */
   public ReportFieldEditor(ReportParameter parameter, Composite parent)
	{
		super(parent, SWT.NONE);
		this.parameter = parameter;

      setupLocalization();

		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.INNER_SPACING;
		layout.marginTop = 0;
		layout.marginBottom = 0;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		setLayout(layout);

      label = new Label(this, SWT.NONE);
      label.setText(parameter.getDescription());
      label.setFont(JFaceResources.getBannerFont());
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
		label.setLayoutData(gd);

		Control content = createContent(this);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.TOP;
      content.setLayoutData(gd);
	}

   /**
    * Setup localization as needed
    */
   abstract protected void setupLocalization();

	/**
	 * Create editor's content.
	 * 
	 * @param parent parent composite
	 */
	abstract protected Control createContent(Composite parent);

	/**
	 * Get current value for parameter being edited.
	 * 
	 * @return current value for parameter
	 */
	abstract public String getValue();

	/**
	 * @param editor
	 */
	public void setDependantEditor(ReportFieldEditor editor)
	{
		this.dependantEditor = editor;
	}

	/**
	 * @param parentEditor
	 */
	public void parentEditorChanged(ReportFieldEditor parentEditor)
	{
	}
}
