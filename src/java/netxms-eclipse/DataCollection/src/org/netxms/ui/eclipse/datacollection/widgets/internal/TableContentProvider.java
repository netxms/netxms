/**
 * NetXMS - open source network management system
 * Copyright (C) 2013-2015 Raden Solutions
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
package org.netxms.ui.eclipse.datacollection.widgets.internal;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.jface.viewers.IStructuredContentProvider;
import org.eclipse.jface.viewers.Viewer;
import org.netxms.client.NXCSession;
import org.netxms.client.Table;
import org.netxms.client.TableRow;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.objects.ObjectWrapper;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Content provider for NetXMS table viewer
 */
public class TableContentProvider implements IStructuredContentProvider
{
	private Table table = null;
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IContentProvider#inputChanged(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public void inputChanged(Viewer viewer, Object oldInput, Object newInput)
	{
		if ((newInput != null) && (newInput instanceof Table))
		{
			table = (Table)newInput;
		}
		else
		{
			table = null;
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IStructuredContentProvider#getElements(java.lang.Object)
	 */
	@Override
	public Object[] getElements(Object inputElement)
	{
		if (table == null)
			return new Object[0];
		
		List <RowWrapper> list = new ArrayList<RowWrapper>();
		TableRow[] rows = table.getAllRows();
		for(int i = 0; i < rows.length; i++)
		{
		   list.add(new RowWrapper(rows[i]));		   
		}
		
		return list.toArray();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IContentProvider#dispose()
	 */
	@Override
	public void dispose()
	{
	}
	
	/**
	 * Row wrapper for selection provider
	 */
	class RowWrapper extends TableRow implements ObjectWrapper
	{
      public RowWrapper(TableRow src)
      {
         super(src);
      }

      @Override
      public AbstractObject getObject()
      {
         return ((NXCSession)ConsoleSharedData.getSession()).findObjectById(getObjectId(), AbstractNode.class);
      }
	}
}
