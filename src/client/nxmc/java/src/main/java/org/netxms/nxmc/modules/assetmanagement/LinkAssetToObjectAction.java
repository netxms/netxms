/**
 * 
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
import org.netxms.nxmc.modules.objects.actions.ForcedPolicyDeploymentAction;
import org.netxms.nxmc.modules.objects.actions.ObjectAction;
import org.netxms.nxmc.modules.objects.dialogs.ObjectSelectionDialog;
import org.netxms.nxmc.resources.ResourceManager;
import org.xnap.commons.i18n.I18n;

/**
 * Action for linking asset to object
 */
public class LinkAssetToObjectAction extends ObjectAction<Asset>
{
   private static final I18n i18n = LocalizationHelper.getI18n(ForcedPolicyDeploymentAction.class);

   /**
    * Create action for linking asset to object.
    *
    * @param viewPlacement view placement information
    * @param selectionProvider associated selection provider
    */
   public LinkAssetToObjectAction(ViewPlacement viewPlacement, ISelectionProvider selectionProvider)
   {
      super(Asset.class, i18n.tr("&Link to object..."), viewPlacement, selectionProvider);
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

      final Asset asset = objects.get(0);
      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Linking asset \"{0}\" to object \"{1}\"", asset.getObjectName(), object.getObjectName()), null, getMessageArea()) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.linkAsset(asset.getObjectId(), object.getObjectId());
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  getMessageArea().addMessage(MessageArea.INFORMATION,
                        i18n.tr("{0} \"{0}\" successfully linked with asset \"{2}\"", object.getObjectClassName(), object.getObjectName(), asset.getObjectName()));
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
