/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.snmp.SnmpVersion;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.base.widgets.PasswordInputField;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.widgets.ObjectSelector;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "SNMP" property page for node
 */
public class SNMP extends ObjectPropertyPage
{
   private I18n i18n = LocalizationHelper.getI18n(SNMP.class);

   private AbstractNode node;
   private Combo snmpVersion;
   private LabeledText snmpPort;
   private Combo snmpAuth;
   private Combo snmpPriv;
   private ObjectSelector snmpProxy;
   private PasswordInputField snmpAuthName;
   private PasswordInputField snmpAuthPassword;
   private PasswordInputField snmpPrivPassword;
   private LabeledText snmpCodepage;
   private Button snmpSettingsLocked;

   /**
    * Create new page.
    *
    * @param object object to edit
    */
   public SNMP(AbstractObject object)
   {
      super(LocalizationHelper.getI18n(SNMP.class).tr("SNMP"), object);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "communication.snmp";
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
      FormLayout dialogLayout = new FormLayout();
      dialogLayout.marginWidth = 0;
      dialogLayout.marginHeight = 0;
      dialogLayout.spacing = WidgetHelper.DIALOG_SPACING;
      dialogArea.setLayout(dialogLayout);

      FormData fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(0, 0);
      snmpVersion = WidgetHelper.createLabeledCombo(dialogArea, SWT.BORDER | SWT.READ_ONLY, i18n.tr("Version"), fd);
      snmpVersion.add("1"); //$NON-NLS-1$
      snmpVersion.add("2c"); //$NON-NLS-1$
      snmpVersion.add("3"); //$NON-NLS-1$
      snmpVersion.select(snmpVersionToIndex(node.getSnmpVersion()));
      snmpVersion.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }

