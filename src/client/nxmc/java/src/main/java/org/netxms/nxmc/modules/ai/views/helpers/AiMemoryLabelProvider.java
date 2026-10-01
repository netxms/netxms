/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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
package org.netxms.nxmc.modules.ai.views.helpers;

import java.util.Date;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.NXCSession;
import org.netxms.client.ai.AiMemoryEntry;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.ai.views.AiMemoryManager;
import org.netxms.nxmc.tools.ViewerElementUpdater;
import org.xnap.commons.i18n.I18n;

/**
 * Label provider for AI memory entries
 */
public class AiMemoryLabelProvider extends LabelProvider implements ITableLabelProvider
{
   private final I18n i18n = LocalizationHelper.getI18n(AiMemoryLabelProvider.class);

   private NXCSession session = Registry.getSession();
   private TableViewer viewer;

   /**
    * Constructor
    *
    * @param viewer owning viewer
    */
   public AiMemoryLabelProvider(TableViewer viewer)
   {
      this.viewer = viewer;
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
   @Override
   public Image getColumnImage(Object element, int columnIndex)
   {
      return null;
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
   @Override
   public String getColumnText(Object element, int columnIndex)
   {
      AiMemoryEntry entry = (AiMemoryEntry)element;
      switch(columnIndex)
      {
         case AiMemoryManager.COLUMN_ID:
            return Integer.toString(entry.getId());
         case AiMemoryManager.COLUMN_SCOPE:
            return getScopeText(entry);
         case AiMemoryManager.COLUMN_TARGET:
            return getTargetText(entry, element);
         case AiMemoryManager.COLUMN_TITLE:
            return entry.getTitle();
         case AiMemoryManager.COLUMN_CONTENT:
            return entry.getContent().replace('\n', ' ');
         case AiMemoryManager.COLUMN_CREATED_BY:
            return entry.isCreatedByModel() ? i18n.tr("Model") : i18n.tr("Human");
         case AiMemoryManager.COLUMN_SOURCE:
            return getSourceText(entry, element);
         case AiMemoryManager.COLUMN_CREATED:
            return formatTime(entry.getCreationTime());
         case AiMemoryManager.COLUMN_UPDATED:
            return formatTime(entry.getModificationTime());
         case AiMemoryManager.COLUMN_LOCKED:
            return entry.isLocked() ? i18n.tr("Yes") : i18n.tr("No");
      }
      return null;
   }

   /**
    * Get scope text for entry.
    *
    * @param entry memory entry
    * @return scope text
    */
   public String getScopeText(AiMemoryEntry entry)
   {
      switch(entry.getScope())
      {
         case ENVIRONMENT:
            return i18n.tr("Environment");
         case USER:
            return i18n.tr("User");
         case OBJECT:
            return i18n.tr("Object");
         default:
            return i18n.tr("Unknown");
      }
   }

   /**
    * Get scope target text (user or object name) for entry.
    *
    * @param entry memory entry
    * @param element viewer element for deferred update
    * @return target text
    */
   public String getTargetText(AiMemoryEntry entry, Object element)
   {
      switch(entry.getScope())
      {
         case USER:
            return getUserName(entry.getScopeId(), element);
         case OBJECT:
            AbstractObject object = session.findObjectById(entry.getScopeId());
            return (object != null) ? object.getObjectName() : ("[" + Integer.toString(entry.getScopeId()) + "]");
         default:
            return "";
      }
   }

   /**
    * Get source text (execution context and user) for entry.
    *
    * @param entry memory entry
    * @param element viewer element for deferred update
    * @return source text
    */
   public String getSourceText(AiMemoryEntry entry, Object element)
   {
      String user = getUserName(entry.getSourceUserId(), element);
      switch(entry.getSourceType())
      {
         case AiMemoryEntry.SOURCE_CHAT:
            return i18n.tr("Chat {0} ({1})", Integer.toString(entry.getSourceId()), user);
         case AiMemoryEntry.SOURCE_TASK:
            return i18n.tr("AI task {0} ({1})", Integer.toString(entry.getSourceId()), user);
         case AiMemoryEntry.SOURCE_OPERATOR:
            return i18n.tr("AI operator {0} ({1})", Integer.toString(entry.getSourceId()), user);
         default:
            return user;
      }
   }

   /**
    * Resolve user name by ID.
    *
    * @param userId user ID
    * @param element viewer element for deferred update
    * @return user name or ID in brackets
    */
   private String getUserName(int userId, Object element)
   {
      AbstractUserObject user = session.findUserDBObjectById(userId, new ViewerElementUpdater(viewer, element));
      return (user != null) ? user.getName() : ("[" + Integer.toString(userId) + "]");
   }

   /**
    * Format timestamp, treating epoch value 0 as "never".
    *
    * @param time timestamp
    * @return formatted timestamp or empty string
    */
   private static String formatTime(Date time)
   {
      if ((time == null) || (time.getTime() == 0))
         return "";
      return DateFormatFactory.getDateTimeFormat().format(time);
   }
}
