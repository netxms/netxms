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
package org.netxms.ui.eclipse.epp.widgets.helpers;

import org.netxms.ui.eclipse.epp.widgets.Cell;
import org.netxms.ui.eclipse.epp.widgets.CommentCell;
import org.netxms.ui.eclipse.epp.widgets.EventsCell;
import org.netxms.ui.eclipse.epp.widgets.ObjectsCell;
import org.netxms.ui.eclipse.epp.widgets.Rule;
import org.netxms.ui.eclipse.epp.widgets.ScriptCell;

/**
 * Cell factory for event processing policy
 *
 */
public class EPPCellFactory extends CellFactory
{
	private static final int COLUMN_COUNT = 4;
	
	private static final int COLUMN_OBJECTS = 0;
	private static final int COLUMN_EVENTS = 1;
	private static final int COLUMN_SCRIPT = 2;
	private static final int COLUMN_COMMENTS = 3;
	
	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.epp.widgets.helpers.CellFactory#createCell(int, org.netxms.ui.eclipse.epp.widgets.Rule, java.lang.Object)
	 */
	@Override
	public Cell createCell(int column, Rule rule, Object data)
	{
		switch(column)
		{
			case COLUMN_OBJECTS:
				return new ObjectsCell(rule, data);
			case COLUMN_EVENTS:
				return new EventsCell(rule, data);
			case COLUMN_SCRIPT:
				return new ScriptCell(rule, data);
			case COLUMN_COMMENTS:
				return new CommentCell(rule, data);
			default:
				return null;
		}
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.epp.widgets.helpers.CellFactory#getColumnCount()
	 */
	@Override
	public int getColumnCount()
	{
		return COLUMN_COUNT;
	}
}
