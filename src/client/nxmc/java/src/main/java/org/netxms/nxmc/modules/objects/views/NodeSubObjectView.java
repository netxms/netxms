/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Raden Solutions
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

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Node;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Generic base class for all node views that display node sub-objects (interfaces, services, etc.)
 */
public abstract class NodeSubObjectView extends ObjectView
{
   private I18n i18n = LocalizationHelper.getI18n(NodeSubObjectView.class);

   protected Composite mainArea;
   protected NXCSession session;
   protected SessionListener sessionListener = null;
   protected boolean objectsFullySync;

   /**
    * @param name
    * @param image
    * @param id
    */
   public NodeSubObjectView(String name, ImageDescriptor image, String id)
   {
      super(name, image, id);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      session = Registry.getSession();

      PreferenceStore coreStore = PreferenceStore.getInstance();
      objectsFullySync = coreStore.getAsBoolean("ObjectBrowser.FullSync", false);

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
      session.addListener(sessionListener);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return (context != null) && (context instanceof AbstractNode);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#activate()
    */
   @Override
   public void activate()
   {
      checkAndSyncChildren(getObject());
      super.activate();
      refresh();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#dispose()
    */
   @Override
   public void dispose()
   {
      session.removeListener(sessionListener);
      super.dispose();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectChange(AbstractObject object)
   {
      checkAndSyncChildren(object);
      refresh();
   }

   /**
    * Check is child objects are synchronized and synchronize if needed
    * 
    * @param object current object
    */
   private void checkAndSyncChildren(AbstractObject object)
   {
      if (!objectsFullySync && isVisible())
      {
         if ((object instanceof Node) && object.hasChildren() && !session.areChildrenSynchronized(object.getObjectId()))
         {
            syncChildren(object);
         }
      }
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
      labelText.setText(i18n.tr("Loading..."));
      labelText.setLayoutData(new GridData(SWT.CENTER, SWT.CENTER, true, true));
      labelText.setBackground(label.getBackground());

      label.moveAbove(null);
      FormData fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(0, 0);
      fd.bottom = new FormAttachment(100, 0);
      fd.right = new FormAttachment(100, 0);
      label.setLayoutData(fd);
      mainArea.layout();

      Job job = new Job(i18n.tr("Synchronize node sub-objects"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.syncChildren(object);
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
            return i18n.tr("Cannot synchronize node sub-objects");
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
