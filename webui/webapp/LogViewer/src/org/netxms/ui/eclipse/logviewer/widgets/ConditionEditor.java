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
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.ui.forms.events.HyperlinkAdapter;
import org.eclipse.ui.forms.events.HyperlinkEvent;
import org.eclipse.ui.forms.widgets.FormToolkit;
import org.eclipse.ui.forms.widgets.ImageHyperlink;
import org.netxms.client.log.ColumnFilter;
import org.netxms.ui.eclipse.shared.SharedIcons;

/**
 * Editor widget for single log viewer condition
 */
public abstract class ConditionEditor extends Composite
{
	protected FormToolkit toolkit;
	private Runnable deleteHandler;
	private Label logicalOperation;
	private Combo operation;
	
	/**
	 * @param parent
	 * @param style
	 */
	public ConditionEditor(Composite parent, FormToolkit toolkit)
	{
		super(parent, SWT.NONE);
		
		this.toolkit = toolkit;

		GridLayout layout = new GridLayout();
		layout.numColumns = 4;
		setLayout(layout);
		
		logicalOperation = new Label(this, SWT.NONE);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.CENTER;
		gd.widthHint = 30;
		logicalOperation.setLayoutData(gd);
		
		operation = new Combo(this, SWT.READ_ONLY);
		toolkit.adapt(operation);
		for(String s : getOperations())
			operation.add(s);
		operation.select(0);
		operation.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				operationSelectionChanged(operation.getSelectionIndex());
			}
			
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		});
		
		createContent(this);
		
		ImageHyperlink link = new ImageHyperlink(this, SWT.NONE);
		link.setImage(SharedIcons.IMG_DELETE_OBJECT);
		link.addHyperlinkListener(new HyperlinkAdapter() {
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
				ConditionEditor.this.dispose();
				deleteHandler.run();
			}
		});
	}
	
	/**
	 * @param selectionIndex
	 */
	protected void operationSelectionChanged(int selectionIndex)
	{
	}

	/**
	 * Get possible operations
	 * 
	 * @return
	 */
	protected abstract String[] getOperations();
	
	/**
	 * Create editor content
	 * 
	 * @param parent
	 */
	protected abstract void createContent(Composite parent);
	
	/**
	 * Create log filter
	 * 
	 * @return
	 */
	public abstract ColumnFilter createFilter();

	/**
	 * @param name
	 */
	public void setLogicalOperation(String name)
	{
		logicalOperation.setText(name);
	}

	/**
	 * @param deleteHandler the deleteHandler to set
	 */
	public void setDeleteHandler(Runnable deleteHandler)
	{
		this.deleteHandler = deleteHandler;
	}
	
	/**
	 * @return
	 */
	protected int getSelectedOperation()
	{
		return operation.getSelectionIndex();
	}
}
