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

import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CLabel;
import org.eclipse.swt.events.ControlAdapter;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.widgets.Composite;
import org.netxms.ui.eclipse.epp.widgets.helpers.CellFactory;

/**
 * Rule widget for policy editor
 *
 */
public class Rule extends Composite
{
	public static final int CELL_SPACING = 1;
	
	private static final int HEADER_WIDTH = 45;
	
	private PolicyEditor editor;
	private CellFactory cellFactory;
	private ArrayList<Cell> cells;
	private RuleHeader header;
	private int id;
	private boolean collapsed;
	private String name;
	private CLabel collapsedLabel;
	
	public Rule(Composite parent, PolicyEditor editor, Object data, int id)
	{
		super(parent, SWT.NONE);
		this.editor = editor;
		this.cellFactory = editor.getCellFactory();
		this.id = id;
		collapsed = false;
		name = "";
		
		header = new RuleHeader(this);
		
		collapsedLabel = new CLabel(this, SWT.LEFT);
		collapsedLabel.setVisible(collapsed);
		collapsedLabel.setBackground(PolicyEditor.COLOR_COLLAPSED_RULE);
		
		cells = new ArrayList<Cell>(cellFactory.getColumnCount());
		for(int i = 0; i < cellFactory.getColumnCount(); i++)
		{
			Cell cell = cellFactory.createCell(i, this, data);
			cells.add(cell);
		}
		
		addControlListener(new ControlAdapter() {
			/* (non-Javadoc)
			 * @see org.eclipse.swt.events.ControlAdapter#controlResized(org.eclipse.swt.events.ControlEvent)
			 */
			@Override
			public void controlResized(ControlEvent e)
			{
				resize();
			}
		});
		
		setBackground(parent.getBackground());
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.widgets.Composite#computeSize(int, int, boolean)
	 */
	@Override
	public Point computeSize(int wHint, int hHint, boolean changed)
	{
		int width = 0;
		int height = header.computeSize(wHint, hHint, changed).y;
		
		if (collapsed)
		{
			for(int i = 0; i < cells.size(); i++)
				width += editor.getColumnWidth(i) + CELL_SPACING;
		}
		else
		{
			for(int i = 0; i < cells.size(); i++)
			{
				Cell cell = cells.get(i);
				Point size = cell.computeSize(editor.getColumnWidth(i), SWT.DEFAULT);
				if (size.y > height)
					height = size.y;
				width += editor.getColumnWidth(i) + CELL_SPACING;
			}
		}
		return new Point(width + HEADER_WIDTH, height);
	}

	/**
	 * Resize rule widget
	 */
	private void resize()
	{
		int height = getSize().y;
		
		header.setLocation(0, 0);
		header.setSize(HEADER_WIDTH, height);
		
		for(int i = 0, x = HEADER_WIDTH + CELL_SPACING; i < cells.size(); i++)
		{
			Cell cell = cells.get(i);
			cell.setLocation(x, 0);
			int width = editor.getColumnWidth(i);
			cell.setSize(width, height);
			x += width + CELL_SPACING;
		}
		
		collapsedLabel.setLocation(HEADER_WIDTH + CELL_SPACING, 0);
		collapsedLabel.setSize(getSize().x - HEADER_WIDTH - CELL_SPACING, height);
	}
	
	/**
	 * Show or hide all cells
	 * @param visible Visibility flag
	 */
	private void setCellsVisible(boolean visible)
	{
		for(Cell cell : cells)
		{
			cell.setVisible(visible);
		}
	}

	/**
	 * @return the id
	 */
	public int getId()
	{
		return id;
	}

	/**
	 * @param id the id to set
	 */
	public void setId(int id)
	{
		this.id = id;
	}

	/**
	 * @return the collapsed
	 */
	public boolean isCollapsed()
	{
		return collapsed;
	}

	/**
	 * @param collapsed the collapsed to set
	 */
	public void setCollapsed(boolean collapsed)
	{
		this.collapsed = collapsed;
		setCellsVisible(!collapsed);
		collapsedLabel.setVisible(collapsed);
		editor.adjustRuleAreaSize();
	}

	/**
	 * @return the name
	 */
	public String getRuleName()
	{
		return name;
	}

	/**
	 * @param name the name to set
	 */
	public void setRuleName(String name)
	{
		this.name = name;
		collapsedLabel.setText((name != null) ? name : "");
	}
}
