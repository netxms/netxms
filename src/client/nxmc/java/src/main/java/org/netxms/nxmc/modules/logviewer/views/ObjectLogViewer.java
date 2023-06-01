/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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
import org.netxms.nxmc.modules.logviewer.LogDeclaration;

public class ObjectLogViewer extends LogViewer
{   
   private LogDeclaration logDeclaration;
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
    * @param logDeclaration log declaration object
    * @param object object to show log for
    * @param contextId context id
    */
   public ObjectLogViewer(LogDeclaration logDeclaration, AbstractObject object, long contextId)
   {
      super(logDeclaration.getViewName(), logDeclaration.getLogName());
      this.logDeclaration = logDeclaration;  
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
      this.logDeclaration = origin.logDeclaration;  
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
      queryWithFilter(logDeclaration.getFilter(object));      
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
    * @see org.netxms.nxmc.base.views.View#isCloseable()
    */
   @Override
   public boolean isCloseable()
   {
      return true;
   }
}
