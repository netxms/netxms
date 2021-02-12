/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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

import org.eclipse.swt.widgets.Composite;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.netxms.nxmc.resources.ResourceManager;
import org.xnap.commons.i18n.I18n;

/**
 * "Last Values" view
 */
public class LastValuesView extends ObjectView
{
   private static final I18n i18n = LocalizationHelper.getI18n(LastValuesView.class);

   /**
    * @param name
    * @param image
    */
   public LastValuesView()
   {
      super(i18n.tr("Last Values"), ResourceManager.getImageDescriptor("icons/object-views/last_values.png"), "LastValues");
   }

   @Override
   protected void createContent(Composite parent)
   {
      // TODO Auto-generated method stub

   }
}
