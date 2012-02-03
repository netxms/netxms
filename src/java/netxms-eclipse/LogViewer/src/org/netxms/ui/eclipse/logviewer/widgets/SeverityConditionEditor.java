/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.logviewer.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.forms.widgets.FormToolkit;
import org.netxms.client.constants.Severity;
import org.netxms.client.log.ColumnFilter;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.widgets.ImageCombo;

/**
 * Condition editor for severity columns
 */
public class SeverityConditionEditor extends ConditionEditor
{
	private static final String[] OPERATIONS = { "LIKE", "NOT LIKE" };
	
	private ImageCombo severity;
	
	/**
	 * @param parent
	 * @param toolkit
	 * @param column
	 * @param parentElement
	 */
	public SeverityConditionEditor(Composite parent, FormToolkit toolkit)
	{
		super(parent, toolkit);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.logviewer.widgets.ConditionEditor#getOperations()
	 */
	@Override
	protected String[] getOperations()
	{
		return OPERATIONS;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.logviewer.widgets.ConditionEditor#createContent(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected void createContent(Composite parent)
	{
		severity = new ImageCombo(this, SWT.READ_ONLY | SWT.BORDER);
		toolkit.adapt(severity);
		for(int i = Severity.NORMAL; i <= Severity.CRITICAL; i++)
			severity.add(StatusDisplayInfo.getStatusImage(i), StatusDisplayInfo.getStatusText(i));
		severity.select(0);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		severity.setLayoutData(gd);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.logviewer.widgets.ConditionEditor#createFilter()
	 */
	@Override
	public ColumnFilter createFilter()
	{
		ColumnFilter filter = new ColumnFilter(severity.getSelectionIndex());
		filter.setNegated(getSelectedOperation() == 1);
		return filter;
	}
}
