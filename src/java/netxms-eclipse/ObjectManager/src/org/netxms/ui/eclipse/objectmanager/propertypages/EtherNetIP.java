/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Victor Kirhenshtein
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
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * "EtherNet/IP" property page for node
 */
public class EtherNetIP extends PropertyPage
{
   private AbstractNode node;
   private LabeledText tcpPort;
   private ObjectSelector proxy;

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
      dialogLayout.horizontalSpacing = WidgetHelper.DIALOG_SPACING;
      dialogLayout.numColumns = 2;
      dialogArea.setLayout(dialogLayout);

      tcpPort = new LabeledText(dialogArea, SWT.NONE);
      tcpPort.setLabel(Messages.get().Communication_TCPPort);
      tcpPort.setText(Integer.toString(node.getEtherNetIpPort()));
      GridData gd = new GridData();
      gd.widthHint = 100;
      tcpPort.setLayoutData(gd);
      
      proxy = new ObjectSelector(dialogArea, SWT.NONE, true);
      proxy.setLabel(Messages.get().Communication_Proxy);
      proxy.setObjectId(node.getEtherNetIpProxyId());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      proxy.setLayoutData(gd);

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
         md.setEtherNetIPPort(Integer.parseInt(tcpPort.getText(), 10));
      }
      catch(NumberFormatException e)
      {
         MessageDialog.openWarning(getShell(), Messages.get().Communication_Warning,
               Messages.get().Communication_WarningInvalidEtherNetIPPort);
         if (isApply)
            setValid(true);
         return false;
      }
      md.setEtherNetIPProxy(proxy.getObjectId());

      final NXCSession session = ConsoleSharedData.getSession();
      new ConsoleJob(String.format("Updating EtherNet/IP communication settings for node %s", node.getObjectName()), null,
            Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.modifyObject(md);
         }

         @Override
         protected String getErrorMessage()
         {
            return String.format("Cannot update EtherNet/IP communication settings for node %s", node.getObjectName());
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
                     EtherNetIP.this.setValid(true);
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
      
      tcpPort.setText("44818"); //$NON-NLS-1$
      proxy.setObjectId(0);
   }
}
