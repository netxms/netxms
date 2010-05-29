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
package org.netxms.ui.eclipse.objectview.objecttabs;

import org.eclipse.core.runtime.IConfigurationElement;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CTabFolder;
import org.eclipse.swt.custom.CTabItem;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.objects.GenericObject;

/**
 * Abstract object tab class
 *
 */
public abstract class ObjectTab
{
	private CTabItem tabItem;
	private GenericObject object;
	
	/**
	 * Create object tab. Intended to be called only by TabbedObjectView.
	 * 
	 * @param tabFolder Parent tab folder
	 * @param ce Configuration element
	 */
	public void create(CTabFolder tabFolder, IConfigurationElement ce)
	{
		tabItem = new CTabItem(tabFolder, SWT.NONE);
		String name = ce.getAttribute("name");
		tabItem.setText((name != null) ? name : "<noname>");
		final Composite clientArea = new Composite(tabFolder, SWT.NONE);
		tabItem.setControl(clientArea);
		createTabContent(clientArea);
	}
	
	/**
	 * Create tab's content.
	 * 
	 * @param parent Parent composite
	 */
	protected abstract void createTabContent(Composite parent);
	
	/**
	 * Test if tab should be shown for given NetXMS object. Default implementation always returns true.
	 * 
	 * @param object Object to test
	 * @return Should return true if tab must be shown for given object
	 */
	public boolean showForObject(GenericObject object)
	{
		return true;
	}
	
	/**
	 * Called by parent view to inform tab that currently selected object was changed.
	 * 
	 * @param object New object to display
	 */
	public abstract void objectChanged(GenericObject object);
	
	/**
	 * Change current object. Intended to be called only by parent view.
	 * 
	 * @param object New object to display
	 */
	public void changeObject(GenericObject object)
	{
		this.object = object;
		objectChanged(object);
	}

	/**
	 * Get currently selected object.
	 * 
	 * @return Currently selected object
	 */
	public GenericObject getObject()
	{
		return object;
	}
}
