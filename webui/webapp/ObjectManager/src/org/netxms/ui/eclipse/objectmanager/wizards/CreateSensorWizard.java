/**
 * NetXMS - open source network management system
 * Copyright (C) 2017 Raden Solutions
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
   private SensorGeneralPage first;
   private SensorLoraWan lora;
   
   /**
    * Wizard constructor. Also creates NXCObjectCreationData for new object. 
    * 
    * @param parentId parent object ID in object tree
    */
   public CreateSensorWizard(long parentId)
   {
      super();
      cd = new NXCObjectCreationData(AbstractObject.OBJECT_SENSOR,  "", parentId);
   }
   
   @Override
   public String getWindowTitle() {
       return "Create sensor";
   }
   
   @Override
   public void addPages() {
       first = new SensorGeneralPage();
       lora = new SensorLoraWan();
       
       
       addPage(first);
       addPage(lora);
   }
   
   @Override
   public IWizardPage getNextPage(IWizardPage currentPage) {
      if(currentPage == first)
      {
         if (first.getCommMethod() == Sensor.COMM_LORAWAN) {
            return lora;
         }
      }
      return null;
   }
   
   @Override
   public boolean performFinish()
   {
      cd.setName(first.getObjectName());
      cd.setCommProtocol(first.getCommMethod());
      SensorCommon commonData =  first.getCommonData();
      cd.setMacAddress(commonData.getMacAddress());
      cd.setDeviceClass(commonData.getDeviceClass());
      cd.setVendor(commonData.getVendor());
      cd.setSerialNumber(commonData.getSerial());
      cd.setMetaType(commonData.getMetaType());
      cd.setDeviceAddress(commonData.getDeviceAddress());
      cd.setDescription(commonData.getDescription());
      cd.setSensorProxy(commonData.getProxyNode());
      if(cd.getCommProtocol() == Sensor.COMM_LORAWAN)
         cd.setXmlRegConfig(lora.getRegConfig());
      if(cd.getCommProtocol() == Sensor.COMM_LORAWAN)
         cd.setXmlConfig(lora.getConfig());
      return true;
   }

   /**
    * Returns filled object creation object for Sensor object
    * @return
    */
   public NXCObjectCreationData getCreationData()
   {
      return cd;
   }

}
