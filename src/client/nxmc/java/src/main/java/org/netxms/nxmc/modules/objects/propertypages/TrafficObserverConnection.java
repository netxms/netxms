/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Victor Kirhenshtein
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
import org.netxms.client.objects.Node;
import org.netxms.client.objects.TrafficObserver;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.LabeledCombo;
import org.netxms.nxmc.base.widgets.LabeledSpinner;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.widgets.ObjectSelector;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Connection" property page for traffic observer objects
 */
public class TrafficObserverConnection extends ObjectPropertyPage
{
   private I18n i18n = LocalizationHelper.getI18n(TrafficObserverConnection.class);

   private TrafficObserver observer;
   private LabeledText connectorName;
   private LabeledText credentials;
   private ObjectSelector linkedNode;
   private LabeledCombo removalPolicy;
   private LabeledSpinner gracePeriod;

   /**
    * Create new page.
    *
    * @param object object to edit
    */
   public TrafficObserverConnection(AbstractObject object)
   {
      super(LocalizationHelper.getI18n(TrafficObserverConnection.class).tr("Connection"), object);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "trafficObserverConnection";
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#isVisible()
    */
   @Override
   public boolean isVisible()
   {
      return (object instanceof TrafficObserver);
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      Composite dialogArea = new Composite(parent, SWT.NONE);
      observer = (TrafficObserver)object;

      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      dialogArea.setLayout(layout);

      connectorName = new LabeledText(dialogArea, SWT.NONE, SWT.BORDER | SWT.READ_ONLY);
      connectorName.setLabel(i18n.tr("Connector"));
      connectorName.setText(observer.getConnectorName());
      connectorName.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      credentials = new LabeledText(dialogArea, SWT.NONE, SWT.BORDER | SWT.MULTI);
      credentials.setLabel(observer.hasCredentials()
            ? i18n.tr("Credentials (JSON; leave empty to keep current)")
            : i18n.tr("Credentials (JSON)"));
      GridData gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
      gd.heightHint = 120;
      credentials.setLayoutData(gd);

      linkedNode = new ObjectSelector(dialogArea, SWT.NONE, true);
      linkedNode.setLabel(i18n.tr("Node representing the analyzer itself (optional)"));
      linkedNode.setObjectClass(Node.class);
      linkedNode.setObjectId(observer.getLinkedNodeId());
      linkedNode.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      removalPolicy = new LabeledCombo(dialogArea, SWT.NONE);
      removalPolicy.setLabel(i18n.tr("Action when observation point disappears from discovery"));
      removalPolicy.add(i18n.tr("Mark as inactive"));
      removalPolicy.add(i18n.tr("Delete after grace period"));
      removalPolicy.add(i18n.tr("Delete immediately"));
      removalPolicy.add(i18n.tr("Ignore"));
      removalPolicy.select((observer.getRemovalPolicy() >= 0) && (observer.getRemovalPolicy() <= 3) ? observer.getRemovalPolicy() : 0);
      removalPolicy.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      gracePeriod = new LabeledSpinner(dialogArea, SWT.NONE);
      gracePeriod.setLabel(i18n.tr("Grace period (days)"));
      gracePeriod.setRange(1, 3650);
      gracePeriod.setSelection(observer.getGracePeriod());
      gracePeriod.setLayoutData(new GridData(SWT.LEFT, SWT.CENTER, false, false));

      return dialogArea;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(final boolean isApply)
   {
      final NXCObjectModificationData md = new NXCObjectModificationData(observer.getObjectId());

      String credentialsText = credentials.getText().trim();
      if (!credentialsText.isEmpty())
         md.setCredentials(credentialsText);

      md.setLinkedNodeId(linkedNode.getObjectId());
      md.setRemovalPolicy(removalPolicy.getSelectionIndex());
      md.setGracePeriod(gracePeriod.getSelection());

      if (isApply)
         setValid(false);

      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Updating traffic observer connection settings"), null, messageArea) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.modifyObject(md);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot update traffic observer connection settings");
         }

         @Override
         protected void jobFinalize()
         {
            if (isApply)
               runInUIThread(() -> TrafficObserverConnection.this.setValid(true));
         }
      }.start();
      return true;
   }
}
