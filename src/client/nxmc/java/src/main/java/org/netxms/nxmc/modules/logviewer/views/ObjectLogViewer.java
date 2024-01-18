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
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.services.LogDescriptor;

/**
 * Ad-hoc log viewer to be shown in object context
 */
public class ObjectLogViewer extends LogViewer
{   
   private LogDescriptor logDescriptor;
   private AbstractObject object;
   private long contextId;

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
      super(logDescriptor.getViewTitle(), logDescriptor.getLogName());
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
      return object.getObjectName();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#isCloseable()
    */
   @Override
   public boolean isCloseable()
   {
      return true;
   }
}
