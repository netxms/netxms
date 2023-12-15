/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Chassis;
import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.DataCollectionTarget;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.widgets.ObjectSelector;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Communication" property page for nodes
 */
public class Communication extends ObjectPropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(Communication.class);

   private AbstractNode node;
	private LabeledText primaryName;
   private Button externalGateway;
   private Button enablePingOnPrimaryIP;
	private boolean primaryNameChanged = false;
   private Chassis chassis;
   private ObjectSelector controllerNode;
   private Button bindUnderController;

   /**
    * Create page.
    *
    * @param object object to edit
    */
   public Communication(AbstractObject object)
   {
      super(LocalizationHelper.getI18n(Communication.class).tr("Communication"), object);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "communication";
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 10;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#isVisible()
    */
   @Override
   public boolean isVisible()
   {
      return (object instanceof DataCollectionTarget) && !(object instanceof Cluster);
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
      Composite dialogArea = new Composite(parent, SWT.NONE);
      GridLayout dialogLayout = new GridLayout();
      dialogLayout.marginWidth = 0;
      dialogLayout.marginHeight = 0;
      dialogArea.setLayout(dialogLayout);

      if (object instanceof AbstractNode)
      {
         node = (AbstractNode)object;

         primaryName = new LabeledText(dialogArea, SWT.NONE);
         primaryName.setLabel(i18n.tr("Primary host name"));
         primaryName.setText(node.getPrimaryName());
         GridData gd = new GridData();
         gd.horizontalAlignment = SWT.FILL;
         gd.grabExcessHorizontalSpace = true;
         primaryName.setLayoutData(gd);
         primaryName.getTextControl().addModifyListener(new ModifyListener() {
            @Override
            public void modifyText(ModifyEvent e)
            {
               primaryNameChanged = true;
            }
         });

         externalGateway = new Button(dialogArea, SWT.CHECK);
         externalGateway.setText(i18n.tr("Communication through external gateway"));
         externalGateway.setSelection((node.getFlags() & AbstractNode.NF_EXTERNAL_GATEWAY) != 0);
         gd = new GridData();
         gd.horizontalAlignment = SWT.FILL;
         gd.grabExcessHorizontalSpace = true;
         externalGateway.setLayoutData(gd);

         enablePingOnPrimaryIP = new Button(dialogArea, SWT.CHECK);
         enablePingOnPrimaryIP.setText(i18n.tr("Use ICMP ping on primary IP address to determine node status"));
         enablePingOnPrimaryIP.setSelection(node.isPingOnPrimaryIPEnabled());
         gd = new GridData();
         gd.horizontalAlignment = SWT.FILL;
         gd.grabExcessHorizontalSpace = true;
         enablePingOnPrimaryIP.setLayoutData(gd);
      }
      else if (object instanceof Chassis)
      {
         chassis = (Chassis)object;

         controllerNode = new ObjectSelector(dialogArea, SWT.NONE, true);
         controllerNode.setLabel(i18n.tr("Controller"));
         controllerNode.setObjectId(chassis.getControllerId());
         GridData gd = new GridData();
         gd.horizontalAlignment = SWT.FILL;
         gd.grabExcessHorizontalSpace = true;
         controllerNode.setLayoutData(gd);

         bindUnderController = new Button(dialogArea, SWT.CHECK);
         bindUnderController.setText(i18n.tr("&Bind chassis object under controller node object"));
         bindUnderController.setSelection((chassis.getFlags() & Chassis.CHF_BIND_UNDER_CONTROLLER) != 0);
         gd = new GridData();
         gd.horizontalAlignment = SWT.FILL;
         gd.grabExcessHorizontalSpace = true;
         bindUnderController.setLayoutData(gd);
      }

		return dialogArea;
	}

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(boolean isApply)
	{
      if (node != null)
         return applyNodeChanges(isApply);
      if (chassis != null)
         return applyChassisChanges(isApply);
      return true;
   }

   /**
    * Apply changes for node object.
    *
    * @param isApply true if "Apply" button was pressed
    * @return true if dialog can be closed
    */
   private boolean applyNodeChanges(final boolean isApply)
   {
      final NXCObjectModificationData md = new NXCObjectModificationData(node.getObjectId());

      if (primaryNameChanged)
      {
         // Validate primary name
         final String hostName = primaryName.getText().trim();
         if (!hostName.matches("^(([A-Za-z0-9_-]+\\.)*[A-Za-z0-9_-]+|[A-Fa-f0-9:]+)$")) //$NON-NLS-1$
         {
            MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"),
                  String.format(i18n.tr("String \"%s\" is not a valid host name or IP address. Please enter valid host name or IP address as primary host name."), hostName));
            return false;
         }
         md.setPrimaryName(hostName);
      }

      if (isApply)
         setValid(false);

      int flags = node.getFlags();
      if (externalGateway.getSelection())
         flags |= AbstractNode.NF_EXTERNAL_GATEWAY;
      else
         flags &= ~AbstractNode.NF_EXTERNAL_GATEWAY;
      if (enablePingOnPrimaryIP.getSelection())
         flags |= AbstractNode.NF_PING_PRIMARY_IP;
      else
         flags &= ~AbstractNode.NF_PING_PRIMARY_IP;
      md.setObjectFlags(flags, AbstractNode.NF_EXTERNAL_GATEWAY | AbstractNode.NF_PING_PRIMARY_IP);

      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Updating communication settings for node {0}", node.getObjectName()), null, messageArea) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.modifyObject(md);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot update communication settings");
         }

         @Override
         protected void jobFinalize()
         {
            if (isApply)
               runInUIThread(() -> Communication.this.setValid(true));
         }
      }.start();
      return true;
   }

   /**
    * Apply changes for chassis object.
    *
    * @param isApply true if "Apply" button was pressed
    * @return true if dialog can be closed
    */
   private boolean applyChassisChanges(final boolean isApply)
   {
      final NXCObjectModificationData md = new NXCObjectModificationData(chassis.getObjectId());

      if (isApply)
         setValid(false);

      int flags = chassis.getFlags();
      if (bindUnderController.getSelection())
         flags |= Chassis.CHF_BIND_UNDER_CONTROLLER;
      else
         flags &= ~Chassis.CHF_BIND_UNDER_CONTROLLER;
      md.setObjectFlags(flags, Chassis.CHF_BIND_UNDER_CONTROLLER);

      md.setControllerId(controllerNode.getObjectId());

      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Updating communication settings for chassis {0}", chassis.getObjectName()), null, messageArea) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.modifyObject(md);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot update communication settings");
         }

         @Override
         protected void jobFinalize()
         {
            if (isApply)
               runInUIThread(() -> Communication.this.setValid(true));
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
      if (node != null)
      {
         externalGateway.setSelection(false);
         enablePingOnPrimaryIP.setSelection(false);
      }
	}
}
