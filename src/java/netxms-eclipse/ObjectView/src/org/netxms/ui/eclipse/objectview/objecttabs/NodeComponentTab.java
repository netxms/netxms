/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.objectview.objecttabs;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.preference.IPreferenceStore;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Interface;
import org.netxms.client.objects.Node;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectview.Activator;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Generic node component tab (handles children synchronization)
 */
public abstract class NodeComponentTab extends ObjectTab
{
   protected Composite mainArea;
   protected NXCSession session;
   protected SessionListener sessionListener = null;
   protected boolean objectsFullySync;

   /**
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#createTabContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createTabContent(Composite parent)
   {
      session = ConsoleSharedData.getSession();

      IPreferenceStore coreStore = ConsoleSharedData.getSettings();
      objectsFullySync = coreStore.getBoolean("ObjectsFullSync");

      // Create tab main area
      mainArea = new Composite(parent, SWT.BORDER);
      mainArea.setLayout(new FormLayout());

      sessionListener = new SessionListener() {
         @Override
         public void notificationHandler(SessionNotification n)
         {
            if (n.getCode() == SessionNotification.OBJECT_CHANGED)
            {
               AbstractObject object = (AbstractObject)n.getObject();
               if ((object != null) && needRefreshOnObjectChange(object) && (getObject() != null)
                     && object.isDirectChildOf(getObject().getObjectId()))
               {
                  mainArea.getDisplay().asyncExec(new Runnable() {
                     @Override
                     public void run()
                     {
                        refresh();
                     }
                  });
               }
            }
         }
      };
      ConsoleSharedData.getSession().addListener(sessionListener);
   }

   /**
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#selected()
    */
   @Override
   public void selected()
   {
      checkAndSyncChildren(getObject());
      super.selected();
      refresh();
   }

   /**
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#objectChanged(org.netxms.client.objects.AbstractObject)
    */
   @Override
   public void objectChanged(AbstractObject object)
   {
      checkAndSyncChildren(object);
      refresh();
   }

   /**
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#dispose()
    */
   @Override
   public void dispose()
   {
      ConsoleSharedData.getSession().removeListener(sessionListener);
      super.dispose();
   }

   /**
    * Check is child objects are synchronized and synchronize if needed
    * 
    * @param object current object
    */
   private void checkAndSyncChildren(AbstractObject object)
   {
      if (!objectsFullySync && isActive())
      {
         if ((object instanceof Node) && object.hasChildren() && !session.areChildrenSynchronized(object.getObjectId()))
         {
            syncChildren(object);
         }
      }      
   }
   
   /**
    * Hook for subclasses to synchronize additional objects if needed. Default implementation does nothing.
    *
    * @param object current object
    */
   protected void syncAdditionalObjects(AbstractObject object) throws IOException, NXCException
   {
   }

   /**
    * Sync object children form server
    * 
    * @param object current object
    */
   private void syncChildren(AbstractObject object)
   {
      final Composite label = new Composite(mainArea, SWT.NONE);
      label.setLayout(new GridLayout());
      label.setBackground(label.getDisplay().getSystemColor(SWT.COLOR_LIST_BACKGROUND));
      
      Label labelText = new Label(label, SWT.CENTER);
      labelText.setText("Loading...");
      labelText.setLayoutData(new GridData(SWT.CENTER, SWT.CENTER, true, true));      
      labelText.setBackground(label.getBackground());
      
      label.moveAbove(null);
      FormData fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top =  new FormAttachment(0, 0);
      fd.bottom = new FormAttachment(100, 0);
      fd.right = new FormAttachment(100, 0);
      label.setLayoutData(fd);
      mainArea.layout();
      
      ConsoleJob job = new ConsoleJob("Synchronize node components", null, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.syncChildren(object);
            syncAdditionalObjects(object);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  refresh();
                  label.dispose();
                  mainArea.layout();                  
               }
            });
         }
   
         @Override
         protected String getErrorMessage()
         {
            return "Cannot synchronize node components";
         }
      };
      job.setUser(false);
      job.start();
   }

   /**
    * Returns if view should be updated depending on provided object
    * 
    * @param object updated object
    * @return if tab should be refreshed
    */
   public abstract boolean needRefreshOnObjectChange(AbstractObject object);
}
