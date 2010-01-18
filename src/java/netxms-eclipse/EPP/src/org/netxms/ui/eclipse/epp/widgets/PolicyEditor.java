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
package org.netxms.ui.eclipse.epp.widgets;

import java.util.ArrayList;
import java.util.List;

import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.ScrolledComposite;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Composite;
import org.netxms.ui.eclipse.epp.widgets.helpers.CellFactory;

/**
 * @author victor
 *
 */
public class PolicyEditor extends Composite
{
	private CellFactory cellFactory;
	private int[] columnWidths;
	private List<? extends Object> content;
	private List<Rule> rules;
	private ScrolledComposite rulesArea;
	private Composite ruleList;
	
	public PolicyEditor(Composite parent, int style, CellFactory cellFactory)
	{
		super(parent, style);
		
		this.cellFactory = cellFactory;
		columnWidths = new int[cellFactory.getColumnCount()];
		for(int i = 0; i < columnWidths.length; i++)
			columnWidths[i] = cellFactory.getDefaultColumnWidth(i);
		
		setLayout(new FillLayout());
		
		rulesArea = new ScrolledComposite(this, SWT.H_SCROLL | SWT.V_SCROLL);
//rulesArea.setBackground(new Color(getShell().getDisplay(), 0, 200, 0));		
		ruleList = new Composite(rulesArea, SWT.NONE);
		ruleList.setLayout(new RowLayout(SWT.VERTICAL));
		rulesArea.setContent(ruleList);
ruleList.setBackground(new Color(getShell().getDisplay(), 0, 0, 200));		
	}

	/**
	 * @return the cellFactory
	 */
	public CellFactory getCellFactory()
	{
		return cellFactory;
	}

	/**
	 * Set content for policy editor
	 * 
	 * @param content List of policy rules
	 */
	public void setContent(List<? extends Object> content)
	{
		this.content = content;
		rules = new ArrayList<Rule>(content.size());
		
		for(Object o : content)
		{
			rules.add(new Rule(ruleList, this, o));
		}
		ruleList.setSize(ruleList.computeSize(SWT.DEFAULT, SWT.DEFAULT));
	}

	/**
	 * Get current width of given column
	 * 
	 * @param index Column index
	 * @return current width of given column
	 */
	public int getColumnWidth(int index)
	{
		return ((index >= 0) && (index < columnWidths.length)) ? columnWidths[index] : 0;
	}

	/**
	 * @return the content
	 */
	public List<? extends Object> getContent()
	{
		return content;
	}
}
