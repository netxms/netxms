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
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractNode;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectbrowser.widgets.ObjectSelector;
import org.netxms.ui.eclipse.objectmanager.Activator;
import org.netxms.ui.eclipse.objectmanager.Messages;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * "Agent" property page for node
 */
public class Agent extends PropertyPage
{
   private AbstractNode node;
   private LabeledText agentPort;
   private LabeledText agentSharedSecret;
   private Combo agentAuthMethod;
   private Button agentForceEncryption;
   private ObjectSelector agentProxy;

   /* (non-Javadoc)
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      node = (AbstractNode)getElement().getAdapter(AbstractNode.class);

      Composite dialogArea = new Composite(parent, SWT.NONE);
      FormLayout dialogLayout = new FormLayout();
      dialogLayout.marginWidth = 0;
      dialogLayout.marginHeight = 0;
      dialogLayout.spacing = WidgetHelper.DIALOG_SPACING;
      dialogArea.setLayout(dialogLayout);

      agentPort = new LabeledText(dialogArea, SWT.NONE);
      agentPort.setLabel(Messages.get().Communication_TCPPort);
      agentPort.setText(Integer.toString(node.getAgentPort()));
      FormData fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(0, 0);
      agentPort.setLayoutData(fd);
      
      agentProxy = new ObjectSelector(dialogArea, SWT.NONE, true);
      agentProxy.setLabel(Messages.get().Communication_Proxy);
      agentProxy.setObjectId(node.getAgentProxyId());
      fd = new FormData();
      fd.left = new FormAttachment(agentPort, 0, SWT.RIGHT);
      fd.right = new FormAttachment(100, 0);
      fd.top = new FormAttachment(0, 0);
      agentProxy.setLayoutData(fd);

      agentForceEncryption = new Button(dialogArea, SWT.CHECK);
      agentForceEncryption.setText(Messages.get().Communication_ForceEncryption);
      agentForceEncryption.setSelection((node.getFlags() & AbstractNode.NF_FORCE_ENCRYPTION) != 0);
      fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(agentPort, 0, SWT.BOTTOM);
      agentForceEncryption.setLayoutData(fd);
      
      fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(agentForceEncryption, 0, SWT.BOTTOM);
      agentAuthMethod = WidgetHelper.createLabeledCombo(dialogArea, SWT.BORDER | SWT.READ_ONLY, Messages.get().Communication_AuthMethod, fd);
      agentAuthMethod.add(Messages.get().Communication_AuthNone);
      agentAuthMethod.add(Messages.get().Communication_AuthPlain);
      agentAuthMethod.add(Messages.get().Communication_AuthMD5);
      agentAuthMethod.add(Messages.get().Communication_AuthSHA1);
      agentAuthMethod.select(node.getAgentAuthMethod());
      agentAuthMethod.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }

         @Override
         public void widgetSelected(SelectionEvent e)
         {
            agentSharedSecret.getTextControl().setEnabled(agentAuthMethod.getSelectionIndex() != AbstractNode.AGENT_AUTH_NONE);
         }
      });
      
      agentSharedSecret = new LabeledText(dialogArea, SWT.NONE);
      agentSharedSecret.setLabel(Messages.get().Communication_SharedSecret);
      agentSharedSecret.setText(node.getAgentSharedSecret());
      fd = new FormData();
      fd.left = new FormAttachment(agentAuthMethod.getParent(), 0, SWT.RIGHT);
      fd.right = new FormAttachment(100, 0);
      fd.top = new FormAttachment(agentForceEncryption, 0, SWT.BOTTOM);
      agentSharedSecret.setLayoutData(fd);
      agentSharedSecret.getTextControl().setEnabled(node.getAgentAuthMethod() != AbstractNode.AGENT_AUTH_NONE);
      
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
         md.setAgentPort(Integer.parseInt(agentPort.getText(), 10));
      }
      catch(NumberFormatException e)
      {
         MessageDialog.openWarning(getShell(), Messages.get().Communication_Warning, Messages.get().Communication_WarningInvalidAgentPort);
         if (isApply)
            setValid(true);
         return false;
      }
      md.setAgentProxy(agentProxy.getObjectId());
      md.setAgentAuthMethod(agentAuthMethod.getSelectionIndex());
      md.setAgentSecret(agentSharedSecret.getText());
      
      /* TODO: sync in some way with "Polling" page */
      int flags = node.getFlags();
      if (agentForceEncryption.getSelection())
         flags |= AbstractNode.NF_FORCE_ENCRYPTION;
      else
         flags &= ~AbstractNode.NF_FORCE_ENCRYPTION;
      md.setObjectFlags(flags);

      final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
      new ConsoleJob(String.format("Updating agent communication settings for node %s", node.getObjectName()), null, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.modifyObject(md);
         }

         @Override
         protected String getErrorMessage()
         {
            return String.format("Cannot update communication settings for node %s", node.getObjectName());
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
                     Agent.this.setValid(true);
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
      
      agentPort.setText("4700"); //$NON-NLS-1$
      agentForceEncryption.setSelection(false);
      agentAuthMethod.select(0);
      agentProxy.setObjectId(0);
      agentSharedSecret.setText(""); //$NON-NLS-1$
      agentSharedSecret.getTextControl().setEnabled(false);
   }
}
