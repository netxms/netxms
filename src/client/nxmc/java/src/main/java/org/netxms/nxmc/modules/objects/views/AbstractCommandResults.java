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
package org.netxms.nxmc.modules.objects.views;

import java.util.List;
import java.util.Map;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.nxmc.base.widgets.TextConsole;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.ObjectContext;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.xnap.commons.i18n.I18n;

/**
 * Abstract view to display command execution results
 */
public abstract class AbstractCommandResults extends ObjectToolResults
{
   private static I18n i18n = LocalizationHelper.getI18n(AbstractCommandResults.class);

   protected String executionString;
   protected Map<String, String> inputValues;
   protected List<String> maskedFields;
   
	protected TextConsole console;

	private Action actionClear;
	private Action actionScrollLock;
	private Action actionCopy;
	private Action actionSelectAll;
	
	/**
	 * Constructor
	 * 
	 * @param image
	 * @param id
	 * @param hasFilter
	 * @param nodeId
	 * @param toolId
	 */
	public AbstractCommandResults(ObjectContext node, ObjectTool tool, final Map<String, String> inputValues, final List<String> maskedFields) 
	{
	   super(node, tool, ResourceManager.getImageDescriptor("icons/object-tools/console.png"), false);
      this.inputValues = inputValues;
      this.maskedFields = maskedFields;
      this.executionString = tool.getData();
	}
   
   /* (non-Javadoc)
    * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
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
		createPopupMenu();
		execute();
	}

	/**
	 * Create actions
	 */
	protected void createActions()
	{
		actionClear = new Action(i18n.tr("C&lear console"), SharedIcons.CLEAR_LOG) {
			@Override
			public void run()
			{
				console.clear();
			}
		};

		actionScrollLock = new Action(i18n.tr("&Scroll lock"), Action.AS_CHECK_BOX) {
			@Override
			public void run()
			{
			   console.setAutoScroll(!actionScrollLock.isChecked());
			}
		};
		actionScrollLock.setImageDescriptor(ResourceManager.getImageDescriptor("icons/scroll-lock.png")); 
		actionScrollLock.setChecked(false);

		actionCopy = new Action(i18n.tr("&Copy")) {
			@Override
			public void run()
			{
			   console.copy();
			}
		};
		actionCopy.setEnabled(false);
		
		actionSelectAll = new Action(i18n.tr("Select &all")) {
			@Override
			public void run()
			{
			   console.selectAll();
			}
		};
	}
	
	/**
	 * Execute action
	 */
	protected abstract void execute();

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolbar(org.eclipse.jface.action.ToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionClear);
      manager.add(actionScrollLock);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(org.eclipse.jface.action.MenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionClear);
      manager.add(actionScrollLock);
      manager.add(new Separator());
      manager.add(actionSelectAll);
      manager.add(actionCopy);
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
}
