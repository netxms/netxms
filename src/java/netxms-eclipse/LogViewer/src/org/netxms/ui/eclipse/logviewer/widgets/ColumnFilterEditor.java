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
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.ui.forms.events.HyperlinkAdapter;
import org.eclipse.ui.forms.events.HyperlinkEvent;
import org.eclipse.ui.forms.widgets.FormToolkit;
import org.eclipse.ui.forms.widgets.ImageHyperlink;
import org.netxms.client.log.ColumnFilter;
import org.netxms.client.log.LogColumn;
import org.netxms.ui.eclipse.logviewer.widgets.helpers.FilterTreeElement;
import org.netxms.ui.eclipse.shared.SharedIcons;
import org.netxms.ui.eclipse.widgets.DashboardComposite;

/**
 * Widget for editing filter for single column
 */
public class ColumnFilterEditor extends DashboardComposite
{
	private FormToolkit toolkit;
	private LogColumn column;
	private FilterTreeElement columnElement;
	private FilterBuilder filterBuilder;
	private List<ConditionEditor> conditions = new ArrayList<ConditionEditor>();
	
	/**
	 * @param parent
	 * @param style
	 * @param columnElement 
	 * @param column 
	 */
	public ColumnFilterEditor(Composite parent, FormToolkit toolkit, LogColumn column, FilterTreeElement columnElement, final Runnable deleteHandler)
	{
		super(parent, SWT.BORDER);
		
		this.toolkit = toolkit;
		this.column = column;
		this.columnElement = columnElement;
		
		GridLayout layout = new GridLayout();
		setLayout(layout);
		
		final Composite header = new Composite(this, SWT.NONE);
		layout = new GridLayout();
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		layout.numColumns = 3;
		header.setLayout(layout);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		header.setLayoutData(gd);
		
		final Label title = new Label(header, SWT.NONE);
		title.setText(column.getDescription());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		title.setLayoutData(gd);
		
		ImageHyperlink link = new ImageHyperlink(header, SWT.NONE);
		link.setImage(SharedIcons.IMG_ADD_OBJECT);
		link.addHyperlinkListener(new HyperlinkAdapter() {
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
				addElement(ColumnFilterEditor.this.columnElement);
			}
		});

		link = new ImageHyperlink(header, SWT.NONE);
		link.setImage(SharedIcons.IMG_DELETE_OBJECT);
		link.addHyperlinkListener(new HyperlinkAdapter() {
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
				deleteHandler.run();
			}
		});
	}
	
	/**
	 * Add new filter element
	 */
	private void addElement(FilterTreeElement currentElement)
	{
		boolean isGroup = false;
		if (currentElement.getType() == FilterTreeElement.FILTER)
		{
			if (((ColumnFilter)currentElement.getObject()).getType() == ColumnFilter.SET)
				isGroup = true;
		}
		if (currentElement.hasChilds() && !isGroup)
		{
			addGroup(currentElement);
		}
		else
		{
			addCondition(currentElement);
		}
	}
	
	private void addGroup(FilterTreeElement currentElement)
	{
		
	}
	
	/**
	 * Add condition
	 * 
	 * @param currentElement
	 */
	private void addCondition(FilterTreeElement currentElement)
	{
		final ConditionEditor ce = new ConditionEditor(this, toolkit, column, currentElement);
		ce.setDeleteHandler(new Runnable() {
			@Override
			public void run()
			{
				conditions.remove(ce);
				if (conditions.size() > 0)
					conditions.get(0).setLogicalOperation("");
				filterBuilder.getParent().layout(true, true);
			}
		});
		
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		ce.setLayoutData(gd);
		conditions.add(ce);
		if (conditions.size() > 1)
			ce.setLogicalOperation("OR");

		filterBuilder.getParent().layout(true, true);
	}

	/**
	 * @param filterBuilder the filterBuilder to set
	 */
	public void setFilterBuilder(FilterBuilder filterBuilder)
	{
		this.filterBuilder = filterBuilder;
	}
}
