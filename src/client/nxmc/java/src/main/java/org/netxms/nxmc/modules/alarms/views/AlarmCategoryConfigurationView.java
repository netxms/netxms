/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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

import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.netxms.nxmc.base.views.ConfigurationView;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.alarms.widgets.AlarmCategoryList;
import org.netxms.nxmc.resources.ResourceManager;

/**
 * Configuration view for alarm categories
 */
public class AlarmCategoryConfigurationView extends ConfigurationView
{
   private AlarmCategoryList dataView;

   /**
    * Constructor
    */
   public AlarmCategoryConfigurationView()
   {
      super(LocalizationHelper.getI18n(AlarmCategoryConfigurationView.class).tr("Alarm Categories"), ResourceManager.getImageDescriptor("icons/config-views/alarm_category.png"),
            "configuration.alarm-categories", true);
   }

   @Override
   protected void createContent(Composite parent)
   {
      parent.setLayout(new FillLayout());

      dataView = new AlarmCategoryList(this, parent, SWT.NONE, "AlarmCategoryConfigurator", true);
      setFilterClient(dataView.getViewer(), dataView.getFilter());

      dataView.getViewer().addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            dataView.editCategory();
         }
      });
   }

   /**
    * Fill view's local toolbar
    *
    * @param manager toolbar manager
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(dataView.getAddCategoryAction());
      manager.add(new Separator());
   }
   
   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      dataView.refreshView();
   }

   /**
    * @see org.netxms.nxmc.base.views.ConfigurationView#isModified()
    */
   @Override
   public boolean isModified()
   {
      return false;
   }

   /**
    * @see org.netxms.nxmc.base.views.ConfigurationView#save()
    */
   @Override
   public void save()
   {
   }
}
