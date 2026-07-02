/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Victor Kirhenshtein
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

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.window.Window;
import org.netxms.base.MacAddress;
import org.netxms.client.NXCObjectCreationData;
import org.netxms.client.NXCSession;
import org.netxms.client.asset.AssetAttribute;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.AccessPoint;
import org.netxms.client.objects.Circuit;
import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.Collector;
import org.netxms.client.objects.DataCollectionTarget;
import org.netxms.client.objects.MobileDevice;
import org.netxms.client.objects.Rack;
import org.netxms.client.objects.Sensor;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.ViewPlacement;
import org.netxms.nxmc.base.widgets.MessageArea;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.assetmanagement.dialogs.CreateAssetDialog;
import org.netxms.nxmc.modules.objects.actions.ObjectAction;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Action for creating new asset linked to selected object. Asset properties are pre-filled from the object, and server will
 * update asset identification and run auto fill scripts after linking.
 */
public class CreateAssetFromObjectAction extends ObjectAction<AbstractObject>
{
   private final I18n i18n = LocalizationHelper.getI18n(CreateAssetFromObjectAction.class);

   /**
    * Create action for creating new asset linked to object.
    *
    * @param viewPlacement view placement information
    * @param selectionProvider associated selection provider
    */
   public CreateAssetFromObjectAction(ViewPlacement viewPlacement, ISelectionProvider selectionProvider)
   {
      super(AbstractObject.class, LocalizationHelper.getI18n(CreateAssetFromObjectAction.class).tr("Create &asset..."), viewPlacement, selectionProvider);
      setImageDescriptor(ResourceManager.getImageDescriptor("icons/objects/asset.png"));
   }

   /**
    * @see org.netxms.nxmc.modules.objects.actions.ObjectAction#run(java.util.List)
    */
   @Override
   protected void run(List<AbstractObject> objects)
   {
      final NXCSession session = Registry.getSession();
      final AbstractObject object = objects.get(0);

      if (object.getAssetId() != 0)
      {
         String question = String.format(i18n.tr("Object \"%s\" already linked to asset \"%s\". Are you sure you want to link it to a new asset?"), object.getObjectName(),
               session.getObjectName(object.getAssetId()));
         if (!MessageDialogHelper.openConfirm(getShell(), i18n.tr("Confirm Asset Creation"), question))
            return;
      }

      final CreateAssetDialog dlg = new CreateAssetDialog(getShell(), object.getObjectName(), getPrefilledProperties(object));
      if (dlg.open() != Window.OK)
         return;

      new Job(i18n.tr("Creating asset for object \"{0}\"", object.getObjectName()), null, getMessageArea()) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            NXCObjectCreationData cd = new NXCObjectCreationData(AbstractObject.OBJECT_ASSET, dlg.getName(), dlg.getParentId());
            cd.setObjectAlias(dlg.getAlias());
            cd.setAssetProperties(dlg.getProperties());
            cd.setLinkedObjectId(object.getObjectId());
            session.createObject(cd);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  getMessageArea().addMessage(MessageArea.SUCCESS,
                        i18n.tr("Asset \"{0}\" successfully created and linked with {1} \"{2}\"", dlg.getName(), object.getObjectClassName(), object.getObjectName()));
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot create asset \"{0}\"", dlg.getName());
         }
      }.start();
   }

   /**
    * Get asset property values that can be pre-filled from the object (for attributes with system type set).
    *
    * @param object object the new asset will be linked to
    * @return pre-filled asset properties
    */
   private Map<String, String> getPrefilledProperties(AbstractObject object)
   {
      String serial = null;
      String vendor = null;
      String model = null;
      String ipAddress = null;
      MacAddress macAddress = null;
      if (object instanceof AbstractNode)
      {
         AbstractNode node = (AbstractNode)object;
         serial = node.getHardwareSerialNumber();
         vendor = node.getHardwareVendor();
         model = node.getHardwareProductName();
         macAddress = node.getPrimaryMAC();
         if ((node.getPrimaryIP() != null) && node.getPrimaryIP().isValidUnicastAddress())
            ipAddress = node.getPrimaryIP().getHostAddress();
      }
      else if (object instanceof AccessPoint)
      {
         AccessPoint accessPoint = (AccessPoint)object;
         serial = accessPoint.getSerialNumber();
         vendor = accessPoint.getVendor();
         model = accessPoint.getModel();
         macAddress = accessPoint.getMacAddress();
      }
      else if (object instanceof MobileDevice)
      {
         MobileDevice mobileDevice = (MobileDevice)object;
         serial = mobileDevice.getSerialNumber();
         vendor = mobileDevice.getVendor();
         model = mobileDevice.getModel();
      }
      else if (object instanceof Sensor)
      {
         Sensor sensor = (Sensor)object;
         serial = sensor.getSerialNumber();
         vendor = sensor.getVendor();
         model = sensor.getModel();
         macAddress = sensor.getMacAddress();
      }

      Map<String, String> properties = new HashMap<>();
      for(AssetAttribute a : Registry.getSession().getAssetManagementSchema().values())
      {
         String value = null;
         switch(a.getSystemType())
         {
            case SERIAL:
               value = serial;
               break;
            case MAC_ADDRESS:
               // Mirror server-side identification logic - MAC address is used only when serial number is not available
               if (((serial == null) || serial.isEmpty()) && (macAddress != null) && !macAddress.isNull())
                  value = macAddress.toString();
               break;
            case IP_ADDRESS:
               value = ipAddress;
               break;
            case VENDOR:
               value = vendor;
               break;
            case MODEL:
               value = model;
               break;
            default:
               break;
         }
         if ((value != null) && !value.isEmpty())
            properties.put(a.getName(), value);
      }
      return properties;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.actions.ObjectAction#isValidForSelection(org.eclipse.jface.viewers.IStructuredSelection)
    */
   @Override
   public boolean isValidForSelection(IStructuredSelection selection)
   {
      if (selection.size() != 1)
         return false;
      Object object = selection.getFirstElement();
      if (object instanceof Rack)
         return true;
      return (object instanceof DataCollectionTarget) && !(object instanceof Cluster) && !(object instanceof Collector) && !(object instanceof Circuit);
   }
}
