/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2009 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.logviewer.widgets.helpers;

import java.util.ArrayList;
import org.netxms.ui.eclipse.logviewer.widgets.ColumnFilterEditor;

/**
 * @author Victor
 *
 */
public class FilterTreeElement
{
	public static final int ROOT = 0;
	public static final int COLUMN = 1;
	public static final int FILTER = 2;
	
	private int type;
	private Object object;
	private FilterTreeElement parent;
	private ArrayList<FilterTreeElement> childs;
	private ColumnFilterEditor editor;
	
	/**
	 * @param type
	 * @param object
	 * @param parent
	 */
	public FilterTreeElement(int type, Object object, FilterTreeElement parent)
	{
		this.type = type;
		this.object = object;
		this.parent = parent;
		childs = new ArrayList<FilterTreeElement>();
		if (parent != null)
			parent.addChild(this);
	}

	/**
	 * @return the type
	 */
	public int getType()
	{
		return type;
	}

	/**
	 * @return the object
	 */
	public Object getObject()
	{
		return object;
	}

	/**
	 * @return the parent
	 */
	public FilterTreeElement getParent()
	{
		return parent;
	}

	/**
	 * @return the childs
	 */
	public FilterTreeElement[] getChilds()
	{
		return childs.toArray(new FilterTreeElement[childs.size()]);
	}
	
	/**
	 * Returns true if element has childs
	 * 
	 * @return true if element has childs
	 */
	public boolean hasChilds()
	{
		return childs.size() > 0;
	}
	
	/**
	 * Get number of child elements
	 * 
	 * @return Number of child elements
	 */
	public int getNumChilds()
	{
		return childs.size();
	}
	
	/**
	 * Add child element
	 * 
	 * @param element Element to add
	 */
	public void addChild(final FilterTreeElement element)
	{
		childs.add(element);
	}
	
	/**
	 * Remove child element
	 * 
	 * @param element Element to remove
	 */
	public void removeChild(final FilterTreeElement element)
	{
		childs.remove(element);
	}
	
	/**
	 * Change elemnt's parent
	 * 
	 * @param newParent New parent element
	 */
	public void changeParent(final FilterTreeElement newParent)
	{
		if (parent != null)
			parent.removeChild(this);
		parent = newParent;
		if (parent != null)
			parent.addChild(this);
	}

	/**
	 * @return the editor
	 */
	public ColumnFilterEditor getEditor()
	{
		return editor;
	}

	/**
	 * @param editor the editor to set
	 */
	public void setEditor(ColumnFilterEditor editor)
	{
		this.editor = editor;
	}
}
