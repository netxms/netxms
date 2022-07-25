/**
 * NetXMS - open source network management system
 * Copyright (C) 2013-2022 Raden Solutions
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
package org.netxms.nxmc.modules.datacollection.widgets.helpers;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.jface.viewers.ITreeContentProvider;
import org.eclipse.jface.viewers.Viewer;
import org.netxms.client.NXCSession;
import org.netxms.client.Table;
import org.netxms.client.TableRow;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.modules.objects.ObjectWrapper;

/**
 * Content provider for NetXMS table viewer
 */
public class SummaryTableContentProvider implements ITreeContentProvider
{
	private Table table = null;
   private NXCSession session = Registry.getSession();

	/**
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

   /**
    * @see org.eclipse.jface.viewers.ITreeContentProvider#getElements(java.lang.Object)
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
		   if (rows[i].getBaseRow() == -1)
		   {
		      list.add(new RowWrapper(rows[i], i, null));
		   }
		   else
		   {
		      RowWrapper p = findParent(list, rows[i].getBaseRow());
		      if (p != null)
		         p.add(new RowWrapper(rows[i], i, p));
		   }
		}
		
		return list.toArray();
	}

	/**
	 * Find parent row
	 * 
	 * @param list row list
	 * @param id parent ID
	 * @return parent row
	 */
	private RowWrapper findParent(List<RowWrapper> list, int id)
	{
	   for(RowWrapper r : list)
	      if (r.id == id)
	         return r;
	   return null;
	}

   /**
    * @see org.eclipse.jface.viewers.ITreeContentProvider#getChildren(java.lang.Object)
    */
   @Override
   public Object[] getChildren(Object parentElement)
   {
      return ((RowWrapper)parentElement).children.toArray();
   }

   /**
    * @see org.eclipse.jface.viewers.ITreeContentProvider#getParent(java.lang.Object)
    */
   @Override
   public Object getParent(Object element)
   {
      return ((RowWrapper)element).parent;
   }

   /**
    * @see org.eclipse.jface.viewers.ITreeContentProvider#hasChildren(java.lang.Object)
    */
   @Override
   public boolean hasChildren(Object element)
   {
      return !((RowWrapper)element).children.isEmpty();
   }

   /**
    * @see org.eclipse.jface.viewers.IContentProvider#dispose()
    */
	@Override
	public void dispose()
	{
	}
	
	/**
	 * Row wrapper for selection provider
	 */
	private class RowWrapper extends TableRow implements ObjectWrapper
	{
	   protected RowWrapper parent;
	   protected int id;
	   protected List<RowWrapper> children = new ArrayList<RowWrapper>(0);
	   
      protected RowWrapper(TableRow src, int id, RowWrapper parent)
      {
         super(src);
         this.id = id;
         this.parent = parent;
      }
      
      protected void add(RowWrapper r)
      {
         children.add(r);
      }

      @Override
      public AbstractObject getObject()
      {
         return session.findObjectById(getObjectId(), AbstractNode.class);
      }
	}
}