         @Override
         public void widgetSelected(SelectionEvent e)
         {
            onSnmpVersionChange();
         }
      });
      
      snmpPort = new LabeledText(dialogArea, SWT.NONE);
      snmpPort.setLabel(i18n.tr("UDP Port"));
      snmpPort.setText(Integer.toString(node.getSnmpPort()));

      fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(snmpVersion.getParent(), 0, SWT.BOTTOM);
      snmpAuth = WidgetHelper.createLabeledCombo(dialogArea, SWT.BORDER | SWT.READ_ONLY, i18n.tr("Authentication"), fd);
      snmpAuth.add(i18n.tr("NONE"));
      snmpAuth.add(i18n.tr("MD5"));
      snmpAuth.add(i18n.tr("SHA1"));
      snmpAuth.add("SHA224");
      snmpAuth.add("SHA256");
      snmpAuth.add("SHA384");
      snmpAuth.add("SHA512");
      snmpAuth.select(node.getSnmpAuthMethod());
      snmpAuth.setEnabled(node.getSnmpVersion() == SnmpVersion.V3);

      fd = new FormData();
      fd.left = new FormAttachment(snmpAuth.getParent(), 0, SWT.RIGHT);
      fd.top = new FormAttachment(snmpVersion.getParent(), 0, SWT.BOTTOM);
      snmpPriv = WidgetHelper.createLabeledCombo(dialogArea, SWT.BORDER | SWT.READ_ONLY, i18n.tr("Encryption"), fd);
      snmpPriv.add(i18n.tr("NONE"));
      snmpPriv.add("DES");
      snmpPriv.add("AES-128");
      snmpPriv.add("AES-192");
      snmpPriv.add("AES-256");
      snmpPriv.select(node.getSnmpPrivMethod());
      snmpPriv.setEnabled(node.getSnmpVersion() == SnmpVersion.V3);
      
      snmpProxy = new ObjectSelector(dialogArea, SWT.NONE, true);
      snmpProxy.setLabel(i18n.tr("Proxy"));
      snmpProxy.setObjectId(node.getSnmpProxyId());
      fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(snmpAuth.getParent(), 0, SWT.BOTTOM);
      fd.right = new FormAttachment(snmpPriv.getParent(), 0, SWT.RIGHT);
      snmpProxy.setLayoutData(fd);
      
      snmpAuthName = new PasswordInputField(dialogArea, SWT.NONE);
      snmpAuthName.setLabel((node.getSnmpVersion() == SnmpVersion.V3) ? i18n.tr("User name") : i18n.tr("Community string"));
      snmpAuthName.setText(node.getSnmpAuthName());
      fd = new FormData();
      fd.left = new FormAttachment(snmpProxy, 0, SWT.RIGHT);
      fd.top = new FormAttachment(0, 0);
      fd.right = new FormAttachment(100, 0);
      snmpAuthName.setLayoutData(fd);

      snmpAuthPassword = new PasswordInputField(dialogArea, SWT.NONE);
      snmpAuthPassword.setLabel(i18n.tr("Authentication password"));
      snmpAuthPassword.setText(node.getSnmpAuthPassword());
      fd = new FormData();
      fd.left = new FormAttachment(snmpAuthName, 0, SWT.LEFT);
      fd.top = new FormAttachment(snmpAuth.getParent(), 0, SWT.TOP);
      fd.right = new FormAttachment(100, 0);
      snmpAuthPassword.setLayoutData(fd);
      snmpAuthPassword.setInputControlsEnabled(node.getSnmpVersion() == SnmpVersion.V3);

      snmpPrivPassword = new PasswordInputField(dialogArea, SWT.NONE);
      snmpPrivPassword.setLabel(i18n.tr("Encryption password"));
      snmpPrivPassword.setText(node.getSnmpPrivPassword());
      fd = new FormData();
      fd.left = new FormAttachment(snmpAuthName, 0, SWT.LEFT);
      fd.top = new FormAttachment(snmpProxy, 0, SWT.TOP);
      fd.right = new FormAttachment(100, 0);
      snmpPrivPassword.setLayoutData(fd);
      snmpPrivPassword.setInputControlsEnabled(node.getSnmpVersion() == SnmpVersion.V3);

      fd = new FormData();
      fd.left = new FormAttachment(snmpVersion.getParent(), 0, SWT.RIGHT);
      fd.right = new FormAttachment(snmpAuthName, 0, SWT.LEFT);
      fd.top = new FormAttachment(0, 0);
      snmpPort.setLayoutData(fd);

      snmpCodepage = new LabeledText(dialogArea, SWT.NONE);
      snmpCodepage.setLabel(i18n.tr("Codepage"));
      snmpCodepage.setText(node.getSNMPCodepage());
      fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.right = new FormAttachment(100, 0);
      fd.top = new FormAttachment(snmpProxy, 0, SWT.BOTTOM);
      snmpCodepage.setLayoutData(fd);

      snmpSettingsLocked = new Button(dialogArea, SWT.CHECK);
      snmpSettingsLocked.setText(i18n.tr("&Prevent automatic SNMP configuration changes"));
      snmpSettingsLocked.setSelection(node.isSnmpSettingsLocked());
      fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.right = new FormAttachment(100, 0);
      fd.top = new FormAttachment(snmpCodepage, 0, SWT.BOTTOM);
      snmpSettingsLocked.setLayoutData(fd);

      return dialogArea;
   }

   /**
    * Convert SNMP version to index in combo box
    * 
    * @param version SNMP version
    * @return index in combo box
    */
   private int snmpVersionToIndex(SnmpVersion version)
   {
      switch(version)
      {
         case V1:
            return 0;
         case V2C:
            return 1;
         case V3:
            return 2;
         default:
            return 0;
      }
   }
   
   /**
    * Convert selection index in SNMP version combo box to SNMP version
    */
   private SnmpVersion snmpIndexToVersion(int index)
   {
      final SnmpVersion[] versions = { SnmpVersion.V1, SnmpVersion.V2C, SnmpVersion.V3 };
      return versions[index];
   }
   
   /**
    * Handler for SNMP version change
    */
   private void onSnmpVersionChange()
   {
      boolean isV3 = (snmpVersion.getSelectionIndex() == 2);
      snmpAuthName.setLabel(isV3 ? i18n.tr("User name") : i18n.tr("Community string"));
      snmpAuth.setEnabled(isV3);
      snmpPriv.setEnabled(isV3);
      snmpAuthPassword.setInputControlsEnabled(isV3);
      snmpPrivPassword.setInputControlsEnabled(isV3);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(final boolean isApply)
   {
      final NXCObjectModificationData md = new NXCObjectModificationData(node.getObjectId());

      if (isApply)
         setValid(false);

      md.setSnmpVersion(snmpIndexToVersion(snmpVersion.getSelectionIndex()));
      try
      {
         md.setSnmpPort(Integer.parseInt(snmpPort.getText(), 10));
      }
      catch(NumberFormatException e)
      {
         MessageDialog.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Please enter valid SNMP port number"));
         if (isApply)
            setValid(true);
         return false;
      }
      md.setSnmpProxy(snmpProxy.getObjectId());
      md.setSnmpAuthentication(snmpAuthName.getText(), snmpAuth.getSelectionIndex(), snmpAuthPassword.getText(), snmpPriv.getSelectionIndex(), snmpPrivPassword.getText());
      md.setSNMPCodepage(snmpCodepage.getText());

      int flags = node.getFlags();
      if (snmpSettingsLocked.getSelection())
         flags |= AbstractNode.NF_SNMP_SETTINGS_LOCKED;
      else
         flags &= ~AbstractNode.NF_SNMP_SETTINGS_LOCKED;
      md.setObjectFlags(flags, AbstractNode.NF_SNMP_SETTINGS_LOCKED);

      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Updating SNMP settings for node {0}", node.getObjectName()), null, messageArea) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.modifyObject(md);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot update SNMP settings for node {0}", node.getObjectName());
         }

         @Override
         protected void jobFinalize()
         {
            if (isApply)
               runInUIThread(() -> SNMP.this.setValid(true));
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
      
      snmpVersion.select(0);
      snmpAuth.select(0);
      snmpPriv.select(0);
      snmpAuthName.setText("public");
      snmpAuthPassword.setText("");
      snmpPrivPassword.setText("");
      snmpProxy.setObjectId(0);
      onSnmpVersionChange();
      snmpCodepage.setText("");
   }
}
