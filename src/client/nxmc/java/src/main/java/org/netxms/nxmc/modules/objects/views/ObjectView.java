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
package org.netxms.nxmc.modules.objects.views;

import org.eclipse.jface.resource.ImageDescriptor;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Memento;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.views.ViewWithContext;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Base class for all context views where context should be NetXMS object.
 */
public abstract class ObjectView extends ViewWithContext
{
   private final I18n i18n = LocalizationHelper.getI18n(ObjectView.class);

   protected NXCSession session;

   private SessionListener clientListener;

   /**
    * Create object view with specific ID.
    *
    * @param name view name
    * @param image view image
    * @param id view ID
    * @param hasFileter true if view should contain filter
    */
   public ObjectView(String name, ImageDescriptor image, String id, boolean hasFilter)
   {
      super(name, image, id, hasFilter);
      session = Registry.getSession();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#getGlobalId()
    */
   @Override
   public String getGlobalId()
   {
      return getBaseId() + "@" + Long.toString(getObjectId());
   }   

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#postContentCreate()
    */
   @Override
   protected void postContentCreate()
   {
      super.postContentCreate();
      setupClientListener();
      if (getObject() != null)
         setContext(getObject());
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#postClone(org.netxms.nxmc.base.views.View)
    */
   @Override
   protected void postClone(View view)
   {
      super.postClone(view);
      setupClientListener();
   }

   /**
    * Setup client listener
    */
   private void setupClientListener()
   {
      clientListener = new SessionListener() {
         @Override
         public void notificationHandler(SessionNotification n)
         {
            if ((n.getCode() == SessionNotification.OBJECT_CHANGED) && ((n.getSubCode() == getObjectId()) || isRelatedObject(n.getSubCode())))
            {
               getDisplay().asyncExec(() -> {
                  setContext(n.getObject(), false);
                  onObjectUpdate((AbstractObject)n.getObject());
               });
            }
         }
      };
      session.addListener(clientListener);
   }

   /**
    * Called when current object is updated
    *
    * @param object updated object
    */
   protected void onObjectUpdate(AbstractObject object)
   {
   }

   /**
    * Check if object with given ID is related and changes to that object should be passed to <code>onObjectUpdate</code>.
    * 
    * @param objectId object ID to test
    * @return true if object is related
    */
   protected boolean isRelatedObject(long objectId)
   {
      return false;
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#getContextName()
    */
   @Override
   protected String getContextName()
   {
      AbstractObject object = getObject();
      return (object != null) ? object.getObjectName() : i18n.tr("[none]");
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#isHidden()
    */
   @Override
   public boolean isHidden()
   {
      return PreferenceStore.getInstance().getAsBoolean("HideView." + getBaseId(), false);
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return (context != null) && (context instanceof AbstractObject);
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#contextChanged(java.lang.Object, java.lang.Object)
    */
   @Override
   protected void contextChanged(Object oldContext, Object newContext)
   {
      onObjectChange((AbstractObject)newContext);
   }

   /**
    * Called when context is changed to new object. Default implementation does nothing.
    *
    * @param object new context
    */
   protected void onObjectChange(AbstractObject object)
   {
   }

   /**
    * Get current context as object.
    *
    * @return current context as object
    */
   public AbstractObject getObject()
   {
      return (AbstractObject)getContext();
   }

   /**
    * Get ID of object set as current context.
    *
    * @return ID of object set as current context or 0 if context is not set
    */
   public long getObjectId()
   {
      AbstractObject object = (AbstractObject)getContext();
      return (object != null) ? object.getObjectId() : 0;
   }

   /**
    * Get name of object set as current context.
    *
    * @return name of object set as current context or 0 if context is not set
    */
   public String getObjectName()
   {
      AbstractObject object = (AbstractObject)getContext();
      return (object != null) ? object.getObjectName() : "";
   }

   /**
    * @see org.netxms.nxmc.base.views.View#dispose()
    */
   @Override
   public void dispose()
   {
      session.removeListener(clientListener);
      super.dispose();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#saveState(org.netxms.nxmc.Memento)
    */
   @Override
   public void saveState(Memento memento)
   {
      super.saveState(memento);
      memento.set("context", getObjectId());
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#restoreContext(org.netxms.nxmc.Memento)
    */
   @Override
   public Object restoreContext(Memento memento)
   {      
      long objectId = memento.getAsLong("context", 0);
      return session.findObjectById(objectId);
   }   
}
