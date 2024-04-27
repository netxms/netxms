/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.datacollection.widgets.helpers;

import org.eclipse.jface.viewers.ITableColorProvider;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.Severity;
import org.netxms.client.datacollection.Threshold;
import org.netxms.client.events.EventTemplate;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.modules.datacollection.propertypages.Thresholds;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.netxms.nxmc.resources.ThemeEngine;

/**
 * Label provider for threshold objects
 */
public class ThresholdLabelProvider extends LabelProvider implements ITableLabelProvider, ITableColorProvider
{
   private NXCSession session = Registry.getSession();
   private Image thresholdIcon = ResourceManager.getImageDescriptor("icons/threshold.png").createImage();
   private Image disabledThresholdIcon = ResourceManager.getImageDescriptor("icons/disabled-threshold.png").createImage();
   private Color disabledElementColor = ThemeEngine.getForegroundColor("List.DisabledItem");

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		switch(columnIndex)
		{
			case Thresholds.COLUMN_OPERATION:
            return ((Threshold)element).isDisabled() ? disabledThresholdIcon : thresholdIcon;
			case Thresholds.COLUMN_EVENT:
				final EventTemplate event = (EventTemplate)session.findEventTemplateByCode(((Threshold)element).getFireEvent());
				return StatusDisplayInfo.getStatusImage((event != null) ? event.getSeverity() : Severity.UNKNOWN);
         case Thresholds.COLUMN_DEACTIVATION_EVENT:
            final EventTemplate rearmEvent = (EventTemplate)session.findEventTemplateByCode(((Threshold)element).getRearmEvent());
            return StatusDisplayInfo.getStatusImage((rearmEvent != null) ? rearmEvent.getSeverity() : Severity.UNKNOWN);
		}
		return null;
	}

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
	@Override
	public String getColumnText(Object element, int columnIndex)
	{
		switch(columnIndex)
		{
			case Thresholds.COLUMN_OPERATION:
				return ((Threshold)element).getTextualRepresentation();
			case Thresholds.COLUMN_EVENT:
            return session.getEventName(((Threshold)element).getFireEvent());
         case Thresholds.COLUMN_DEACTIVATION_EVENT:
            return session.getEventName(((Threshold)element).getRearmEvent());
		}
		return null;
	}

   /**
    * @see org.eclipse.jface.viewers.IBaseLabelProvider#dispose()
    */
	@Override
	public void dispose()
	{
		thresholdIcon.dispose();
      disabledThresholdIcon.dispose();
		super.dispose();
	}

   /**
    * @see org.eclipse.jface.viewers.ITableColorProvider#getForeground(java.lang.Object, int)
    */
   @Override
   public Color getForeground(Object element, int columnIndex)
   {
      return ((Threshold)element).isDisabled() ? disabledElementColor : null;
   }

   /**
    * @see org.eclipse.jface.viewers.ITableColorProvider#getBackground(java.lang.Object, int)
    */
   @Override
   public Color getBackground(Object element, int columnIndex)
   {
      return null;
   }
}
