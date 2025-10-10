/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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
package org.netxms.nxmc.modules.logviewer.views;

import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Memento;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.views.ViewNotRestoredException;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.logviewer.LogDescriptorRegistry;
import org.netxms.nxmc.services.LogDescriptor;
import org.xnap.commons.i18n.I18n;

/**
 * Ad-hoc log viewer to be shown in object context
 */
public class ObjectLogViewer extends LogViewer
{   
   private final I18n i18n = LocalizationHelper.getI18n(ObjectLogViewer.class);
   
   private LogDescriptor logDescriptor;
   private AbstractObject object;
   private long contextId;
   private boolean restored = false;

   /**
    * Internal constructor used for cloning
    */
   protected ObjectLogViewer()
   {
      super();
   }

   /**
    * Create new log viewer view
    *
    * @param logDescriptor log descriptor
    * @param object object to show log for
    * @param contextId context id
    */
   public ObjectLogViewer(LogDescriptor logDescriptor, AbstractObject object, long contextId)
   {
      super(logDescriptor.getViewTitle(), logDescriptor.getLogName(), Long.toString(object.getObjectId()));
      this.logDescriptor = logDescriptor;  
      this.contextId = contextId;
      this.object = object;
   }

   /**
    * @see org.netxms.nxmc.base.views.View#postClone(org.netxms.nxmc.base.views.View)
    */
   @Override
   protected void postClone(View view)
   {
      ObjectLogViewer origin = (ObjectLogViewer)view;
      this.logDescriptor = origin.logDescriptor;  
      this.contextId = origin.contextId;
      this.object = origin.object;
      super.postClone(view);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#postContentCreate()
    */
   @Override
   protected void postContentCreate()
   {            
      if (!restored)
         queryWithFilter(logDescriptor.createFilter(object));
      super.postContentCreate();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return (context != null) && (context instanceof AbstractObject) && 
            ((((AbstractObject)context).getObjectId() == object.getObjectId()) || (((AbstractObject)context).getObjectId() == contextId));
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#getContextName()
    */
   @Override
   protected String getContextName()
   {
      return (object != null) ? object.getObjectName() : "(null)";
   }

   /**
    * @see org.netxms.nxmc.base.views.View#getFullName()
    */
   @Override
   public String getFullName()
   {
      return getName() + " - " + getContextName();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#isCloseable()
    */
   @Override
   public boolean isCloseable()
   {
      return true;
   }

   /**
    * @see org.netxms.nxmc.base.views.View#saveState(org.netxms.nxmc.Memento)
    */
   @Override
   public void saveState(Memento memento)
   {
      super.saveState(memento);
      memento.set("contextId", contextId);
      memento.set("objectId", object.getObjectId());
   }

   /**
    * @throws ViewNotRestoredException 
    * @see org.netxms.nxmc.base.views.ViewWithContext#restoreState(org.netxms.nxmc.Memento)
    */
   @Override
   public void restoreState(Memento memento) throws ViewNotRestoredException
   {      
      super.restoreState(memento);
      logDescriptor = Registry.getSingleton(LogDescriptorRegistry.class).get(getLogName());
      contextId = memento.getAsLong("contextId", 0);
      long objectId = memento.getAsLong("objectId", 0);
      object = session.findObjectById(objectId);
      if (object == null)
         throw(new ViewNotRestoredException(i18n.tr("Invalid object id")));
      restored = true;
   } 
}
