/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.datacollection.views;

import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.EntireNetwork;
import org.netxms.client.objects.Rack;
import org.netxms.client.objects.ServiceRoot;
import org.netxms.client.objects.Subnet;
import org.netxms.client.objects.Zone;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.widgets.ThresholdSummaryWidget;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.tools.VisibilityValidator;
import org.xnap.commons.i18n.I18n;

/**
 * Threshold violation summary page
 */
public class ThresholdSummary extends ObjectView
{
   private final static I18n i18n = LocalizationHelper.getI18n(ThresholdSummary.class);
   
	private ThresholdSummaryWidget tsw;

   /**
    * Create web service manager view
    */
   public ThresholdSummary()
   {
      super(i18n.tr("Thresholds"), ResourceManager.getImageDescriptor("icons/object-views/thresholds.gif"), "Thresholds", false);
   }

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
		tsw = new ThresholdSummaryWidget(parent, SWT.NONE, this, new VisibilityValidator() {
         @Override
         public boolean isVisible()
         {
            return isActive();
         }
      });
	}

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return (context instanceof EntireNetwork) || (context instanceof Zone) ||
            (context instanceof Subnet) || (context instanceof ServiceRoot) ||
            (context instanceof Container) || (context instanceof Rack) || 
            (context instanceof Cluster);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectChange(AbstractObject object)
   {
		tsw.setObject(object);
	}

   /**
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#refresh()
    */
	@Override
	public void refresh()
	{
		tsw.refresh();
	}
}
