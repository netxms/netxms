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
package org.netxms.nxmc.modules.objects.views.helpers;

import org.eclipse.jface.viewers.ColumnViewer;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.MaintenanceJournalEntry;
import org.netxms.client.NXCSession;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.modules.objects.views.MaintenanceJournalView;
import org.netxms.nxmc.tools.ViewerElementUpdater;

/**
 * Physical link label provider
 */
public class MaintenanceJournalLabelProvider extends LabelProvider implements ITableLabelProvider
{
   private NXCSession session;
   private ColumnViewer viewer;

   /**
    * Physical link label provider constructor
    */
   public MaintenanceJournalLabelProvider(ColumnViewer viewer)
   {
      session = Registry.getSession();
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
      MaintenanceJournalEntry me = (MaintenanceJournalEntry)element;
      switch(columnIndex)
      {
         case MaintenanceJournalView.COL_ID:
            return Long.toString(me.getId());
         case MaintenanceJournalView.COL_OBJECT:
            return session.getObjectName(me.getObjectId());
         case MaintenanceJournalView.COL_AUHTHOR:
            AbstractUserObject author = session.findUserDBObjectById(me.getAuthor(), new ViewerElementUpdater(viewer, element));
            return (author != null) ? author.getName() : ("[" + Long.toString(me.getAuthor()) + "]");
         case MaintenanceJournalView.COL_EDITOR:
            AbstractUserObject editor = session.findUserDBObjectById(me.getLastEditedBy(), new ViewerElementUpdater(viewer, element));
            return (editor != null) ? editor.getName() : ("[" + Long.toString(me.getLastEditedBy()) + "]");
         case MaintenanceJournalView.COL_DESCRIPTION:
            return me.getDescriptionShort();
         case MaintenanceJournalView.COL_CREATE_TIME:
            return me.getCreationTime().toString();
         case MaintenanceJournalView.COL_MODIFY_TIME:
            return me.getModificationTime().toString();
      }
      return null;
   }
}
