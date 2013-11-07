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

import java.util.ArrayList;
import java.util.List;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.ui.forms.events.HyperlinkAdapter;
import org.eclipse.ui.forms.events.HyperlinkEvent;
import org.eclipse.ui.forms.widgets.FormToolkit;
import org.eclipse.ui.forms.widgets.ImageHyperlink;
import org.netxms.client.log.ColumnFilter;
import org.netxms.client.log.LogColumn;
import org.netxms.ui.eclipse.logviewer.Messages;
import org.netxms.ui.eclipse.shared.SharedIcons;
import org.netxms.ui.eclipse.widgets.DashboardComposite;

/**
 * Widget for editing filter for single column
 */
public class ColumnFilterEditor extends DashboardComposite
{
	private FormToolkit toolkit;
	private LogColumn column;
	private FilterBuilder filterBuilder;
	private List<ConditionEditor> conditions = new ArrayList<ConditionEditor>();
	private int booleanOperation = ColumnFilter.OR;
	
	/**
	 * @param parent
	 * @param style
	 * @param columnElement 
	 * @param column 
	 */
	public ColumnFilterEditor(Composite parent, FormToolkit toolkit, LogColumn column, final Runnable deleteHandler)
	{
		super(parent, SWT.BORDER);
		
		this.toolkit = toolkit;
		this.column = column;
		
		toolkit.adapt(this);
		
		GridLayout layout = new GridLayout();
		setLayout(layout);
		
		final Composite header = toolkit.createComposite(this);
		layout = new GridLayout();
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		layout.numColumns = 5;
		header.setLayout(layout);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		header.setLayoutData(gd);
		
		final Label title = toolkit.createLabel(header, column.getDescription());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = false;
		title.setLayoutData(gd);
		
		final Composite buttons = toolkit.createComposite(header); 
		layout = new GridLayout();
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		layout.numColumns = 2;
		buttons.setLayout(layout);
		gd = new GridData();
		gd.horizontalAlignment = SWT.CENTER;
		gd.grabExcessHorizontalSpace = true;
		buttons.setLayoutData(gd);

		final Button radioAnd = toolkit.createButton(buttons, Messages.ColumnFilterEditor_AndCondition, SWT.RADIO);
		radioAnd.setSelection(booleanOperation == ColumnFilter.AND);
		radioAnd.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				setBooleanOperation(ColumnFilter.AND);
			}
			
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		});
		
		final Button radioOr = toolkit.createButton(buttons, Messages.ColumnFilterEditor_OrCondition, SWT.RADIO);
		radioOr.setSelection(booleanOperation == ColumnFilter.OR);
		radioOr.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				setBooleanOperation(ColumnFilter.OR);
			}
			
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		});
		
		ImageHyperlink link = toolkit.createImageHyperlink(header, SWT.NONE);
		link.setImage(SharedIcons.IMG_ADD_OBJECT);
		link.addHyperlinkListener(new HyperlinkAdapter() {
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
				addCondition();
			}
		});

		link = toolkit.createImageHyperlink(header, SWT.NONE);
		link.setImage(SharedIcons.IMG_DELETE_OBJECT);
		link.addHyperlinkListener(new HyperlinkAdapter() {
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
				ColumnFilterEditor.this.dispose();
				deleteHandler.run();
			}
		});
	}
	
	/**
	 * @param op
	 */
	private void setBooleanOperation(int op)
	{
		booleanOperation = op;
		final String opName = (op == ColumnFilter.AND) ? Messages.ColumnFilterEditor_And : Messages.ColumnFilterEditor_Or;
		for(int i = 1; i < conditions.size(); i++)
		{
			conditions.get(i).setLogicalOperation(opName);
		}
	}

	/**
	 * Add condition
	 */
	private void addCondition()
	{
		final ConditionEditor ce = createConditionEditor();
		ce.setDeleteHandler(new Runnable() {
			@Override
			public void run()
			{
				conditions.remove(ce);
				if (conditions.size() > 0)
					conditions.get(0).setLogicalOperation(""); //$NON-NLS-1$
				filterBuilder.updateLayout();
			}
		});
		
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		ce.setLayoutData(gd);
		conditions.add(ce);
		if (conditions.size() > 1)
			ce.setLogicalOperation((booleanOperation == ColumnFilter.AND) ? Messages.ColumnFilterEditor_And : Messages.ColumnFilterEditor_Or);

		filterBuilder.updateLayout();
	}
	
	/**
	 * @param currentElement
	 * @return
	 */
	private ConditionEditor createConditionEditor()
	{
		switch(column.getType())
		{
			case LogColumn.LC_ALARM_HD_STATE:
				return new AlarmHDStateConditionEditor(this, toolkit);
			case LogColumn.LC_ALARM_STATE:
				return new AlarmStateConditionEditor(this, toolkit);
			case LogColumn.LC_EVENT_CODE:
				return new EventConditionEditor(this, toolkit);
			case LogColumn.LC_INTEGER:
				return new IntegerConditionEditor(this, toolkit);
			case LogColumn.LC_OBJECT_ID:
				return new ObjectConditionEditor(this, toolkit);
			case LogColumn.LC_SEVERITY:
				return new SeverityConditionEditor(this, toolkit);
			case LogColumn.LC_TIMESTAMP:
				return new TimestampConditionEditor(this, toolkit);
			case LogColumn.LC_USER_ID:
				return new UserConditionEditor(this, toolkit);
			default:
				return new TextConditionEditor(this, toolkit);
		}
	}

	/**
	 * @param filterBuilder the filterBuilder to set
	 */
	public void attachFilterBuilder(FilterBuilder filterBuilder)
	{
		this.filterBuilder = filterBuilder;
		addCondition();
	}
	
	/**
	 * Build filter tree
	 */
	public ColumnFilter buildFilterTree()
	{
		if (conditions.size() == 0)
			return null;
		
		if (conditions.size() > 1)
		{
			ColumnFilter root = new ColumnFilter();
			root.setOperation(booleanOperation);
			for(ConditionEditor e : conditions)
			{
				ColumnFilter filter = e.createFilter();
				root.addSubFilter(filter);
			}
			return root;
		}
		else
		{
			return conditions.get(0).createFilter();
		}
	}
}
