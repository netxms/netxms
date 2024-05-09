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
package org.netxms.nxmc.base.views;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.NXCSession;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.AbstractTraceWidget;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.SharedIcons;
import org.xnap.commons.i18n.I18n;

/**
 * Abstract trace view (like event trace)
 */
public abstract class AbstractTraceView extends View
{
   private final I18n i18n = LocalizationHelper.getI18n(AbstractTraceView.class);

   protected NXCSession session = Registry.getSession();
   protected AbstractTraceWidget traceWidget;

	private Action actionClear;
   private boolean subscribed = false;

   /**
    * Create trace view.
    *
    * @param name view name
    * @param image view image
    * @param baseId base ID
    */
   public AbstractTraceView(String name, ImageDescriptor image, String baseId)
   {
      super(name, image, baseId, true);
   }
   
   /**
    * @see org.netxms.nxmc.base.views.View#postClone(org.netxms.nxmc.base.views.View)
    */
   @Override
   protected void postClone(View origin)
   {
      super.postClone(origin);

      AbstractTraceView view = (AbstractTraceView)origin;
      traceWidget.clone(view.traceWidget);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
	@Override
   public void createContent(Composite parent)
	{
		traceWidget = createTraceWidget(parent);
      setFilterClient(traceWidget.getViewer(), traceWidget.getFilter());

		createActions();
		createContextMenu();
	}

	/**
    * @see org.netxms.nxmc.base.views.View#activate()
    */
   @Override
   public void activate()
   {
      super.activate();
      subscribe(getChannelName());
   }

   /**
    * @see org.netxms.nxmc.base.views.View#deactivate()
    */
   @Override
   public void deactivate()
   {
      unsubscribe(getChannelName());
      super.deactivate();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#dispose()
    */
   @Override
   public void dispose()
   {
      unsubscribe(getChannelName());
      super.dispose();
   }

   /**
    * Create trace widget.
    *
    * @param parent parent composite
    * @return trace widget to be used
    */
	protected abstract AbstractTraceWidget createTraceWidget(Composite parent);
	
   /**
    * Get channel name for subscription
    *
    * @return channel name for subscription
    */
   protected abstract String getChannelName();

	/**
	 * @return
	 */
	protected AbstractTraceWidget getTraceWidget()
	{
		return traceWidget;
	}
	
	/**
	 * Create actions
	 */
	protected void createActions()
	{
      actionClear = new Action(i18n.tr("&Clear"), SharedIcons.CLEAR_LOG) {
			@Override
			public void run()
			{
				traceWidget.clear();
			}
		};
      addKeyBinding("M1+L", actionClear);

      addKeyBinding("M1+C", traceWidget.getActionCopy());
      addKeyBinding("M1+P", traceWidget.getActionPause());
	}

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(traceWidget.getActionPause());
      manager.add(actionClear);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(traceWidget.getActionPause());
      manager.add(actionClear);
      manager.add(new Separator());
      manager.add(traceWidget.getActionCopy());
   }

   /**
    * Create pop-up menu for user list
    */
   private void createContextMenu()
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
   }

   /**
    * Fill context menu
    * 
    * @param mgr Menu manager
    */
   protected void fillContextMenu(final IMenuManager manager)
   {   
      manager.add(traceWidget.getActionCopy());
   }
   
   /**
    * @see org.netxms.nxmc.base.views.View#setFocus()
    */
	@Override
	public void setFocus()
	{
      if (!traceWidget.isDisposed())
         traceWidget.setFocus();
	}

	/**
	 * Subscribe to channel
	 * 
	 * @param channel
	 */
   protected synchronized void subscribe(final String channel)
	{
      new Job(String.format(i18n.tr("Subscribing to channel %s"), channel), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.subscribe(channel);
            subscribed = true;
         }

         @Override
         protected String getErrorMessage()
         {
            return String.format(i18n.tr("Cannot subscribe to channel %s"), channel);
         }
      }.start();
	}

	/**
	 * Unsubscribe from channel
	 * 
	 * @param channel
	 */
   protected synchronized void unsubscribe(final String channel)
	{
      if (!subscribed)
         return;

      subscribed = false;
      Job job = new Job(String.format(i18n.tr("Unsubscribing from channel %s"), channel), null) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.unsubscribe(channel);
         }

         @Override
         protected String getErrorMessage()
         {
            return String.format(i18n.tr("Cannot unsubscribe from channel %s"), channel);
         }
      };
      job.setUser(false);
      job.setSystem(true);
      job.start();
	}
}
