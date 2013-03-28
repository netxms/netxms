/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CTabFolder;
import org.eclipse.swt.custom.CTabItem;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.part.ViewPart;
import org.eclipse.ui.plugin.AbstractUIPlugin;
import org.netxms.client.objects.AbstractObject;

/**
 * Abstract object tab class
 *
 */
public abstract class ObjectTab
{
	private ViewPart viewPart;
	private CTabFolder tabFolder;
	private CTabItem tabItem;
	private Composite clientArea;
	private AbstractObject object;
	private String name;
	private int order;
	private ImageDescriptor icon;
	private Image tabImage;

	/**
	 * Read configuration before widget creation
	 * 
	 * @param ce Eclipse registry configuration element
	 */
	public void configure(IConfigurationElement ce, ViewPart viewPart)
	{
		this.viewPart = viewPart;
		
		name = ce.getAttribute("name");
		if (name == null)
			name = "<noname>";
		
		try
		{
			order = Integer.parseInt(ce.getAttribute("order"), 10);
		}
		catch(NumberFormatException e)
		{
			order = Integer.MAX_VALUE;
		}
		
		String path = ce.getAttribute("icon");
		if (path != null)
		{
			icon = AbstractUIPlugin.imageDescriptorFromPlugin(ce.getContributor().getName(), path);
		}
	}
	
	/**
	 * Create object tab. Intended to be called only by TabbedObjectView.
	 * 
	 * @param tabFolder Parent tab folder
	 * @param ce Configuration element
	 */
	public void create(final CTabFolder tabFolder)
	{
		this.tabFolder = tabFolder;
		clientArea = new Composite(tabFolder, SWT.NONE);
		clientArea.setLayout(new FillLayout());
		createTabContent(clientArea);
		clientArea.setVisible(false);
	}
	
	/**
	 * Create tab's content.
	 * 
	 * @param parent Parent composite
	 */
	protected abstract void createTabContent(Composite parent);
	
	/**
	 * Called by framework when tab is selected.
	 */
	public void selected()
	{
		clientArea.moveAbove(null);
		clientArea.setFocus();
	}
	
	/**
	 * Test if tab should be shown for given NetXMS object. Default implementation always returns true.
	 * 
	 * @param object Object to test
	 * @return Should return true if tab must be shown for given object
	 */
	public boolean showForObject(final AbstractObject object)
	{
		return true;
	}
	
	/**
	 * Called by parent view to inform tab that currently selected object was changed.
	 * 
	 * @param object New object to display
	 */
	public abstract void objectChanged(AbstractObject object);
	
	/**
	 * Change current object. Intended to be called only by parent view.
	 * 
	 * @param object New object to display
	 */
	public void changeObject(AbstractObject object)
	{
		this.object = object;
		objectChanged(object);
	}
	
	/**
	 * Called by parent view to inform tab that update notification for
	 * current object was received from server. Default implementation do nothing.
	 * This method always called in UI thread.
	 * 
	 * @param object new instance of current object
	 */
	public void currentObjectUpdated(AbstractObject object)
	{
	}
	
	/**
	 * Show tab
	 */
	public void show()
	{
		if (tabItem == null)
		{
			// Find insertion point
			int index = tabFolder.getItemCount();
			for(int i = 0; i < tabFolder.getItemCount(); i++)
			{
				ObjectTab tab = (ObjectTab)tabFolder.getItem(i).getData();
				if (tab.getOrder() > order)
				{
					index = i;
					break;
				}
			}
			
			tabItem = new CTabItem(tabFolder, SWT.NONE, index);
			tabItem.setText(name);
			if (icon != null)
			{
				tabImage = icon.createImage();
				tabItem.setImage(tabImage);
			}
			tabItem.setControl(clientArea);
			tabItem.setData(this);
			clientArea.setVisible(true);
		}
	}
	
	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}

	/**
	 * Hide tab
	 */
	public void hide()
	{
		if (tabItem != null)
		{
			tabItem.setControl(null);
			tabItem.dispose();
			tabItem = null;
			if (tabImage != null)
			{
				tabImage.dispose();
				tabImage = null;
			}
			clientArea.setVisible(false);
		}
	}
	
	/**
	 * Check if this tab is currently visible
	 * 
	 * @return
	 */
	public final boolean isVisible()
	{
		return tabItem != null;
	}

	/**
	 * Get currently selected object.
	 * 
	 * @return Currently selected object
	 */
	public AbstractObject getObject()
	{
		return object;
	}

	/**
	 * @return the order
	 */
	public int getOrder()
	{
		return order;
	}

	/**
	 * @return the viewPart
	 */
	public ViewPart getViewPart()
	{
		return viewPart;
	}
	
	/**
	 * Disposes the tab.
	 * 
	 * This is the last method called on the ObjectTab. There is no guarantee that createTabContent() has been called,
	 * so the tab controls may never have been created.
	 * 
	 * Within this method a tab may release any resources, fonts, images, etc.  held by this tab. It is also very
	 * important to deregister all listeners from the workbench.
	 * 
	 * Clients should not call this method (the tab manager calls this method at appropriate times). 
	 */
	public void dispose()
	{
		if (tabImage != null)
		{
			tabImage.dispose();
			tabImage = null;
		}
	}

	/**
	 * @return the clientArea
	 */
	protected Composite getClientArea()
	{
		return clientArea;
	}

	/**
	 * Called by tab manager when user press refresh button in tab view.
	 * Default implementation do nothing.
	 */
	public void refresh()
	{
	}
	
	/**
	 * Get selection provider on tab, if any. If there are no selection provider
	 * on tab, must return null. Default implementation returns null.
	 * 
	 * @return selection provider on tab
	 */
	public ISelectionProvider getSelectionProvider()
	{
		return null;
	}
}
