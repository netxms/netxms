/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2016 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.objectmanager.propertypages;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractNode;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectbrowser.widgets.ObjectSelector;
import org.netxms.ui.eclipse.objectmanager.Activator;
import org.netxms.ui.eclipse.objectmanager.Messages;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * "SSH" property page for node
 */
public class SSH extends PropertyPage
{
   private AbstractNode node;
   private ObjectSelector sshProxy;
   private LabeledText sshLogin;
   private LabeledText sshPassword;

   /* (non-Javadoc)
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      node = (AbstractNode)getElement().getAdapter(AbstractNode.class);

      Composite dialogArea = new Composite(parent, SWT.NONE);
      GridLayout dialogLayout = new GridLayout();
      dialogLayout.marginWidth = 0;
      dialogLayout.marginHeight = 0;
      dialogLayout.numColumns = 2;
      dialogLayout.makeColumnsEqualWidth = true;
      dialogArea.setLayout(dialogLayout);
      
      sshLogin = new LabeledText(dialogArea, SWT.NONE);
      sshLogin.setLabel("Login");
      sshLogin.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      sshLogin.setText(node.getSshLogin());
      
      sshPassword = new LabeledText(dialogArea, SWT.NONE);
      sshPassword.setLabel("Password");
      sshPassword.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      sshPassword.setText(node.getSshPassword());
      
      sshProxy = new ObjectSelector(dialogArea, SWT.NONE, true);
      sshProxy.setLabel(Messages.get().Communication_Proxy);
      sshProxy.setEmptySelectionName("<default>");
      sshProxy.setObjectId(node.getSshProxyId());
      sshProxy.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false, 2, 1));
      
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
      
      md.setSshProxy(sshProxy.getObjectId());
      md.setSshLogin(sshLogin.getText().trim());
      md.setSshPassword(sshPassword.getText());

      final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
      new ConsoleJob(String.format("Updating SSH settings for node %s", node.getObjectName()), null, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.modifyObject(md);
         }

         @Override
         protected String getErrorMessage()
         {
            return String.format("Cannot update SSH settings for node %s", node.getObjectName());
         }

         @Override
         protected void jobFinalize()
         {
            if (isApply)
            {
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     SSH.this.setValid(true);
                  }
               });
            }
         }
      }.start();
      return true;
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.preference.PreferencePage#performOk()
    */
   @Override
   public boolean performOk()
   {
      return applyChanges(false);
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.preference.PreferencePage#performApply()
    */
   @Override
   protected void performApply()
   {
      applyChanges(true);
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.preference.PreferencePage#performDefaults()
    */
   @Override
   protected void performDefaults()
   {
      super.performDefaults();
      sshProxy.setObjectId(0);
      sshLogin.setText("netxms");
      sshPassword.setText("");
   }
}
