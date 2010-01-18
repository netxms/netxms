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
	private PolicyEditor editor;
	private CellFactory cellFactory;
	private ArrayList<Cell> cells;
	
	public Rule(Composite parent, PolicyEditor editor, Object data)
	{
		super(parent, SWT.NONE);
		this.editor = editor;
		this.cellFactory = editor.getCellFactory();
		
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
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.widgets.Composite#computeSize(int, int, boolean)
	 */
	@Override
	public Point computeSize(int wHint, int hHint, boolean changed)
	{
		int width = 0;
		int height = 10;
		for(int i = 0; i < cells.size(); i++)
		{
			Cell cell = cells.get(i);
			Point size = cell.computeSize(editor.getColumnWidth(i), SWT.DEFAULT);
			if (size.y > height)
				height = size.y;
			width += editor.getColumnWidth(i);  // FIXME: add borders ?
		}
		return new Point(width, height);
	}

	/**
	 * Resize rule widget
	 */
	private void resize()
	{
		int height = getSize().y;
		for(int i = 0, x = 0; i < cells.size(); i++)
		{
			Cell cell = cells.get(i);
			cell.setLocation(x, 0);
			int width = editor.getColumnWidth(i);
			cell.setSize(width, height);
			x += width;
		}
	}
}
