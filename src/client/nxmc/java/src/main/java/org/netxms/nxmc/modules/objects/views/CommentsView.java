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
package org.netxms.nxmc.modules.objects.views;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.ResourceManager;
import org.xnap.commons.i18n.I18n;

/**
 * Comments view
 */
public class CommentsView extends ObjectView
{
   private final I18n i18n = LocalizationHelper.getI18n(CommentsView.class);

   private Text content;
   private boolean showEmptyComments;

   /**
    * Create new comments view
    */
   public CommentsView()
   {
      super(LocalizationHelper.getI18n(CommentsView.class).tr("Comments"), ResourceManager.getImageDescriptor("icons/object-views/comments.png"), "objects.comments", false);
      showEmptyComments = !Registry.getSession().getClientConfigurationHintAsBoolean("ObjectOverview.ShowCommentsOnlyIfPresent", true);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return (context != null) && (context instanceof AbstractObject) && (showEmptyComments || !((AbstractObject)context).getComments().isBlank());
   }

   /**
    * @see org.netxms.nxmc.base.views.View#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 300;
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      content = new Text(parent, SWT.H_SCROLL | SWT.V_SCROLL | SWT.MULTI);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectChange(AbstractObject object)
   {
      content.setText((object != null) ? object.getComments() : "");
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectUpdate(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectUpdate(AbstractObject object)
   {
      content.setText((object != null) ? object.getComments() : "");
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      final long objectId = getObjectId();
      new Job(i18n.tr("Refresh comments"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.syncObjectSet(new long[] { objectId }, NXCSession.OBJECT_SYNC_NOTIFY);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot resynchromize object");
         }
      }.start();
   }
}
