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
package org.netxms.ui.eclipse.networkmaps.views.helpers;

import org.eclipse.swt.widgets.Composite;
import org.eclipse.zest.core.viewers.GraphViewer;

/**
 * Workaround for bug #244496
 * (https://bugs.eclipse.org/bugs/show_bug.cgi?id=244496) 
 *
 */
public class ExtendedGraphViewer extends GraphViewer
{
	/**
	 * @param composite
	 * @param style
	 */
	public ExtendedGraphViewer(Composite composite, int style)
	{
		super(composite, style);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.StructuredViewer#internalRefresh(java.lang.Object, boolean)
	 */
	@SuppressWarnings("restriction")
	@Override
	protected void internalRefresh(Object element, boolean updateLabels)
	{
		if (getInput() == null)
		{
			return;
		}
		if (element == getInput())
		{
			getFactory().refreshGraph(getGraphControl());
		} 
		else
		{
			getFactory().refresh(getGraphControl(), element, updateLabels);
		}
	}
}
