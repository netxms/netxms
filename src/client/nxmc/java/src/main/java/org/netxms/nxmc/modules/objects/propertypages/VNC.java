/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.base.widgets.PasswordInputField;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.widgets.ObjectSelector;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "VNC" property page for node
 */
public class VNC extends ObjectPropertyPage
{
   private I18n i18n = LocalizationHelper.getI18n(VNC.class);

   private AbstractNode node;
   private ObjectSelector vncProxy;
   private LabeledText vncPort;
   private PasswordInputField vncPassword;

   /**
    * Create new page.
    *
    * @param object object to edit
    */
   public VNC(AbstractObject object)
   {
      super(LocalizationHelper.getI18n(VNC.class).tr("VNC"), object);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "communication.vnc";
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
      dialogLayout.numColumns = 2;
      dialogArea.setLayout(dialogLayout);

      vncPort = new LabeledText(dialogArea, SWT.NONE);
      vncPort.setLabel("Port");
      GridData gd = new GridData(SWT.FILL, SWT.CENTER, false, false);
      gd.widthHint = WidgetHelper.getTextWidth(vncPort, "00000") * 2;
      vncPort.setLayoutData(gd);
      vncPort.setText(Integer.toString(node.getVncPort()));
      vncPort.getTextControl().setTextLimit(5);

      vncPassword = new PasswordInputField(dialogArea, SWT.NONE);
      vncPassword.setLabel(i18n.tr("Password"));
      vncPassword.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      vncPassword.setText(node.getVncPassword());

      vncProxy = new ObjectSelector(dialogArea, SWT.NONE, true);
      vncProxy.setLabel(i18n.tr("Proxy"));
      vncProxy.setEmptySelectionName("<default>");
      vncProxy.setObjectId(node.getVncProxyId());
      vncProxy.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false, 2, 1));

      return dialogArea;
   }

   /**
    * Apply changes
    * 
    * @param isApply true if update operation caused by "Apply" button
    */
   protected boolean applyChanges(final boolean isApply)
   {
      final NXCObjectModificationData md = new NXCObjectModificationData(node.getObjectId());

      if (isApply)
         setValid(false);

      try
      {
         md.setVncPort(Integer.parseInt(vncPort.getText(), 10));
      }
      catch(NumberFormatException e)
      {
         MessageDialog.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Please enter valid VNC port number"));
         if (isApply)
            setValid(true);
         return false;
      }

      md.setVncProxy(vncProxy.getObjectId());
      md.setVncPassword(vncPassword.getText());

      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Updating VNC settings for node {0}", node.getObjectName()), null, messageArea) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.modifyObject(md);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot update VNC settings for node {0}", node.getObjectName());
         }

         @Override
         protected void jobFinalize()
         {
            if (isApply)
               runInUIThread(() -> VNC.this.setValid(true));
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
      vncProxy.setObjectId(0);
      vncPassword.setText("");
      vncPort.setText("5900");
   }
}
