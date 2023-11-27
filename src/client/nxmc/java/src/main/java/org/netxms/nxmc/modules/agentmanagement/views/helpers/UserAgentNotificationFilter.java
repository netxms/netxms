/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
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
package org.netxms.nxmc.modules.agentmanagement.views.helpers;

import java.util.Date;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;
import org.netxms.client.UserAgentNotification;
import org.netxms.nxmc.base.views.AbstractViewerFilter;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.xnap.commons.i18n.I18n;

/**
 * User support application notification filter
 */
public class UserAgentNotificationFilter extends ViewerFilter implements AbstractViewerFilter
{
   private final I18n i18n = LocalizationHelper.getI18n(UserAgentNotificationFilter.class);
   
   private String filterString = null;
   private UserAgentNotificationLabelProvider provider;
   private boolean showAllOneTime = true;
   private boolean showAllOneScheduled = true;
   
   /**
    * Constructor
    * 
    * @param provider label provider
    */
   public UserAgentNotificationFilter(UserAgentNotificationLabelProvider provider)
   {
      this.provider = provider;      
   }

   @Override
   public boolean select(Viewer viewer, Object parentElement, Object element)
   {
      UserAgentNotification uam = (UserAgentNotification)element;
      
      if (!showAllOneTime && uam.getEndTime().getTime() == 0 && uam.getCreationTime().before(new Date(System.currentTimeMillis() - 3600 * 1000)))
         return false;
      if (!showAllOneScheduled && uam.getEndTime().getTime() != 0 && uam.getEndTime().before(new Date()))
         return false;
      
      if ((filterString == null) || (filterString.isEmpty()))
         return true;
      else if (Long.toString(uam.getId()).toLowerCase().contains(filterString))
         return true;
      else if (uam.getMessage().toLowerCase().contains(filterString))
         return true;
      else if (uam.getObjectNames().toLowerCase().contains(filterString))
         return true;
      else if (DateFormatFactory.getDateTimeFormat().format(uam.getEndTime()).contains(filterString))
         return true;
      else if (DateFormatFactory.getDateTimeFormat().format(uam.getStartTime()).contains(filterString))
         return true;
      else if (uam.isRecalled() ? i18n.tr("yes").contains(filterString) : i18n.tr("no").contains(filterString))
         return true;
      else if (uam.isStartupNotification() ? i18n.tr("yes").contains(filterString) : i18n.tr("no").contains(filterString))
         return true;
      else if (DateFormatFactory.getDateTimeFormat().format(uam.getCreationTime()).contains(filterString))
         return true;
      else if (provider.getUserName(uam).contains(filterString))
         return true;
      return false;
   }
   
   /**
    * @param filterString the filterString to set
    */
   public void setFilterString(String filterString)
   {
      this.filterString = filterString.toLowerCase();
   }

   /**
    * @return the showAllOneTime
    */
   public boolean isShowAllOneTime()
   {
      return showAllOneTime;
   }

   /**
    * @param showAllOneTime the showAllOneTime to set
    */
   public void setShowAllOneTime(boolean showAllOneTime)
   {
      this.showAllOneTime = showAllOneTime;
   }

   /**
    * @return the showAllOneScheduled
    */
   public boolean isShowAllOneScheduled()
   {
      return showAllOneScheduled;
   }

   /**
    * @param showAllOneScheduled the showAllOneScheduled to set
    */
   public void setShowAllOneScheduled(boolean showAllOneScheduled)
   {
      this.showAllOneScheduled = showAllOneScheduled;
   }
}
