/**
 * NetXMS - open source network management system
 * Copyright (C) 2016-2023 Raden Solutions
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
package org.netxms.nxmc.modules.objects.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;
import org.netxms.client.NXCSession;
import org.netxms.client.topology.FdbEntry;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.helpers.TokenizedFilter;
import org.netxms.nxmc.base.views.AbstractViewerFilter;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Filter for switch forwarding database  
 */
public class FDBFilter extends ViewerFilter implements AbstractViewerFilter
{
   private final I18n i18n = LocalizationHelper.getI18n(FDBFilter.class);
   private final String typeMatchDynamic = i18n.tr("Dynamic").toLowerCase();
   private final String typeMatchStatic = i18n.tr("Static").toLowerCase();
   private final String typeMatchUnknown = i18n.tr("Unknown").toLowerCase();

   private NXCSession session = Registry.getSession();
   private TokenizedFilter filter = new TokenizedFilter(null);

   /**
    * @see org.eclipse.jface.viewers.ViewerFilter#select(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public boolean select(Viewer viewer, Object parentElement, Object element)
   {
      if (filter.isEmpty())
         return true;

      final FdbEntry e = (FdbEntry)element;
      return filter.matches(e.getAddress().toString(), Integer.toString(e.getPort()), e.getInterfaceName(), Integer.toString(e.getVlanId()),
            (e.getNodeId() != 0) ? session.getObjectName(e.getNodeId()) : null,
            getTypeText(e),
            session.getVendorByMac(e.getAddress(), null));
   }

   /**
    * Get text for FDB entry type
    */
   private String getTypeText(FdbEntry en)
   {
      switch(en.getType())
      {
         case 3:
            return typeMatchDynamic;
         case 5:
            return typeMatchStatic;
         default:
            return typeMatchUnknown;
      }
   }

   /**
    * @see org.netxms.nxmc.base.views.AbstractViewerFilter#setFilterString(java.lang.String)
    */
   @Override
   public void setFilterString(String filterString)
   {
      this.filter = new TokenizedFilter(filterString);
   }   
}
