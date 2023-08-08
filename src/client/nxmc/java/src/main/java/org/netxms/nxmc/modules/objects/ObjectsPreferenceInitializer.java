/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objects;

import java.util.ArrayList;
import org.eclipse.jface.action.Action;
import org.netxms.client.NXCSession;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.services.PreferenceInitializer;

/**
 * Preference initializer for charts
 */
public class ObjectsPreferenceInitializer implements PreferenceInitializer
{
   /**
    * @see org.netxms.nxmc.services.PreferenceInitializer#initializeDefaultPreferences(org.netxms.nxmc.PreferenceStore)
    */
   @Override
   public void initializeDefaultPreferences(PreferenceStore ps)
   {
      ps.setDefault("ObjectBrowser.filterAutoApply", true); //$NON-NLS-1$
      ps.setDefault("ObjectBrowser.filterDelay", 300); //$NON-NLS-1$
      ps.setDefault("ObjectBrowser.filterMinLength", 1); //$NON-NLS-1$
      ps.setDefault("ObjectBrowser.showFilter", true); //$NON-NLS-1$
      ps.setDefault("ObjectBrowser.showStatusIndicator", false); //$NON-NLS-1$
      ps.setDefault("ObjectBrowser.useServerFilterSettings", true); //$NON-NLS-1$
      
      ps.setDefault("ObjectStatusIndicator.showIcons", false); //$NON-NLS-1$
      ps.setDefault("ObjectStatusIndicator.hideNormal", true); //$NON-NLS-1$
      ps.setDefault("ObjectStatusIndicator.hideUnmanaged", true); //$NON-NLS-1$
      ps.setDefault("ObjectStatusIndicator.hideUnknown", true); //$NON-NLS-1$
      ps.setDefault("ObjectStatusIndicator.hideDisabled", true); //$NON-NLS-1$    
   }
}
