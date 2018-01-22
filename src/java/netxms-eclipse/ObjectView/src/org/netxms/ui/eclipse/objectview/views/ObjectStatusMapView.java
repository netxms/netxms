/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2018 Raden Solutions
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
package org.netxms.ui.eclipse.objectview.views;

import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.objectview.Activator;
import org.netxms.ui.eclipse.objectview.Messages;
import org.netxms.ui.eclipse.objectview.widgets.AbstractObjectStatusMap;
import org.netxms.ui.eclipse.objectview.widgets.ObjectStatusMap;
import org.netxms.ui.eclipse.objectview.widgets.ObjectStatusMapRadial;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Show object status map for given object
 */
public class ObjectStatusMapView extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.objectview.views.ObjectStatusMapView"; //$NON-NLS-1$
	
	private static final String SETTINGS_DISPLAY_MODE = ID + ".DisplayMode"; 
   private static final String SETTINGS_SHOW_FILTER = ID + ".ShowFilter"; 

	private long rootObjectId;
	private AbstractObjectStatusMap map;
	private Composite clientArea;
	private int displayOption = 1;
   private boolean showFilter = true;
	private Action actionRefresh;
   private Action actionFlatView;
   private Action actionGroupView;
   private Action actionRadialView;
	private Action actionShowFilter;
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
	 */
	@Override
	public void init(IViewSite site) throws PartInitException
	{
		super.init(site);
		
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		rootObjectId = Long.parseLong(site.getSecondaryId());
		final AbstractObject object = session.findObjectById(rootObjectId);
		setPartName(String.format(Messages.get().ObjectStatusMapView_PartName, (object != null) ? object.getObjectName() : ("[" + rootObjectId + "]"))); //$NON-NLS-1$ //$NON-NLS-2$		
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
	   clientArea = parent;
	   
      final IDialogSettings settings = Activator.getDefault().getDialogSettings();
      displayOption = (settings.get(SETTINGS_DISPLAY_MODE) != null) ? settings.getInt(SETTINGS_DISPLAY_MODE) : 1;
      showFilter = (settings.get(SETTINGS_SHOW_FILTER) != null) ? settings.getBoolean(SETTINGS_SHOW_FILTER) : true;
      
      if (displayOption == 2)
      {
         map = new ObjectStatusMapRadial(this, parent, SWT.NONE, true);
      }
      else
      {
         map = new ObjectStatusMap(this, parent, SWT.NONE, true);
         ((ObjectStatusMap)map).setGroupObjects(displayOption == 1);
      }
      
		map.setRootObject(rootObjectId);
		map.enableFilter(showFilter);
				
		map.setFilterCloseAction(new Action() {
         @Override
         public void run()
         {
            showFilter = false;
            actionShowFilter.setChecked(false);
            map.enableFilter(false);
         }
      });

		createActions();
		contributeToActionBars();
		
		getSite().setSelectionProvider((ISelectionProvider)map);
	}
	
	

	/* (non-Javadoc)
    * @see org.eclipse.ui.part.WorkbenchPart#dispose()
    */
   @Override
   public void dispose()
   {
      final IDialogSettings settings = Activator.getDefault().getDialogSettings();
      
      if (actionFlatView.isChecked())
         settings.put(SETTINGS_DISPLAY_MODE, 0);
      else if (actionGroupView.isChecked())
         settings.put(SETTINGS_DISPLAY_MODE, 1);
      else if (actionRadialView.isChecked())
         settings.put(SETTINGS_DISPLAY_MODE, 2);
      
      settings.put(SETTINGS_SHOW_FILTER, showFilter);
      
      super.dispose();
   }

   /**
	 * Create actions
	 */
	private void createActions()
	{
		actionRefresh = new RefreshAction(this) {
			@Override
			public void run()
			{
				map.refresh();
			}
		};
      
		actionFlatView = new Action("&Flat view", Action.AS_RADIO_BUTTON) {
         @Override
         public void run()
         {
            ((ObjectStatusMap)map).setGroupObjects(false);
            map.refresh();
         }
      };
      actionFlatView.setChecked(displayOption == 0);
      actionFlatView.setImageDescriptor(Activator.getImageDescriptor("icons/not_grouped_nodes.png"));
		
		actionGroupView = new Action("&Group view", Action.AS_RADIO_BUTTON) {
			@Override
			public void run()
			{
	         ((ObjectStatusMap)map).setGroupObjects(true);
				map.refresh();
			}
		};
		actionGroupView.setChecked(displayOption == 1);
		actionGroupView.setImageDescriptor(Activator.getImageDescriptor("icons/grouped_nodes.png"));
      
      actionRadialView = new Action("&Radial view", Action.AS_RADIO_BUTTON) {
         @Override
         public void run()
         {
            redraw(actionRadialView.isChecked());
         }
      };
      actionRadialView.setChecked(displayOption == 2);
      actionRadialView.setImageDescriptor(Activator.getImageDescriptor("icons/radial.png"));
      
      actionShowFilter = new Action(Messages.get().ObjectStatusMapView_ActionShowFilter, Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            showFilter = actionShowFilter.isChecked();
            map.enableFilter(showFilter);
         }
      };
      actionShowFilter.setChecked(showFilter);
	}

	/**
	 * Redraw status view 
	 * 
	 * @param radial true if should be redrawn in radial way
	 */
   private void redraw(boolean radial)
   {
      ((Composite)map).dispose();
      
      if (radial)
         map = new ObjectStatusMapRadial(this, clientArea, SWT.NONE, true);
      else
         map = new ObjectStatusMap(this, clientArea, SWT.NONE, true);  

      map.setRootObject(rootObjectId);
      map.enableFilter(showFilter);
            
      map.setFilterCloseAction(new Action() {
         @Override
         public void run()
         {
            showFilter = false;
            actionShowFilter.setChecked(false);
            map.enableFilter(false);
         }
      });

      ((Composite)map).layout();
      ((Composite)map).getParent().layout();
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
	private void fillLocalPullDown(IMenuManager manager)
	{
      manager.add(actionShowFilter);
      manager.add(new Separator());
      manager.add(actionFlatView);
		manager.add(actionGroupView);
      manager.add(actionRadialView);
		manager.add(new Separator());
		manager.add(actionRefresh);
	}

	/**
	 * Fill local tool bar
	 * 
	 * @param manager
	 *           Menu manager for local toolbar
	 */
	private void fillLocalToolBar(IToolBarManager manager)
	{
      manager.add(actionFlatView);
      manager.add(actionGroupView);
      manager.add(actionRadialView);
      manager.add(new Separator());
      manager.add(actionRefresh);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
	   ((Composite)map).setFocus();
	}
}
