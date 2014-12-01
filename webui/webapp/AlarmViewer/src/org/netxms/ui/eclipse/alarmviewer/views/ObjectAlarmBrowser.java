/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.alarmviewer.views;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.actions.ExportToCsvAction;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.alarmviewer.Activator;
import org.netxms.ui.eclipse.alarmviewer.Messages;
import org.netxms.ui.eclipse.alarmviewer.widgets.AlarmList;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.widgets.CompositeWithMessageBar;

/**
 * Alarm browser view - alarms for specific objects. List of object IDs is passed as a '&' separated secondaryId
 */
public class ObjectAlarmBrowser extends ViewPart
{
   public static final String ID = "org.netxms.ui.eclipse.alarmviewer.views.ObjectAlarmBrowser"; //$NON-NLS-1$

   private CompositeWithMessageBar content;
   private AlarmList alarmView;
   private Action actionRefresh;
   private Action actionExportToCsv;
   private List<Long> objects = new ArrayList<Long>(1);

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
	 */
   @Override
	public void init(IViewSite site) throws PartInitException 
   {
      super.init(site);
		for(String id : site.getSecondaryId().split("&"))  //$NON-NLS-1$
		{
		   try
		   {
		      objects.add(Long.parseLong(id));
		   }
		   catch(NumberFormatException e)
		   {
		      Activator.logError("Invalid number in AlarmViewer secondary ID", e); //$NON-NLS-1$
		   }
		}
   }

	/* (non-Javadoc)
    * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
    */
   @Override
	public void createPartControl(Composite parent) 
   {
		parent.setLayout(new FillLayout());		
		content = new CompositeWithMessageBar(parent, SWT.NONE);

      alarmView = new AlarmList(this, content.getContent(), SWT.NONE, "ObjectAlarmBrowser"); //$NON-NLS-1$
		alarmView.setRootObjects(objects);

		if (objects.size() == 1) 
		{
	      NXCSession session = (NXCSession)ConsoleSharedData.getSession();
			setPartName(String.format(Messages.get().ObjectAlarmBrowser_Title, session.getObjectName(objects.get(0))));
			//content.hideMessage();
		} 
		else
		{
			setPartName(Messages.get().ObjectAlarmBrowser_TitleMultipleObjects);
			showObjectList();
		}

      createActions();
      contributeToActionBars();

      getSite().setSelectionProvider(alarmView.getSelectionProvider());
   }
   
   /**
    * Show list of selected objects
    */
   private void showObjectList()
   {
      NXCSession session = (NXCSession)ConsoleSharedData.getSession();
      StringBuilder sb = new StringBuilder();
      for(Long id : objects)
      {
         if (sb.length() > 0)
            sb.append(", "); //$NON-NLS-1$
         sb.append(session.getObjectName(id));
      }
      content.showMessage(CompositeWithMessageBar.INFORMATION, String.format(Messages.get().ObjectAlarmBrowser_SelectedObjects, sb.toString()));
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionRefresh = new RefreshAction() {
         @Override
         public void run()
         {
            alarmView.refresh();
            showObjectList();
         }
      };

      actionExportToCsv = new ExportToCsvAction(this, alarmView.getViewer(), false);
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
   private void fillLocalPullDown(IMenuManager manager)
   {
      manager.add(actionExportToCsv);
      manager.add(new Separator());
      manager.add(actionRefresh);
   }

   /**
    * Fill local tool bar
    * 
    * @param manager Menu manager for local toolbar
    */
   private void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionExportToCsv);
      manager.add(actionRefresh);
   }

	/* (non-Javadoc)
    * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
    */
   @Override
   public void setFocus()
   {
      alarmView.setFocus();
   }

	/* (non-Javadoc)
    * @see org.eclipse.ui.part.WorkbenchPart#dispose()
    */
   @Override
   public void dispose()
   {
      alarmView.dispose();
      super.dispose();
   }
}
