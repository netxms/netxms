/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Raden Solutions
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
package org.netxms.nxmc.modules.alarms.views;

import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.base.actions.ExportToCsvAction;
import org.netxms.nxmc.base.widgets.helpers.SearchQueryContentProposalProvider;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.alarms.widgets.AlarmList;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.netxms.nxmc.resources.ResourceManager;
import org.xnap.commons.i18n.I18n;

/**
 * "Alarms" view
 */
public class AlarmsView extends ObjectView
{
   private final I18n i18n = LocalizationHelper.getI18n(AlarmsView.class);

   protected AlarmList alarmList;
   private Action actionExportToCsv;

   /**
    * Create alarm view
    */
   public AlarmsView()
   {
      super(LocalizationHelper.getI18n(AlarmsView.class).tr("Alarms"), ResourceManager.getImageDescriptor("icons/object-views/alarms.png"), "objects.alarms", true);
   }

   /**
    * Create alarm view with given name and ID.
    *
    * @param name view name
    * @param id view ID
    */
   protected AlarmsView(String name, String id)
   {
      super(name, ResourceManager.getImageDescriptor("icons/object-views/alarms.png"), id, true);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      alarmList = new AlarmList(this, parent, SWT.NONE, "AlarmView.AlarmList", () -> AlarmsView.this.isActive());
      setFilterClient(alarmList.getViewer(), alarmList.getFilter());
      enableFilterAutocomplete(new SearchQueryContentProposalProvider(alarmList.getAttributeProposals()));
      
      createActions();
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionExportToCsv = new ExportToCsvAction(this, alarmList.getViewer(), false);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectChange(AbstractObject object)
   {
      alarmList.setRootObject((object != null) ? object.getObjectId() : 0);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#activate()
    */
   @Override
   public void activate()
   {
      super.activate();
      alarmList.doPendingUpdates();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      alarmList.refresh();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 15;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return (context != null) && (context instanceof AbstractObject) && ((AbstractObject)context).isAlarmsVisible();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#getContextName()
    */
   @Override
   protected String getContextName()
   {
      return (getObject() != null) ? getObject().getObjectName() : i18n.tr("All");
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(alarmList.getActionShowColors());
      manager.add(new Separator());
      manager.add(actionExportToCsv);
      super.fillLocalMenu(manager);
   }
}
