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
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.TrafficConnector;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Node;
import org.netxms.client.objects.TrafficObserver;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.LabeledCombo;
import org.netxms.nxmc.base.widgets.LabeledSpinner;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.widgets.ObjectSelector;
import org.netxms.nxmc.modules.objects.widgets.ZoneSelector;
import org.netxms.nxmc.modules.traffic.widgets.TrafficCredentialsEditor;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Communication" property page for traffic observer objects: connector credentials, host
 * matching zone, analyzer node link, and observation point removal policy. Takes the
 * "communication" page slot so the web service proxy sub-page attaches to it.
 */
public class TrafficObserverCommunication extends ObjectPropertyPage
{
   private I18n i18n = LocalizationHelper.getI18n(TrafficObserverCommunication.class);

   private TrafficObserver observer;
   private LabeledCombo connectorName;
   private TrafficCredentialsEditor credentials;
   private Label connectorWarning;
   private boolean credentialsEditable = true;   // false while the selected connector is the observer's own and its module is not loaded on the server
   private List<TrafficConnector> connectors = new ArrayList<TrafficConnector>();
   private ZoneSelector zoneSelector;
   private ObjectSelector linkedNode;
   private LabeledCombo removalPolicy;
   private LabeledSpinner gracePeriod;

   /**
    * Create new page.
    *
    * @param object object to edit
    */
   public TrafficObserverCommunication(AbstractObject object)
   {
      super(LocalizationHelper.getI18n(TrafficObserverCommunication.class).tr("Communication"), object);
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
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#isVisible()
    */
   @Override
   public boolean isVisible()
   {
      return (object instanceof TrafficObserver);
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

      connectorName = new LabeledCombo(dialogArea, SWT.NONE);
      connectorName.setLabel(i18n.tr("Connector"));
      connectorName.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      connectorName.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            onConnectorSelected();
         }
      });

      credentials = new TrafficCredentialsEditor(dialogArea, SWT.NONE);
      credentials.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      if (!observer.isConnectorLoaded())
      {
         credentialsEditable = false;
         credentials.setVisible(false);
         ((GridData)credentials.getLayoutData()).exclude = true;
         connectorWarning = new Label(dialogArea, SWT.WRAP);
         connectorWarning.setText(i18n.tr("Warning: connector module \"{0}\" is not loaded on the server; connector configuration cannot be viewed or changed until the module is loaded.", observer.getConnectorName()));
         connectorWarning.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      }

      if (Registry.getSession().isZoningEnabled())
      {
         zoneSelector = new ZoneSelector(dialogArea, SWT.NONE, true);
         zoneSelector.setLabel(i18n.tr("Zone for host matching"));
         zoneSelector.setEmptySelectionText(i18n.tr("All zones"));
         zoneSelector.setZoneUIN(observer.getZoneId());
         zoneSelector.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      }

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

      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Reading traffic connector information"), null, messageArea) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final List<TrafficConnector> list = session.getTrafficConnectors();
            list.sort(Comparator.comparing(TrafficConnector::getName));
            runInUIThread(() -> {
               if (connectorName.isDisposed())
                  return;
               connectors = list;
               for(TrafficConnector c : connectors)
                  connectorName.add(c.getName());
               int index = connectorName.indexOf(observer.getConnectorName());
               if (index == -1)
               {
                  // Connector of this observer is not loaded on the server - keep its name selectable
                  connectorName.add(observer.getConnectorName());
                  index = connectors.size();
               }
               connectorName.select(index);
               onConnectorSelected();
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot read traffic connector information");
         }
      }.start();

      return dialogArea;
   }

   /**
    * Rebuild credential form for currently selected connector. Stored credentials only apply
    * to the observer's current connector; switching to another one starts from an empty form.
    * While the observer's connector module is not loaded on the server, its configuration
    * cannot be shown or changed, so a warning is displayed instead of the form.
    */
   private void onConnectorSelected()
   {
      int index = connectorName.getSelectionIndex();
      TrafficConnector connector = ((index >= 0) && (index < connectors.size())) ? connectors.get(index) : null;
      boolean sameConnector = connectorName.getText().equals(observer.getConnectorName());
      credentials.setConnector(connector, sameConnector ? observer.getCredentials() : null);
      credentialsEditable = !sameConnector || observer.isConnectorLoaded();
      if (connectorWarning != null)
      {
         connectorWarning.setVisible(!credentialsEditable);
         ((GridData)connectorWarning.getLayoutData()).exclude = credentialsEditable;
         credentials.setVisible(credentialsEditable);
         ((GridData)credentials.getLayoutData()).exclude = !credentialsEditable;
         credentials.getParent().layout(true, true);
      }
      WidgetHelper.adjustWindowSize(this);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(final boolean isApply)
   {
      final NXCObjectModificationData md = new NXCObjectModificationData(observer.getObjectId());

      if (credentialsEditable && !credentials.validate())
         return false;
      if (!connectorName.getText().equals(observer.getConnectorName()))
         md.setConnectorName(connectorName.getText());
      if (credentialsEditable)
         md.setCredentials(credentials.getCredentials());

      if ((zoneSelector != null) && (zoneSelector.getZoneUIN() != observer.getZoneId()))
         md.setZoneUIN(zoneSelector.getZoneUIN());

      md.setLinkedNodeId(linkedNode.getObjectId());
      md.setRemovalPolicy(removalPolicy.getSelectionIndex());
      md.setGracePeriod(gracePeriod.getSelection());

      if (isApply)
         setValid(false);

      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Updating traffic observer communication settings"), null, messageArea) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.modifyObject(md);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot update traffic observer communication settings");
         }

         @Override
         protected void jobFinalize()
         {
            if (isApply)
               runInUIThread(() -> TrafficObserverCommunication.this.setValid(true));
         }
      }.start();
      return true;
   }
}
