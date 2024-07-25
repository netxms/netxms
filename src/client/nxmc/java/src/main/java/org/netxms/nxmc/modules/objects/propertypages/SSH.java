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

import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.SshKeyPair;
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
 * "SSH" property page for node
 */
public class SSH extends ObjectPropertyPage
{
   private I18n i18n = LocalizationHelper.getI18n(SSH.class);

   private AbstractNode node;
   private ObjectSelector sshProxy;
   private LabeledText sshLogin;
   private PasswordInputField sshPassword;
   private LabeledText sshPort;
   private Combo sshKey;
   private List<SshKeyPair> keyList;
   private NXCSession session;

   /**
    * Create new page.
    *
    * @param object object to edit
    */
   public SSH(AbstractObject object)
   {
      super(LocalizationHelper.getI18n(SSH.class).tr("SSH"), object);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "communication.ssh";
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
      session = Registry.getSession();

      Composite dialogArea = new Composite(parent, SWT.NONE);
      GridLayout dialogLayout = new GridLayout();
      dialogLayout.marginWidth = 0;
      dialogLayout.marginHeight = 0;
      dialogLayout.numColumns = 2;
      dialogLayout.makeColumnsEqualWidth = true;
      dialogArea.setLayout(dialogLayout);
      
      sshLogin = new LabeledText(dialogArea, SWT.NONE);
      sshLogin.setLabel(i18n.tr("Login"));
      sshLogin.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      sshLogin.setText(node.getSshLogin());
      
      sshPassword = new PasswordInputField(dialogArea, SWT.NONE);
      sshPassword.setLabel(i18n.tr("Password"));
      sshPassword.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      sshPassword.setText(node.getSshPassword());
      
      sshPort = new LabeledText(dialogArea, SWT.NONE);
      sshPort.setLabel(i18n.tr("Port"));
      sshPort.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      sshPort.setText(Integer.toString(node.getSshPort()));

      sshKey = WidgetHelper.createLabeledCombo(dialogArea, SWT.BORDER | SWT.READ_ONLY, 
            i18n.tr("Key"), new GridData(SWT.FILL, SWT.CENTER, true, false, 2, 1));

      sshProxy = new ObjectSelector(dialogArea, SWT.NONE, true);
      sshProxy.setLabel(i18n.tr("Proxy"));
      sshProxy.setEmptySelectionName(i18n.tr("<default>"));
      sshProxy.setObjectId(node.getSshProxyId());
      sshProxy.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false, 2, 1));
      loadSshKeyList();

      return dialogArea;
   }
   
   /**
    * Get user info and refresh view
    */
   private void loadSshKeyList()
   {
      Job job = new Job(i18n.tr("Reloading SSH key list"), null) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final List<SshKeyPair> list = session.getSshKeys(true);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  keyList = list;
                  sshKey.add("");   
                  int selection = 0;
                  for (int i = 0; i < keyList.size(); i++)
                  {
                     SshKeyPair d = keyList.get(i);
                     sshKey.add(d.getName());   
                     if (d.getId() == node.getSshKeyId())
                        selection = i + 1;
                  }      
                  sshKey.select(selection);              
               }
            });
         }
         
         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot get list of SSH keys");
         }
      };
      job.setUser(false);
      job.start();  
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
         md.setSshPort(Integer.parseInt(sshPort.getText(), 10));
      }
      catch(NumberFormatException e)
      {
         MessageDialog.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Please enter valid SSH port number"));
         if (isApply)
            setValid(true);
         return false;
      }
      
      int selection = sshKey.getSelectionIndex();
      if (selection > 0)
      {
         SshKeyPair d = keyList.get(selection - 1);
         md.setSshKeyId(d.getId());
      }
      else
      {
         md.setSshKeyId(0);
      }
         
      md.setSshProxy(sshProxy.getObjectId());
      md.setSshLogin(sshLogin.getText().trim());
      md.setSshPassword(sshPassword.getText());

      new Job(i18n.tr("Updating SSH settings for node {0}", node.getObjectName()), null, messageArea) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.modifyObject(md);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot update SSH settings for node {0}", node.getObjectName());
         }

         @Override
         protected void jobFinalize()
         {
            if (isApply)
               runInUIThread(() -> SSH.this.setValid(true));
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
      sshProxy.setObjectId(0);
      sshLogin.setText("netxms");
      sshPassword.setText("");
      sshPort.setText("22");
   }
}
