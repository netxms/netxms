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
package org.netxms.ui.eclipse.dashboard.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.FocusAdapter;
import org.eclipse.swt.events.FocusEvent;
import org.eclipse.ui.IViewPart;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.xml.XMLTools;
import org.netxms.ui.eclipse.alarmviewer.widgets.AlarmList;
import org.netxms.ui.eclipse.console.Activator;
import org.netxms.ui.eclipse.dashboard.widgets.internal.AlarmViewerConfig;

/**
 * Alarm viewer element for dashboard
 */
public class AlarmViewerElement extends ElementWidget
{
	private AlarmList viewer;
	private AlarmViewerConfig config;

	/**
	 * Create new alarm viewer element
	 * 
	 * @param parent Dashboard control
	 * @param element Dashboard element
	 * @param viewPart viewPart
	 */
	public AlarmViewerElement(DashboardControl parent, DashboardElement element, IViewPart viewPart)
	{
		super(parent, element, viewPart);

		try
		{
         config = XMLTools.createFromXml(AlarmViewerConfig.class, element.getData());
		}
		catch(Exception e)
		{
         Activator.logError("Cannot parse dashboard element configuration", e);
			config = new AlarmViewerConfig();
		}

      processCommonSettings(config);

      viewer = new AlarmList(viewPart, getContentArea(), SWT.NONE, "Dashboard.AlarmList", null); //$NON-NLS-1$
      viewer.setRootObject(getEffectiveObjectId(config.getObjectId()));
      viewer.setSeverityFilter(config.getSeverityFilter());
      viewer.setStateFilter(config.getStateFilter());
      viewer.setIsLocalSoundEnabled(config.getIsLocalSoundEnabled());
      viewer.getViewer().getControl().addFocusListener(new FocusAdapter() {
         @Override
         public void focusGained(FocusEvent e)
         {
            setSelectionProviderDelegate(viewer.getSelectionProvider());
         }
      });
	}
}
