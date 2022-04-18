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
package org.netxms.nxmc.modules.events.views;

import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.action.ToolBarManager;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.netxms.nxmc.base.views.ConfigurationView;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.events.widgets.EventTemplateList;
import org.netxms.nxmc.resources.ResourceManager;
import org.xnap.commons.i18n.I18n;

/**
 * Event configuration view
 */
public class EventConfigurator extends ConfigurationView
{
   private static final I18n i18n = LocalizationHelper.getI18n(EventConfigurator.class);   
	public static final String ID = "EventTemplateList"; //$NON-NLS-1$
	public static final String JOB_FAMILY = "EventConfiguratorJob"; //$NON-NLS-1$

	private EventTemplateList dataView;

   /**
    * Create event template manager view
    */
   public EventConfigurator()
   {
      super(i18n.tr("Event Templates"), ResourceManager.getImageDescriptor("icons/config-views/event_configurator.png"), ID, true);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {		
      dataView = new EventTemplateList(this, parent, SWT.NONE, ID);
		dataView.getViewer().addDoubleClickListener(new IDoubleClickListener() {
			@Override
			public void doubleClick(DoubleClickEvent event)
			{
			   dataView.getActionEdit().run();
			}
		});
		
      setFilterClient(dataView.getViewer(), dataView.getFilter());
	}

   /**
    * Fill view's local toolbar
    *
    * @param manager toolbar manager
    */
	@Override
   protected void fillLocalToolbar(ToolBarManager manager)
   {
      manager.add(dataView.getActionNewTemplate());
      manager.add(new Separator());
   }

	/**
	 * Fill context menu
	 * 
	 * @param mgr Menu manager
	 */
	protected void fillContextMenu(final IMenuManager mgr)
	{
		mgr.add(dataView.getActionRefresh());
		mgr.add(dataView.getActionDelete());
		mgr.add(new Separator());
		mgr.add(dataView.getActionEdit());
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
