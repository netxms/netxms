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
package org.netxms.ui.eclipse.objecttools.views;

import java.io.IOException;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.commands.ActionHandler;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.contexts.IContextService;
import org.eclipse.ui.handlers.IHandlerService;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.TextOutputAdapter;
import org.netxms.client.TextOutputListener;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.objecttools.Activator;
import org.netxms.ui.eclipse.objecttools.Messages;
import org.netxms.ui.eclipse.objecttools.api.ObjectToolsCache;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.widgets.TextConsole;
import org.netxms.ui.eclipse.widgets.TextConsole.IOConsoleOutputStream;

/**
 * Abstract view to display command execution results
 */
public abstract class AbstractCommandResults extends ViewPart
{
	protected long nodeId;
	protected long toolId;
   protected int remotePort;
	protected TextConsole console;
   protected long streamId = 0;
   protected NXCSession session;

   private IOConsoleOutputStream out;
   private TextOutputListener outputListener;

	private Action actionClear;
	private Action actionScrollLock;
	private Action actionCopy;
	private Action actionSelectAll;

	/**
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
	 */
   @Override
   public void init(IViewSite site) throws PartInitException
   {
      super.init(site);

      // Secondary ID must be in form toolId&nodeId or toolId&nodeId&remotePort
		String[] parts = site.getSecondaryId().split("&"); //$NON-NLS-1$
      if (parts.length < 2)
			throw new PartInitException("Internal error"); //$NON-NLS-1$

      session = ConsoleSharedData.getSession();

      outputListener = new TextOutputAdapter() {
         @Override
         public void messageReceived(String text)
         {
            try
            {
               if (out != null)
                  out.write(text.replace("\r", "")); //$NON-NLS-1$ //$NON-NLS-2$
            }
            catch(IOException e)
            {
            }
         }

         @Override
         public void setStreamId(long streamId)
         {
            AbstractCommandResults.this.streamId = streamId;
         }
      };

		try
		{
			nodeId = Long.parseLong(parts[0]);
			toolId = Long.parseLong(parts[1]);
         remotePort = (parts.length > 2) ? Integer.parseInt(parts[2]) : 0;

			AbstractObject object = session.findObjectById(nodeId);
			setPartName(object.getObjectName() + " - " + ObjectToolsCache.getInstance().findTool(toolId).getDisplayName()); //$NON-NLS-1$
		}
		catch(Exception e)
		{
			throw new PartInitException("Unexpected initialization failure", e); //$NON-NLS-1$
		}
   }

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createPartControl(Composite parent)
	{
		console = new TextConsole(parent, SWT.NONE);
		console.addSelectionChangedListener(new ISelectionChangedListener() {
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
				actionCopy.setEnabled(console.canCopy());
			}
		});

		createActions();
		contributeToActionBars();
		createPopupMenu();

		activateContext();
	}

	/**
	 * Activate context
	 */
	private void activateContext()
	{
		IContextService contextService = (IContextService)getSite().getService(IContextService.class);
		if (contextService != null)
		{
			contextService.activateContext("org.netxms.ui.eclipse.objecttools.context.ObjectTools"); //$NON-NLS-1$
		}
	}

	/**
	 * Create actions
	 */
	protected void createActions()
	{
		final IHandlerService handlerService = (IHandlerService)getSite().getService(IHandlerService.class);

		actionClear = new Action(Messages.get().LocalCommandResults_ClearConsole, SharedIcons.CLEAR_LOG) {
			@Override
			public void run()
			{
				console.clear();
			}
		};
		actionClear.setActionDefinitionId("org.netxms.ui.eclipse.objecttools.commands.clear_output"); //$NON-NLS-1$
      handlerService.activateHandler(actionClear.getActionDefinitionId(), new ActionHandler(actionClear));

		actionScrollLock = new Action(Messages.get().LocalCommandResults_ScrollLock, Action.AS_CHECK_BOX) {
			@Override
			public void run()
			{
			   console.setAutoScroll(!actionScrollLock.isChecked());
			}
		};
		actionScrollLock.setImageDescriptor(Activator.getImageDescriptor("icons/scroll_lock.gif")); //$NON-NLS-1$
		actionScrollLock.setChecked(false);
		actionScrollLock.setActionDefinitionId("org.netxms.ui.eclipse.objecttools.commands.scroll_lock"); //$NON-NLS-1$
      handlerService.activateHandler(actionScrollLock.getActionDefinitionId(), new ActionHandler(actionScrollLock));

		actionCopy = new Action(Messages.get().LocalCommandResults_Copy) {
			@Override
			public void run()
			{
			   console.copy();
			}
		};
		actionCopy.setEnabled(false);
      actionCopy.setActionDefinitionId("org.netxms.ui.eclipse.objecttools.commands.copy"); //$NON-NLS-1$
		handlerService.activateHandler(actionCopy.getActionDefinitionId(), new ActionHandler(actionCopy));

		actionSelectAll = new Action(Messages.get().LocalCommandResults_SelectAll) {
			@Override
			public void run()
			{
			   console.selectAll();
			}
		};
      actionSelectAll.setActionDefinitionId("org.netxms.ui.eclipse.objecttools.commands.select_all"); //$NON-NLS-1$
		handlerService.activateHandler(actionSelectAll.getActionDefinitionId(), new ActionHandler(actionSelectAll));
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
	 * @param manager Menu manager for pull-down menu
	 */
	protected void fillLocalPullDown(IMenuManager manager)
	{
		manager.add(actionClear);
		manager.add(actionScrollLock);
		manager.add(new Separator());
		manager.add(actionSelectAll);
		manager.add(actionCopy);
	}

	/**
	 * Fill local tool bar
	 * 
	 * @param manager Menu manager for local toolbar
	 */
	protected void fillLocalToolBar(IToolBarManager manager)
	{
		manager.add(actionClear);
		manager.add(actionScrollLock);
	}

	/**
	 * Create pop-up menu
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
		Menu menu = menuMgr.createContextMenu(console);
		console.setMenu(menu);
	}

	/**
	 * Fill context menu
	 * 
	 * @param mgr Menu manager
	 */
	protected void fillContextMenu(final IMenuManager manager)
	{
		manager.add(actionClear);
		manager.add(actionScrollLock);
		manager.add(new Separator());
		manager.add(actionSelectAll);
		manager.add(actionCopy);
	}

	/**
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		console.setFocus();
	}	

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#dispose()
    */
   @Override
   public void dispose()
   {
      closeOutputStream();
      super.dispose();
   }

   /**
    * Get output listener
    *
    * @return output listener
    */
   protected TextOutputListener getOutputListener()
   {
      return outputListener;
   }

   /**
    * Open new output stream (will close existing one if any)
    */
   protected void createOutputStream()
   {
      closeOutputStream();
      out = console.newOutputStream();
   }

   /**
    * Close output stream
    */
   protected void closeOutputStream()
   {
      if (out == null)
         return;

      try
      {
         out.close();
      }
      catch(IOException e)
      {
      }
      out = null;
   }

   /**
    * Write text to output stream.
    *
    * @param text text to write
    */
   protected void writeToOutputStream(String text)
   {
      if (out == null)
         return;

      try
      {
         out.write(text);
      }
      catch(IOException e)
      {
      }
   }
}
