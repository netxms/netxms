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
import org.eclipse.swt.layout.RowData;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Text;
import org.eclipse.ui.forms.widgets.FormToolkit;
import org.netxms.client.log.ColumnFilter;
import org.netxms.ui.eclipse.logviewer.Messages;

/**
 * Condition editor for timestamp columns
 */
public class IntegerConditionEditor extends ConditionEditor
{
	private static final String[] OPERATIONS = { Messages.IntegerConditionEditor_Equal, Messages.IntegerConditionEditor_NotEqual, "<", "<=", ">=", ">", Messages.IntegerConditionEditor_Between };  //$NON-NLS-1$ //$NON-NLS-2$//$NON-NLS-3$ //$NON-NLS-4$ //$NON-NLS-5$ //$NON-NLS-6$

	private Text value1;
	private Text value2;
	private Label andLabel;
	
	/**
	 * @param parent
	 * @param toolkit
	 * @param column
	 * @param parentElement
	 */
	public IntegerConditionEditor(Composite parent, FormToolkit toolkit)
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
		Composite group = new Composite(this, SWT.NONE);
		RowLayout layout = new RowLayout();
		layout.type = SWT.HORIZONTAL;
		layout.marginBottom = 0;
		layout.marginTop = 0;
		layout.marginLeft = 0;
		layout.marginRight = 0;
		group.setLayout(layout);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		group.setLayoutData(gd);

		value1 = toolkit.createText(group, "0"); //$NON-NLS-1$
		RowData rd = new RowData();
		rd.width = 90;
		value1.setLayoutData(rd);
		andLabel = toolkit.createLabel(group, Messages.IntegerConditionEditor_And);
		andLabel.setVisible(false);
		value2 = toolkit.createText(group, "0"); //$NON-NLS-1$
		rd = new RowData();
		rd.width = 90;
		value2.setLayoutData(rd);
		value2.setVisible(false);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.logviewer.widgets.ConditionEditor#operationSelectionChanged(int)
	 */
	@Override
	protected void operationSelectionChanged(int selectionIndex)
	{
		if (selectionIndex == 6)	// between
		{
			andLabel.setVisible(true);
			value2.setVisible(true);
		}
		else
		{
			andLabel.setVisible(false);
			value2.setVisible(false);
		}
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.logviewer.widgets.ConditionEditor#createFilter()
	 */
	@Override
	public ColumnFilter createFilter()
	{
		long n1;
		try
		{
			n1 = Long.parseLong(value1.getText());
		}
		catch(NumberFormatException e)
		{
			n1 = 0;
		}
		
		ColumnFilter filter;
		switch(getSelectedOperation())
		{
			case 1:	// not equal
				filter = new ColumnFilter(ColumnFilter.EQUALS, n1);
				filter.setNegated(true);
				break;
			case 2:	// less
				filter = new ColumnFilter(ColumnFilter.LESS, n1);
				break;
			case 3:	// less or equal
				filter = new ColumnFilter(ColumnFilter.GREATER, n1);
				filter.setNegated(true);
				break;
			case 4:	// greater or equal
				filter = new ColumnFilter(ColumnFilter.LESS, n1);
				filter.setNegated(true);
				break;
			case 5:	// greater
				filter = new ColumnFilter(ColumnFilter.GREATER, n1);
				break;
			case 6:	// between
				long n2;
				try
				{
					n2 = Long.parseLong(value2.getText());
				}
				catch(NumberFormatException e)
				{
					n2 = 0;
				}
				filter = new ColumnFilter(n1, n2);
				break;
			default:
				filter = new ColumnFilter(ColumnFilter.EQUALS, n1);
				break;
		}
		return filter;
	}
}
