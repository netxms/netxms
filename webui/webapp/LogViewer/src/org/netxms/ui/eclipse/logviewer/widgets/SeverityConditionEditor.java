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
package org.netxms.ui.eclipse.logviewer.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.forms.widgets.FormToolkit;
import org.netxms.client.constants.Severity;
import org.netxms.client.log.ColumnFilter;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.logviewer.Messages;

/**
 * Condition editor for severity columns
 */
public class SeverityConditionEditor extends ConditionEditor
{
	private final String[] OPERATIONS = { Messages.get().SeverityConditionEditor_Is, Messages.get().SeverityConditionEditor_IsNot, Messages.get().SeverityConditionEditor_Below, Messages.get().SeverityConditionEditor_Above };
	
	private Combo severity;
	
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
		severity = new Combo(this, SWT.READ_ONLY | SWT.BORDER);
		toolkit.adapt(severity);
		for(int i = Severity.NORMAL.getValue(); i <= Severity.CRITICAL.getValue(); i++)
			severity.add(StatusDisplayInfo.getStatusText(i));
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
		switch(getSelectedOperation())
		{
			case 0:	// IS
				return new ColumnFilter(ColumnFilter.EQUALS, severity.getSelectionIndex());
			case 1:	// IS NOT
				ColumnFilter filter = new ColumnFilter(ColumnFilter.EQUALS, severity.getSelectionIndex());
				filter.setNegated(true);
				return filter;
			case 2:	// BELOW
				return new ColumnFilter(ColumnFilter.LESS, severity.getSelectionIndex());
			case 3:	// ABOVE
				return new ColumnFilter(ColumnFilter.GREATER, severity.getSelectionIndex());
		}
		return new ColumnFilter(ColumnFilter.EQUALS, severity.getSelectionIndex());
	}
}
