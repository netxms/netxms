/*******************************************************************************
 * Copyright (c) 2008, 2011 EclipseSource and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *    EclipseSource - initial API and implementation
 ******************************************************************************/
package org.netxms.webui.design.managers;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.jface.action.IContributionItem;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.action.SubContributionItem;
import org.eclipse.rwt.lifecycle.WidgetUtil;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.MenuItem;
import org.eclipse.swt.widgets.ToolBar;
import org.eclipse.swt.widgets.ToolItem;

public class MenuBarManager extends MenuManager
{
	private static final long serialVersionUID = 1L;
	private static final String MENU_BAR_VARIANT = "menuBar"; //$NON-NLS-1$
	private Composite menuParent;
	@SuppressWarnings("rawtypes")
	private List toolItemList = new ArrayList();
	private ToolBar toolbar;

	public void fill(final Composite parent)
	{
		menuParent = parent;
		toolbar = new ToolBar(parent, SWT.WRAP);
		toolbar.setData(WidgetUtil.CUSTOM_VARIANT, MENU_BAR_VARIANT);
		update(false, false);
	}

	protected void update(final boolean force, final boolean recursive)
	{
		super.update(force, recursive);
		if (menuParent != null && (force || isDirty()))
		{
			disposeToolItems();
			IContributionItem[] items = getItems();
			if (items.length > 0 && menuParent != null)
			{
				for(int i = 0; i < items.length; i++)
				{
					IContributionItem item = items[i];
					if (item.isVisible())
					{
						makeEntry(item);
					}
				}
			}
			menuParent.layout(true, true);
		}
	}

	private void disposeToolItems()
	{
		for(int i = 0; i < toolItemList.size(); i++)
		{
			ToolItem item = (ToolItem)toolItemList.get(i);
			if (!item.isDisposed())
			{
				Object data = item.getData();
				if (data != null && data instanceof Menu)
				{
					Menu menu = (Menu)data;
					if (!menu.isDisposed())
					{
						menu.dispose();
					}
				}
				item.dispose();
			}
		}
	}

	@SuppressWarnings("unchecked")
	private void makeEntry(final IContributionItem item)
	{
		IContributionItem tempItem = null;
		if (item instanceof SubContributionItem)
		{
			SubContributionItem subItem = (SubContributionItem)item;
			tempItem = subItem.getInnerItem();
		}
		else if (item instanceof MenuManager)
		{
			tempItem = item;
		}
		if (tempItem != null && tempItem instanceof MenuManager)
		{
			final MenuManager manager = (MenuManager)tempItem;
			int style = extractStyle(manager);
			final ToolItem toolItem = new ToolItem(toolbar, style);
			toolItem.setText(manager.getMenuText());
			toolItem.setData(WidgetUtil.CUSTOM_VARIANT, MENU_BAR_VARIANT);
			createMenu(manager, toolItem);
			// needed to clear all controls in case of an update
			toolItemList.add(toolItem);
		}
	}

	private int extractStyle(final MenuManager manager)
	{
		int style = SWT.NONE;
		if (manager.getItems() != null && manager.getItems().length > 0)
		{
			style = SWT.DROP_DOWN;
		}
		return style;
	}

	private void createMenu(final MenuManager manager, final ToolItem toolItem)
	{
		final Menu menu = new Menu(menuParent);
		toolItem.setData(menu);
		menu.setData(WidgetUtil.CUSTOM_VARIANT, MENU_BAR_VARIANT);
		toolItem.addSelectionListener(new SelectionAdapter()
		{
			/**
			 * 
			 */
			private static final long serialVersionUID = 1L;

			public void widgetSelected(final SelectionEvent e)
			{
				// cleanup the menu
				MenuItem[] menuItems = menu.getItems();
				for(int i = 0; i < menuItems.length; i++)
				{
					menuItems[i].dispose();
				}
				hookMenuToToolItem(manager, menu);
				// set the menu position
				Display display = toolItem.getDisplay();
				Rectangle bounds = toolItem.getBounds();
				int leftIndent = bounds.x;
				int topIndent = bounds.y + bounds.height;
				Point indent = new Point(leftIndent, topIndent);
				Point menuLocation = display.map(toolbar, toolbar.getShell(), indent);
				menu.setLocation(menuLocation);
				// style the menuitems and show the menu
				menu.setData(WidgetUtil.CUSTOM_VARIANT, MENU_BAR_VARIANT);
				styleMenuItems(menu);
				menu.setVisible(true);
			}

			private void hookMenuToToolItem(final MenuManager manager, final Menu menu)
			{
				IContributionItem[] contribItems = manager.getItems();
				if (contribItems != null && contribItems.length > 0)
				{
					for(int i = 0; i < contribItems.length; i++)
					{
						if (i > 0 || !(contribItems[i] instanceof Separator))
						{
							contribItems[i].fill(menu, -1);
						}
					}
				}
			};

		});
	}

	private void styleMenuItems(final Menu menu)
	{
		MenuItem[] items = menu.getItems();
		if (items != null && items.length > 0)
		{
			for(int i = 0; i < items.length; i++)
			{
				items[i].setData(WidgetUtil.CUSTOM_VARIANT, MENU_BAR_VARIANT);
				Menu subMenu = items[i].getMenu();
				if (subMenu != null)
				{
					subMenu.setData(WidgetUtil.CUSTOM_VARIANT, MENU_BAR_VARIANT);
					styleMenuItems(subMenu);
				}
			}
		}
	}

	public ToolBar getMenuToolBar()
	{
		return toolbar;
	}

}
