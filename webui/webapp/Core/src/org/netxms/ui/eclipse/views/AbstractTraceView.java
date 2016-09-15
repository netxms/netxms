/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2016 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.views;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.commands.ActionHandler;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.contexts.IContextService;
import org.eclipse.ui.handlers.IHandlerService;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.console.Activator;
import org.netxms.ui.eclipse.console.Messages;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.widgets.AbstractTraceWidget;

/**
 * Abstract trace view (like event trace)
 */
public abstract class AbstractTraceView extends ViewPart
{
   protected NXCSession session = ConsoleSharedData.getSession();
   
	protected AbstractTraceWidget traceWidget;
	private Action actionClear;
	protected Action actionShowFilter;
	protected boolean initShowFilter = true;
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		traceWidget = createTraceWidget(parent);
		
		createActions();
		contributeToActionBars();
		createPopupMenu();
		
		traceWidget.setFilterCloseAction(actionShowFilter);
      traceWidget.enableFilter(initShowFilter);
		
		activateContext();
	}
	
	/**
	 * @param parent
	 * @return trace widget to be used
	 */
	protected abstract AbstractTraceWidget createTraceWidget(Composite parent);
	
	/**
	 * @return
	 */
	protected AbstractTraceWidget getTraceWidget()
	{
		return traceWidget;
	}

	/**
	 * Activate context
	 */
	protected void activateContext()
	{
		IContextService contextService = (IContextService)getSite().getService(IContextService.class);
		if (contextService != null)
		{
			contextService.activateContext("org.netxms.ui.eclipse.library.context.AbstractTraceView"); //$NON-NLS-1$
		}
	}
	
	/**
	 * Create actions
	 */
	protected void createActions()
	{
	   final IHandlerService handlerService = (IHandlerService)getSite().getService(IHandlerService.class);
	   
		actionClear = new Action(Messages.get().AbstractTraceView_Clear, SharedIcons.CLEAR_LOG) {
			@Override
			public void run()
			{
				traceWidget.clear();
			}
		};
		
		actionShowFilter = new Action("Show &filter", Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            traceWidget.enableFilter(!traceWidget.isFilterEnabled());
            actionShowFilter.setChecked(traceWidget.isFilterEnabled());
         }
      };
      actionShowFilter.setImageDescriptor(SharedIcons.FILTER);
      actionShowFilter.setChecked(initShowFilter);
      actionShowFilter.setId("org.netxms.ui.eclipse.library.actions.showSnmpFilter");
      actionShowFilter.setActionDefinitionId("org.netxms.ui.eclipse.library.commands.show_filter");
      final ActionHandler showFilterHandler = new ActionHandler(actionShowFilter);
      handlerService.activateHandler(actionShowFilter.getActionDefinitionId(), showFilterHandler);
	}
	
	/**
	 * Contribute actions to action bar
	 */
	private void contributeToActionBars()
	{
		IActionBars bars = getViewSite().getActionBars();
		fillLocalPullDown(bars.getMenuManager());
		fillLocalToolBar(bars.getToolBarManager());
	}

	/**
	 * Fill local pull-down menu
	 * 
	 * @param manager
	 *           Menu manager for pull-down menu
	 */
	protected void fillLocalPullDown(IMenuManager manager)
	{
		manager.add(actionShowFilter);
		manager.add(new Separator());
		manager.add(traceWidget.getActionPause());
		manager.add(actionClear);
		//manager.add(new Separator());
		//manager.add(traceWidget.getActionCopy());
	}

	/**
	 * Fill local tool bar
	 * 
	 * @param manager
	 *           Menu manager for local toolbar
	 */
	protected void fillLocalToolBar(IToolBarManager manager)
	{
		manager.add(traceWidget.getActionPause());
		manager.add(actionClear);
	}

   /**
    * Create pop-up menu for user list
    */
   private void createPopupMenu()
   {
      // Create menu manager
      MenuManager menuMgr = new MenuManager();
      menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener(new IMenuListener() {
         public void menuAboutToShow(IMenuManager mgr)
         {
            fillContextMenu(mgr);
         }
      });

      // Create menu
      Menu menu = menuMgr.createContextMenu(traceWidget.getViewer().getControl());
      traceWidget.getViewer().getControl().setMenu(menu);

      // Register menu for extension.
      getSite().registerContextMenu(menuMgr, traceWidget.getViewer());
   }

   /**
    * Fill context menu
    * 
    * @param mgr Menu manager
    */
   protected void fillContextMenu(final IMenuManager manager)
   {   
   }
   
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		traceWidget.setFocus();
	}
	
	/**
	 * Subscribe to channel
	 * 
	 * @param channel
	 */
	protected void subscribe(final String channel)
	{
      new ConsoleJob(String.format(Messages.get().AbstractTraceView_Subscribing, channel), this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.subscribe(channel);
         }
         
         @Override
         protected String getErrorMessage()
         {
            return String.format(Messages.get().AbstractTraceView_CannotSubscribe, channel);
         }
      }.start();
	}
	
	/**
	 * Unsubscribe from channel
	 * 
	 * @param channel
	 */
	protected void unsubscribe(final String channel)
	{
      ConsoleJob job = new ConsoleJob(String.format(Messages.get().AbstractTraceView_Unsubscribing, channel), null, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.unsubscribe(channel);
         }
         
         @Override
         protected String getErrorMessage()
         {
            return String.format(Messages.get().AbstractTraceView_CannotUnsubscribe, channel);
         }
      };
      job.setUser(false);
      job.setSystem(true);
      job.start();
	}
}
