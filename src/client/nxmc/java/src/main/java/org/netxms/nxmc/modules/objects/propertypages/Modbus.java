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
package org.netxms.nxmc.modules.objects.propertypages;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.LabeledSpinner;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.widgets.ObjectSelector;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Modbus" property page for node
 */
public class Modbus extends ObjectPropertyPage
{
   private I18n i18n = LocalizationHelper.getI18n(Modbus.class);

   private AbstractNode node;
   private LabeledSpinner tcpPort;
   private LabeledSpinner unitId;
   private ObjectSelector proxy;

   /**
    * Create new page.
    *
    * @param object object to edit
    */
   public Modbus(AbstractObject object)
   {
      super(LocalizationHelper.getI18n(Modbus.class).tr("Modbus"), object);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "communication.modbus";
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getParentId()
    */
   @Override
   public String getParentId()
   {
      return "communication";
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#isVisible()
    */
   @Override
   public boolean isVisible()
   {
      return object instanceof AbstractNode;
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      node = (AbstractNode)object;

      Composite dialogArea = new Composite(parent, SWT.NONE);
      GridLayout dialogLayout = new GridLayout();
      dialogLayout.marginWidth = 0;
      dialogLayout.marginHeight = 0;
      dialogLayout.horizontalSpacing = WidgetHelper.DIALOG_SPACING;
      dialogLayout.numColumns = 2;
      dialogLayout.makeColumnsEqualWidth = true;
      dialogArea.setLayout(dialogLayout);

      tcpPort = new LabeledSpinner(dialogArea, SWT.NONE);
      tcpPort.setLabel(i18n.tr("TCP port"));
      tcpPort.setRange(1, 65535);
      tcpPort.setSelection(node.getModbusTcpPort());
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      tcpPort.setLayoutData(gd);

      unitId = new LabeledSpinner(dialogArea, SWT.NONE);
      unitId.setLabel(i18n.tr("Unit ID"));
      unitId.setRange(0, 255);
      unitId.setSelection(node.getModbusUnitId());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      unitId.setLayoutData(gd);

      proxy = new ObjectSelector(dialogArea, SWT.NONE, true);
      proxy.setLabel(i18n.tr("Proxy"));
      proxy.setObjectId(node.getModbusProxyId());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      proxy.setLayoutData(gd);

      return dialogArea;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(final boolean isApply)
   {
      if (isApply)
         setValid(false);

      final NXCObjectModificationData md = new NXCObjectModificationData(node.getObjectId());
      md.setModbusTcpPort(tcpPort.getSelection());
      md.setModbusUnitId((short)unitId.getSelection());
      md.setModbusProxy(proxy.getObjectId());

      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Updating Modbus communication settings for node {0}", node.getObjectName()), null, messageArea) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.modifyObject(md);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot update Modbus communication settings for node {0}", node.getObjectName());
         }

         @Override
         protected void jobFinalize()
         {
            if (isApply)
               runInUIThread(() -> Modbus.this.setValid(true));
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
      tcpPort.setText("502");
      unitId.setText("1");
      proxy.setObjectId(0);
   }
}
