/**
 * NetXMS - open source network management system
 * Copyright (C) 2017-2022 Raden Solutions
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
package org.netxms.ui.eclipse.objectmanager.wizards;

import org.eclipse.jface.wizard.IWizardPage;
import org.eclipse.jface.wizard.Wizard;
import org.netxms.client.NXCObjectCreationData;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Sensor;
import org.netxms.ui.eclipse.objectmanager.widgets.SensorCommon;
import org.netxms.ui.eclipse.objectmanager.wizards.pages.SensorGeneralPage;
import org.netxms.ui.eclipse.objectmanager.wizards.pages.SensorLoraWan;

/**
 * Sensor creation wizard
 */
public class CreateSensorWizard extends Wizard
{
   private NXCObjectCreationData cd;
   private SensorGeneralPage generalPage;
   private SensorLoraWan loraWanPage;

   /**
    * Wizard constructor. Also creates NXCObjectCreationData for new object.
    * 
    * @param parentId parent object ID in object tree
    */
   public CreateSensorWizard(long parentId)
   {
      super();
      cd = new NXCObjectCreationData(AbstractObject.OBJECT_SENSOR, "", parentId);
   }

   /**
    * @see org.eclipse.jface.wizard.Wizard#getWindowTitle()
    */
   @Override
   public String getWindowTitle()
   {
      return "Create Sensor";
   }

   /**
    * @see org.eclipse.jface.wizard.Wizard#addPages()
    */
   @Override
   public void addPages()
   {
      generalPage = new SensorGeneralPage();
      loraWanPage = new SensorLoraWan();

      addPage(generalPage);
      addPage(loraWanPage);
   }

   /**
    * @see org.eclipse.jface.wizard.Wizard#getNextPage(org.eclipse.jface.wizard.IWizardPage)
    */
   @Override
   public IWizardPage getNextPage(IWizardPage currentPage)
   {
      if (currentPage == generalPage)
      {
         if (generalPage.getCommMethod() == Sensor.COMM_LORAWAN)
         {
            return loraWanPage;
         }
      }
      return null;
   }

   /**
    * @see org.eclipse.jface.wizard.Wizard#performFinish()
    */
   @Override
   public boolean performFinish()
   {
      cd.setName(generalPage.getObjectName());
      cd.setObjectAlias(generalPage.getObjectAlias());
      cd.setCommProtocol(generalPage.getCommMethod());
      SensorCommon commonData = generalPage.getCommonData();
      cd.setMacAddress(commonData.getMacAddress());
      cd.setDeviceClass(commonData.getDeviceClass());
      cd.setVendor(commonData.getVendor());
      cd.setSerialNumber(commonData.getSerial());
      cd.setMetaType(commonData.getMetaType());
      cd.setDeviceAddress(commonData.getDeviceAddress());
      cd.setDescription(commonData.getDescription());
      cd.setSensorProxy(commonData.getProxyNode());
      if (cd.getCommProtocol() == Sensor.COMM_LORAWAN)
         cd.setXmlRegConfig(loraWanPage.getRegConfig());
      if (cd.getCommProtocol() == Sensor.COMM_LORAWAN)
         cd.setXmlConfig(loraWanPage.getConfig());
      return true;
   }

   /**
    * Returns filled object creation object for Sensor object
    * 
    * @return
    */
   public NXCObjectCreationData getCreationData()
   {
      return cd;
   }
}
