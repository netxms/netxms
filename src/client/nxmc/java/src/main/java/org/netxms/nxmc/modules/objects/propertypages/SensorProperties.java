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
package org.netxms.nxmc.modules.objects.propertypages;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Sensor;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.widgets.CommonSensorAttributesEditor;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

public class SensorProperties extends ObjectPropertyPage
{
   private static I18n i18n = LocalizationHelper.getI18n(SensorProperties.class);

   private Sensor sensor;
   private CommonSensorAttributesEditor commonData;

   /**
    * Create new page.
    *
    * @param object object to edit
    */
   public SensorProperties(AbstractObject object)
   {
      super(i18n.tr("Sensor"), object);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "sensorProperties";
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 1;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#isVisible()
    */
   @Override
   public boolean isVisible()
   {
      return (object instanceof Sensor);
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      Composite dialogArea = new Composite(parent, SWT.NONE);      
      sensor = (Sensor)object;
      
      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
      layout.horizontalSpacing = WidgetHelper.DIALOG_SPACING;
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      dialogArea.setLayout(layout);
      
      commonData = new CommonSensorAttributesEditor(dialogArea, SWT.NONE, null, sensor.getMacAddress().toString(), sensor.getDeviceClass(), sensor.getVendor(),
                                    sensor.getSerialNumber(), sensor.getDeviceAddress(), sensor.getMetaType(), sensor.getDescription(), 
                                    sensor.getProxyId(), sensor.getCommProtocol());
      
      GridData gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      commonData.setLayoutData(gd);
      
      return dialogArea;
   }
   
   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(final boolean isApply)
   {
      if (!commonData.validate())
         return false;
      
      final NXCObjectModificationData md = new NXCObjectModificationData(sensor.getObjectId());
      md.setMacAddress(commonData.getMacAddress());
      md.setDeviceClass(commonData.getDeviceClass());
      md.setVendor(commonData.getVendor());
      md.setSerialNumber(commonData.getSerial());
      md.setDeviceAddress(commonData.getDeviceAddress());         
      md.setMetaType(commonData.getMetaType());         
      md.setDescription(commonData.getDescription());
      md.setSensorProxy(commonData.getProxyNode());
      
      if (isApply)
         setValid(false);
      
      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Updating sensor configuration"), null, messageArea) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.modifyObject(md);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot update sensor polling settings");
         }

         @Override
         protected void jobFinalize()
         {
            if (isApply)
               runInUIThread(() -> SensorProperties.this.setValid(true));
         }
      }.start();
      return true;
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performDefaults()
    */
   @Override
   protected void performDefaults()
   {
      super.performDefaults();
      commonData.updateFields("", 0, "", "", "", "", "");
   }
}
