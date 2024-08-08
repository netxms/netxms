/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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
package org.netxms.nxmc.modules.objects.views.elements;

import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.Table;
import org.eclipse.swt.widgets.TableColumn;
import org.eclipse.swt.widgets.TableItem;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Generic table element
 */
public abstract class TableElement extends OverviewPageElement
{
   private final I18n i18n = LocalizationHelper.getI18n(TableElement.class);

	private Table table;
	private Action actionCopy;
	private Action actionCopyName;
	private Action actionCopyValue;

	/**
    * Create element.
    *
    * @param parent parent composite
    * @param anchor anchor element
    * @param objectView owning view
    */
   public TableElement(Composite parent, OverviewPageElement anchor, ObjectView objectView)
	{
      super(parent, anchor, objectView);
	}

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.OverviewPageElement#createClientArea(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createClientArea(Composite parent)
	{
		table = new Table(parent, SWT.FULL_SELECTION | SWT.HIDE_SELECTION | SWT.SINGLE | SWT.H_SCROLL);
		setupTable();
		createActions();
		createPopupMenu();
		fillTableInternal();
		return table;
	}

	/**
	 * Setup table
	 */
   protected void setupTable()
	{
		TableColumn tc = new TableColumn(table, SWT.LEFT);
      tc.setText(i18n.tr("Name"));
		tc = new TableColumn(table, SWT.LEFT);
      tc.setText(i18n.tr("Value"));
		
		table.setHeaderVisible(false);
		table.setLinesVisible(false);
	}

	/**
	 * Create actions required for popup menu
	 */
	protected void createActions()
	{
      actionCopy = new Action(i18n.tr("&Copy to clipboard")) {
			@Override
			public void run()
			{
				int index = table.getSelectionIndex();
				if (index >= 0)
				{
					final TableItem item = table.getItem(index);
               WidgetHelper.copyToClipboard(item.getText(0) + "=" + item.getText(1));
				}
			}
		};

      actionCopyName = new Action(i18n.tr("Copy &name to clipboard")) {
			@Override
			public void run()
			{
				int index = table.getSelectionIndex();
				if (index >= 0)
				{
					final TableItem item = table.getItem(index);
					WidgetHelper.copyToClipboard(item.getText(0));
				}
			}
		};

      actionCopyValue = new Action(i18n.tr("Copy &value to clipboard")) {
			@Override
			public void run()
			{
				int index = table.getSelectionIndex();
				if (index >= 0)
				{
					final TableItem item = table.getItem(index);
					WidgetHelper.copyToClipboard(item.getText(1));
				}
			}
		};
	}

	/**
	 * Create popup menu
	 */
	private void createPopupMenu()
	{
		MenuManager menuMgr = new MenuManager();
		menuMgr.setRemoveAllWhenShown(true);
		menuMgr.addMenuListener(new IMenuListener() {
			public void menuAboutToShow(IMenuManager mgr)
			{
				fillContextMenu(mgr);
			}
		});

		Menu menu = menuMgr.createContextMenu(table);
		table.setMenu(menu);
	}
	
	/**
	 * Fill item's context menu
	 * 
	 * @param manager menu manager
	 */
	protected void fillContextMenu(IMenuManager manager)
	{
		manager.add(actionCopy);
		manager.add(actionCopyName);
		manager.add(actionCopyValue);
	}

	/**
	 * Fill table
	 */
	abstract protected void fillTable();
	
	/**
	 * Fill table - internal wrapper
	 */
	private void fillTableInternal()
	{
		if (getObject() == null)
			return;
		
		fillTable();
		table.getColumn(0).pack();
		table.getColumn(1).pack();
	}
	
	/**
	 * Add attribute/value pair to list
	 * @param attr
	 * @param value
	 * @param allowEmpty if set to false, empty values will not be added
	 */
	protected void addPair(String attr, String value, boolean allowEmpty)
	{
		if (!allowEmpty && ((value == null) || value.isEmpty()))
			return;
		TableItem item = new TableItem(table, SWT.NONE);
		item.setText(0, attr);
		item.setText(1, (value != null) ? value : ""); //$NON-NLS-1$
	}

	/**
	 * Add attribute/value pair to list
	 * @param attr
	 * @param value
	 */
	protected void addPair(String attr, String value)
	{
		addPair(attr, value, true);
	}

   /**
    * @see org.netxms.ui.eclipse.objectview.objecttabs.elements.OverviewPageElement#onObjectChange()
    */
	@Override
	protected void onObjectChange()
	{
      if (!isDisposed())
      {
         table.removeAll();
         fillTableInternal();
      }
	}
}
