/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
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
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Node;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Expected Capabilities" property page for node object
 */
public class ExpectedCapabilities extends ObjectPropertyPage
{
   private I18n i18n = LocalizationHelper.getI18n(ExpectedCapabilities.class);

   private Node node;
   private Button checkboxAgent;
   private Button checkboxSNMP;
   private Button checkboxEtherNetIP;
   private Button checkboxModbus;
   private Button checkboxSSH;

   /**
    * Create new page.
    *
    * @param object object to edit
    */
   public ExpectedCapabilities(AbstractObject object)
   {
      super(LocalizationHelper.getI18n(ExpectedCapabilities.class).tr("Expected Capabilities"), object);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "expected-capabilities";
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#isVisible()
    */
   @Override
   public boolean isVisible()
   {
      return object instanceof Node;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 22;
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
      Composite dialogArea = new Composite(parent, SWT.NONE);
		
      node = (Node)object;

		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
      dialogArea.setLayout(layout);

      checkboxAgent = new Button(dialogArea, SWT.CHECK);
      checkboxAgent.setText(i18n.tr("NetXMS &agent"));
      checkboxAgent.setSelection((node.getExpectedCapabilities() & Node.NC_IS_NATIVE_AGENT) != 0);

      checkboxSNMP = new Button(dialogArea, SWT.CHECK);
      checkboxSNMP.setText(i18n.tr("&SNMP"));
      checkboxSNMP.setSelection((node.getExpectedCapabilities() & Node.NC_IS_SNMP) != 0);

      checkboxEtherNetIP = new Button(dialogArea, SWT.CHECK);
      checkboxEtherNetIP.setText(i18n.tr("&EtherNet/IP"));
      checkboxEtherNetIP.setSelection((node.getExpectedCapabilities() & Node.NC_IS_ETHERNET_IP) != 0);

      checkboxModbus = new Button(dialogArea, SWT.CHECK);
      checkboxModbus.setText(i18n.tr("&Modbus TCP"));
      checkboxModbus.setSelection((node.getExpectedCapabilities() & Node.NC_IS_MODBUS_TCP) != 0);

      checkboxSSH = new Button(dialogArea, SWT.CHECK);
      checkboxSSH.setText(i18n.tr("&SSH"));
      checkboxSSH.setSelection((node.getExpectedCapabilities() & Node.NC_IS_SSH) != 0);

		return dialogArea;
	}

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(boolean isApply)
	{
		if (isApply)
			setValid(false);

      long capabilities = 0;
      if (checkboxAgent.getSelection())
         capabilities |= Node.NC_IS_NATIVE_AGENT;
      if (checkboxSNMP.getSelection())
         capabilities |= Node.NC_IS_SNMP;
      if (checkboxEtherNetIP.getSelection())
         capabilities |= Node.NC_IS_ETHERNET_IP;
      if (checkboxModbus.getSelection())
         capabilities |= Node.NC_IS_MODBUS_TCP;
      if (checkboxSSH.getSelection())
         capabilities |= Node.NC_IS_SSH;

		final NXCObjectModificationData md = new NXCObjectModificationData(object.getObjectId());
      md.setExpectedCapabilities(capabilities);

      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Updating node expected capabilities"), null, messageArea) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
			{
				session.modifyObject(md);
			}

			@Override
			protected void jobFinalize()
			{
				if (isApply)
               runInUIThread(() -> ExpectedCapabilities.this.setValid(true));
			}

			@Override
			protected String getErrorMessage()
			{
            return i18n.tr("Cannot update node expected capabilities");
			}
		}.start();

      return true;
	}
}
