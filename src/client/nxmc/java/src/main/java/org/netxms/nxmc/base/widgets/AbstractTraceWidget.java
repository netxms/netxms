/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
package org.netxms.nxmc.base.widgets;

import java.util.LinkedList;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.TableColumn;
import org.eclipse.swt.widgets.TableItem;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.widgets.helpers.AbstractTraceViewFilter;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Abstract widget for trace views
 */
public abstract class AbstractTraceWidget extends Composite
{
   private final I18n i18n = LocalizationHelper.getI18n(AbstractTraceWidget.class);

	private static final int MAX_ELEMENTS = 500;

   protected View view;
   protected TableViewer viewer;
   protected AbstractTraceViewFilter filter = null;

	private LinkedList<Object> data = new LinkedList<Object>();
	private boolean paused = false;
   private long lastUpdated = 0;
   private boolean updateScheduled = false;
	private Action actionPause;
	private Action actionCopy;

	/**
	 * @param parent
	 * @param style
	 * @param viewPart
	 */
   protected AbstractTraceWidget(Composite parent, int style, View view)
	{
		super(parent, style);

      setupLocalization();

      this.view = view;

      setLayout(new FillLayout());

		viewer = new TableViewer(this, SWT.FULL_SELECTION | SWT.MULTI);
		viewer.getTable().setHeaderVisible(true);
		viewer.setContentProvider(new ArrayContentProvider());
		setupViewer(viewer);
      WidgetHelper.restoreColumnSettings(viewer.getTable(), getConfigPrefix());
      viewer.getTable().addDisposeListener((e) -> WidgetHelper.saveColumnSettings(viewer.getTable(), getConfigPrefix()));

      filter = createFilter();
      viewer.addFilter(filter);

      viewer.setInput(data);

		createActions();
		createPopupMenu();

      addDisposeListener((e) -> saveConfig());
	}

   /**
    * Clone widget state on pop out and pin
    * 
    * @param traceWidget origin
    */
   public void clone(AbstractTraceWidget origin)
   {
      data.addAll(origin.data);
   }

	/**
	 * Create actions
	 */
	protected void createActions()
	{
      actionPause = new Action(i18n.tr("&Pause"), Action.AS_CHECK_BOX) {
			@Override
			public void run()
			{
				setPaused(actionPause.isChecked());
			}
		};
      actionPause.setImageDescriptor(ResourceManager.getImageDescriptor("icons/pause.png"));

      actionCopy = new Action(i18n.tr("&Copy to clipboard"), SharedIcons.COPY) {
			@Override
			public void run()
			{
				copySelectionToClipboard();
			}
      };
	}

	/**
	 * Create viewer's popup menu
	 */
	private void createPopupMenu()
	{
		// Create menu manager
		MenuManager menuMgr = new MenuManager();
		menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener((m) -> fillContextMenu(m));

		// Create menu
		Menu menu = menuMgr.createContextMenu(viewer.getControl());
		viewer.getControl().setMenu(menu);
	}

	/**
	 * Fill context menu
	 * 
	 * @param manager Menu manager
	 */
	protected void fillContextMenu(final IMenuManager manager)
	{
		manager.add(actionCopy);
		manager.add(new Separator());
	}

   /**
    * @see org.eclipse.swt.widgets.Composite#setFocus()
    */
	@Override
	public boolean setFocus()
	{
		return viewer.getControl().setFocus();
	}

   /**
    * Setup localization object. It is called early during widget creation and is intended for subclasses to create their i18n
    * objects if needed.
    */
   protected abstract void setupLocalization();

	/**
	 * Setup table vewer
	 * 
	 * @param viewer
	 */
	protected abstract void setupViewer(TableViewer viewer);

   /**
    * Create filter for viewer.
    *
    * @return filter for viewer
    */
   protected abstract AbstractTraceViewFilter createFilter();

	/**
	 * Add column to viewer
	 * 
	 * @param name
	 * @param width
	 */
	protected void addColumn(String name, int width)
	{
		final TableColumn tc = new TableColumn(viewer.getTable(), SWT.LEFT);
		tc.setText(name);
		tc.setWidth(width);
	}

	/**
	 * Add new element to the viewer
	 * 
	 * @param element
	 */
	protected void addElement(Object element)
	{
      if (isDisposed())
         return;

		if (data.size() == MAX_ELEMENTS)
			data.removeLast();

		data.addFirst(element);
		if (!paused)
		{
		   long now = System.currentTimeMillis();
		   if (now - lastUpdated > 200)
		   {
            viewer.refresh();
		      lastUpdated = now;
		   }
		   else if (!updateScheduled)
		   {
		      updateScheduled = true;
		      getDisplay().timerExec(200, new Runnable() {
               @Override
               public void run()
               {
                  viewer.refresh();
                  lastUpdated = System.currentTimeMillis();
                  updateScheduled = false;
               }
            });
		   }
		}
	}

	/**
	 * Clear viewer
	 */
	public void clear()
	{
		data.clear();
      viewer.refresh();
	}

	/**
    * Refresh viewer
    */
   public void refresh()
   {
      viewer.refresh();
   }

   /**
    * @return the paused
    */
	protected boolean isPaused()
	{
		return paused;
	}

	/**
	 * @param paused the paused to set
	 */
	protected void setPaused(boolean paused)
	{
		this.paused = paused;
		if (!paused)
         viewer.refresh();
	}
	
	/**
	 * Get configuration prefix for saving viewer config
	 * 
	 * @return
	 */
	protected abstract String getConfigPrefix();

	/**
	 * Save config
	 */
	protected void saveConfig()
	{
	}

	/**
	 * @param filter the filter to set
	 */
	protected void setFilter(AbstractTraceViewFilter filter)
	{
		if (this.filter != null)
			viewer.removeFilter(this.filter);

		this.filter = filter;
		viewer.addFilter(filter);
	}

   /**
    * Get viewer filter.
    *
    * @return viewer filter
    */
   public AbstractTraceViewFilter getFilter()
   {
      return filter;
   }

   /**
    * Set root object ID.
    *
    * @param objectId new root object ID or 0
    */
   public void setRootObject(long objectId)
   {
      filter.setRootObject(objectId);
      viewer.refresh();
   }

	/**
	 * @param runnable
	 */
	protected void runInUIThread(final Runnable runnable)
	{
		getDisplay().asyncExec(runnable);
	}

	/**
	 * Copy selection in the list to clipboard
	 */
	private void copySelectionToClipboard()
	{
		TableItem[] selection = viewer.getTable().getSelection();
		if (selection.length > 0)
		{
			StringBuilder sb = new StringBuilder();
         final String newLine = WidgetHelper.getNewLineCharacters();
			for(int i = 0; i < selection.length; i++)
			{
				if (i > 0)
					sb.append(newLine);
				sb.append(selection[i].getText(0));
				for(int j = 1; j < viewer.getTable().getColumnCount(); j++)
				{
					sb.append('\t');
					sb.append(selection[i].getText(j));
				}
			}
			WidgetHelper.copyToClipboard(sb.toString());
		}
	}

	/**
	 * @return the actionPause
	 */
	public Action getActionPause()
	{
		return actionPause;
	}

	/**
	 * @return the actionCopy
	 */
	public Action getActionCopy()
	{
		return actionCopy;
	}

	/**
    * Get associated view
    * 
    * @return
    */
   protected View getView()
	{
      return view;
	}

   /**
    * @return the viewer
    */
   public TableViewer getViewer()
   {
      return viewer;
   }
   
   /**
    * Get selection provider
    * 
    * @return
    */
   public ISelectionProvider getSelectionProvider()
   {
      return viewer;
   }
}
