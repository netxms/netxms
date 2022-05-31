/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.dashboard.widgets;

import org.eclipse.core.runtime.Platform;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.dashboard.Messages;

/**
 * Edit pane widget for dashboard elements
 */
public class EditPaneWidget extends Canvas
{
   private static final RGB BACKGROUND_COLOR = new RGB(0, 0, 127);

	private DashboardControl dbc;
	private DashboardElement element;
	private Action actionEdit;
	private Action actionEditXml;
	private Action actionDelete;

	/**
	 * @param parent
	 */
	public EditPaneWidget(Composite parent, DashboardControl dbc, DashboardElement element)
	{
		super(parent, SWT.TRANSPARENT | SWT.NO_BACKGROUND);
		this.dbc = dbc;
		this.element = element;

      final Color color;
      if (Platform.getOS().equals(Platform.OS_WIN32))
      {
         color = new Color(getDisplay(), BACKGROUND_COLOR);
         addPaintListener(new PaintListener() {
            @Override
            public void paintControl(PaintEvent e)
            {
               final GC gc = e.gc;
               final Point size = getSize();
               gc.setBackground(color);
               gc.setAlpha(20);
               gc.fillRectangle(0, 0, size.x, size.y);
            }
         });

      }
      else
      {
         color = new Color(getDisplay(), BACKGROUND_COLOR, 20);
         setBackground(color);
      }
      addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            color.dispose();
         }
      });

		createActions();
		createPopupMenu();
	}

	/**
	 * Create actions
	 */
	private void createActions()
	{
		actionEdit = new Action(Messages.get().EditPaneWidget_Edit) {
			@Override
			public void run()
			{
				dbc.editElement(element);
			}
		};
		actionEdit.setImageDescriptor(SharedIcons.EDIT);
		
		actionEditXml = new Action(Messages.get().EditPaneWidget_EditXML) {
			@Override
			public void run()
			{
				dbc.editElementXml(element);
			}
		};
		actionEditXml.setImageDescriptor(SharedIcons.XML);
		
		actionDelete = new Action(Messages.get().EditPaneWidget_Delete) {
			@Override
			public void run()
			{
				dbc.deleteElement(element);
			}
		};
		actionDelete.setImageDescriptor(SharedIcons.DELETE_OBJECT);
	}

	/**
	 * Create pop-up menu for alarm list
	 */
	private void createPopupMenu()
	{
		// Create menu manager.
		MenuManager menuMgr = new MenuManager();
		menuMgr.setRemoveAllWhenShown(true);
		menuMgr.addMenuListener(new IMenuListener() {
			public void menuAboutToShow(IMenuManager mgr)
			{
				fillContextMenu(mgr);
			}
		});

		// Create menu.
		Menu menu = menuMgr.createContextMenu(this);
		setMenu(menu);
	}

	/**
	 * Fill context menu
	 * @param mgr Menu manager
	 */
	protected void fillContextMenu(IMenuManager mgr)
	{
		mgr.add(actionEdit);
		mgr.add(actionEditXml);
		mgr.add(actionDelete);
	}
}
