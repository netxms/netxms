/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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

import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Chassis;
import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.Collector;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.Rack;
import org.netxms.client.objects.ServiceRoot;
import org.netxms.client.objects.WirelessDomain;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.base.views.AbstractViewerFilter;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.widgets.AbstractObjectStatusMap;
import org.netxms.nxmc.modules.objects.widgets.FlatObjectStatusMap;
import org.netxms.nxmc.modules.objects.widgets.RadialObjectStatusMap;
import org.netxms.nxmc.resources.ResourceManager;
import org.xnap.commons.i18n.I18n;

/**
 * Show object status map for given object
 */
public class StatusMapView extends ObjectView
{
   private final I18n i18n = LocalizationHelper.getI18n(StatusMapView.class);

	private AbstractObjectStatusMap map;
	private Composite clientArea;
	private int displayOption = 1;
   private boolean fitToScreen = true;
   private Action actionFlatView;
   private Action actionGroupView;
   private Action actionRadialView;
   private Action actionFitToScreen;

   /**
    * Constructor.
    */
   public StatusMapView()
   {
      super(LocalizationHelper.getI18n(StatusMapView.class).tr("Status Map"), ResourceManager.getImageDescriptor("icons/object-views/status-map.png"), "objects.status-map", true);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 40;
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
	@Override
   public void createContent(Composite parent)
	{
	   clientArea = parent;

      final PreferenceStore settings = PreferenceStore.getInstance();
      displayOption = settings.getAsInteger("StatusMap.DisplayMode", 1);
      fitToScreen = settings.getAsBoolean("StatusMap.FitToScreen", true);

      if (displayOption == 2)
      {
         map = new RadialObjectStatusMap(this, parent, SWT.NONE);
      }
      else
      {
         map = new FlatObjectStatusMap(this, parent, SWT.NONE);
         ((FlatObjectStatusMap)map).setGroupObjects(displayOption == 1);
      }

      map.setFitToScreen(fitToScreen);

		createActions();

      setFilterClient(map, new AbstractViewerFilter() {
         @Override
         public void setFilterString(String string)
         {
            map.setTextFilter(string);
            map.refresh();
         }
      });
	}

   /**
    * @see org.netxms.nxmc.base.views.View#dispose()
    */
   @Override
   public void dispose()
   {
      final PreferenceStore settings = PreferenceStore.getInstance();
      
      if (actionFlatView.isChecked())
         settings.set("StatusMap.DisplayMode", 0);
      else if (actionGroupView.isChecked())
         settings.set("StatusMap.DisplayMode", 1);
      else if (actionRadialView.isChecked())
         settings.set("StatusMap.DisplayMode", 2);
      
      settings.set("StatusMap.FitToScreen", actionFitToScreen.isChecked());

      super.dispose();
   }

   /**
	 * Create actions
	 */
	private void createActions()
	{
      actionFlatView = new Action(i18n.tr("&Flat view"), Action.AS_RADIO_BUTTON) {
         @Override
         public void run()
         {
            if (map instanceof FlatObjectStatusMap)
            {
               if (isChecked())
               {
                  ((FlatObjectStatusMap)map).setGroupObjects(false);
                  map.refresh();
               }
            }
            else if (isChecked())
            {
               rebuild(false);
            }
         }
      };
      actionFlatView.setChecked(displayOption == 0);
      actionFlatView.setImageDescriptor(ResourceManager.getImageDescriptor("icons/status-map-flat.png"));
      addKeyBinding("M1+M2+F", actionFlatView, "StatusMap.ModeSwitchGroup");

      actionGroupView = new Action(i18n.tr("&Group view"), Action.AS_RADIO_BUTTON) {
			@Override
			public void run()
			{
			   if (map instanceof FlatObjectStatusMap)
			   {
               if (isChecked())
               {
                  ((FlatObjectStatusMap)map).setGroupObjects(true);
                  map.refresh();
               }
			   }
            else if (isChecked())
            {
               rebuild(false);
            }
			}
		};
		actionGroupView.setChecked(displayOption == 1);
      actionGroupView.setImageDescriptor(ResourceManager.getImageDescriptor("icons/status-map-grouped.png"));
      addKeyBinding("M1+M2+G", actionGroupView, "StatusMap.ModeSwitchGroup");

      actionRadialView = new Action(i18n.tr("&Radial view"), Action.AS_RADIO_BUTTON) {
         @Override
         public void run()
         {
            if (isChecked())
               rebuild(true);
         }
      };
      actionRadialView.setChecked(displayOption == 2);
      actionRadialView.setImageDescriptor(ResourceManager.getImageDescriptor("icons/status-map-sunburst.png"));
      addKeyBinding("M1+M2+R", actionRadialView, "StatusMap.ModeSwitchGroup");

      actionFitToScreen = new Action(i18n.tr("Fit to &screen"), Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            fitToScreen = actionFitToScreen.isChecked();
            map.setFitToScreen(fitToScreen);
            map.refresh();
         }
      };
      actionFitToScreen.setChecked(fitToScreen);
	}

	/**
    * Rebuild status view
    * 
    * @param radial true if should be redrawn in radial way
    */
   private void rebuild(boolean radial)
   {
      ((Composite)map).dispose();

      if (radial)
      {
         map = new RadialObjectStatusMap(this, clientArea, SWT.NONE);
      }
      else
      {
         map = new FlatObjectStatusMap(this, clientArea, SWT.NONE);
         ((FlatObjectStatusMap)map).setGroupObjects(actionGroupView.isChecked());
      }

      map.setRootObjectId(getObjectId());
      map.setFitToScreen(fitToScreen);
      map.refresh();

      clientArea.layout();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
	{
      manager.add(actionFitToScreen);
      manager.add(new Separator());
      manager.add(actionFlatView);
		manager.add(actionGroupView);
      manager.add(actionRadialView);
	}

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
	{
      manager.add(actionFlatView);
      manager.add(actionGroupView);
      manager.add(actionRadialView);
	}

   /**
    * @see org.netxms.nxmc.base.views.View#setFocus()
    */
	@Override
	public void setFocus()
	{
	   ((Composite)map).setFocus();
	}

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return (context != null) && ((context instanceof Container) || (context instanceof Collector) || (context instanceof ServiceRoot) || 
            (context instanceof Cluster) || (context instanceof Rack) || (context instanceof Chassis) || (context instanceof WirelessDomain));
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectChange(AbstractObject object)
   {
      map.setRootObjectId((object != null) ? object.getObjectId() : 0);
      map.refresh();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      map.refresh();
   }
}
