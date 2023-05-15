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
import org.eclipse.jface.window.Window;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Asset;
import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.DataCollectionTarget;
import org.netxms.client.objects.Rack;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.ViewPlacement;
import org.netxms.nxmc.base.widgets.MessageArea;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.actions.ObjectAction;
import org.netxms.nxmc.modules.objects.dialogs.ObjectSelectionDialog;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Action for linking asset to object
 */
public class LinkAssetToObjectAction extends ObjectAction<Asset>
{
   private static final I18n i18n = LocalizationHelper.getI18n(LinkAssetToObjectAction.class);

   /**
    * Create action for linking asset to object.
    *
    * @param viewPlacement view placement information
    * @param selectionProvider associated selection provider
    */
   public LinkAssetToObjectAction(ViewPlacement viewPlacement, ISelectionProvider selectionProvider)
   {
      super(Asset.class, i18n.tr("&Link to..."), viewPlacement, selectionProvider);
      setImageDescriptor(ResourceManager.getImageDescriptor("icons/link-objects.png"));
   }

   /**
    * @see org.netxms.nxmc.modules.objects.actions.ObjectAction#run(java.util.List)
    */
   @Override
   protected void run(List<Asset> objects)
   {
      ObjectSelectionDialog dlg = new ObjectSelectionDialog(getShell(), ObjectSelectionDialog.createDataCollectionTargetSelectionFilter());
      if (dlg.open() != Window.OK)
         return;

      final AbstractObject object = dlg.getSelectedObjects().get(0);
      if ((!(object instanceof Rack) && !(object instanceof DataCollectionTarget)) || (object instanceof Cluster))
         return; // Incompatible object selected

      final NXCSession session = Registry.getSession();
      if (object.getAssetId() != 0)
      {
         String question = String.format(i18n.tr("\"%s\" object already linked to \"%s\" asset. Are you sure you want to link it another asset?"), object.getObjectName(), session.getObjectName(object.getAssetId()));
         if (!MessageDialogHelper.openConfirm(getShell(), i18n.tr("Confirm Link"), question))
            return;
      }

      final Asset asset = objects.get(0);
      new Job(i18n.tr("Linking asset \"{0}\" to object \"{1}\"", asset.getObjectName(), object.getObjectName()), null, getMessageArea()) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.linkAsset(asset.getObjectId(), object.getObjectId());
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  getMessageArea().addMessage(MessageArea.SUCCESS,
                        i18n.tr("{0} \"{1}\" successfully linked with asset \"{2}\"", object.getObjectClassName(), object.getObjectName(), asset.getObjectName()));
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot link asset \"{0}\"", asset.getObjectName());
         }
      }.start();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.actions.ObjectAction#isValidForSelection(org.eclipse.jface.viewers.IStructuredSelection)
    */
   @Override
   public boolean isValidForSelection(IStructuredSelection selection)
   {
      return (selection.size() == 1) && (selection.getFirstElement() instanceof Asset);
   }
}
