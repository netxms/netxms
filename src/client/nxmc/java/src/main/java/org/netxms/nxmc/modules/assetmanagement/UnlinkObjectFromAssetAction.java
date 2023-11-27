/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.assetmanagement;

import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.ViewPlacement;
import org.netxms.nxmc.base.widgets.MessageArea;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.actions.ObjectAction;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Action for unlinking asset from object
 */
public class UnlinkObjectFromAssetAction extends ObjectAction<AbstractObject>
{
   private final I18n i18n = LocalizationHelper.getI18n(UnlinkObjectFromAssetAction.class);

   /**
    * Create action for linking asset to object.
    *
    * @param viewPlacement view placement information
    * @param selectionProvider associated selection provider
    */
   public UnlinkObjectFromAssetAction(ViewPlacement viewPlacement, ISelectionProvider selectionProvider)
   {
      super(AbstractObject.class, LocalizationHelper.getI18n(UnlinkObjectFromAssetAction.class).tr("&Unlink from asset"), viewPlacement, selectionProvider);
      setImageDescriptor(ResourceManager.getImageDescriptor("icons/disconnect.png"));
   }

   /**
    * @see org.netxms.nxmc.modules.objects.actions.ObjectAction#run(java.util.List)
    */
   @Override
   protected void run(final List<AbstractObject> objects)
   {
      if (!MessageDialogHelper.openQuestion(getShell(), i18n.tr("Unlink Asset"),
            (objects.size() == 1) ? i18n.tr("Asset will be unlinked from \"{0}\". Are you sure?", objects.get(0).getObjectName()) :
                  i18n.tr("{0} objects will be uninked from assets. Are you sure?", Integer.toString(objects.size()))))
         return;

      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Unlinking assets"), null, getMessageArea()) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(AbstractObject a : objects)
               session.unlinkAsset(a.getAssetId());
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  getMessageArea().addMessage(MessageArea.SUCCESS, i18n.tr("Assets unlinked"));
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot unlink asset");
         }
      }.start();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.actions.ObjectAction#isValidForSelection(org.eclipse.jface.viewers.IStructuredSelection)
    */
   @Override
   public boolean isValidForSelection(IStructuredSelection selection)
   {
      if (selection.isEmpty())
         return false;

      for(Object e : selection.toList())
      {
         if (((AbstractObject)e).getAssetId() == 0)
            return false;
      }

      return true;
   }
}
