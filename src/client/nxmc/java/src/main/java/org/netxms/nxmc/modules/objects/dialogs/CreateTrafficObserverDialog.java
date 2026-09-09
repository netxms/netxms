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
package org.netxms.nxmc.modules.objects.dialogs;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.NXCSession;
import org.netxms.client.TrafficConnector;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.LabeledCombo;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.widgets.ZoneSelector;
import org.netxms.nxmc.modules.traffic.widgets.TrafficCredentialsEditor;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.ObjectNameValidator;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Traffic observer object creation dialog
 */
public class CreateTrafficObserverDialog extends Dialog
{
   private I18n i18n = LocalizationHelper.getI18n(CreateTrafficObserverDialog.class);

   private LabeledText nameField;
   private LabeledText aliasField;
   private LabeledCombo connectorNameField;
   private TrafficCredentialsEditor credentialsEditor;
   private ZoneSelector zoneSelector;
   private List<TrafficConnector> connectors = new ArrayList<TrafficConnector>();

   private String name;
   private String alias;
   private String connectorName;
   private String credentials;
   private int zoneUIN = 0;

   /**
    * @param parentShell
    */
   public CreateTrafficObserverDialog(Shell parentShell)
   {
      super(parentShell);
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(i18n.tr("Create Traffic Observer"));
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      dialogArea.setLayout(layout);

      nameField = new LabeledText(dialogArea, SWT.NONE);
      nameField.setLabel(i18n.tr("Name"));
      nameField.getTextControl().setTextLimit(255);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.widthHint = 400;
      nameField.setLayoutData(gd);

      aliasField = new LabeledText(dialogArea, SWT.NONE);
      aliasField.setLabel(i18n.tr("Alias"));
      aliasField.getTextControl().setTextLimit(255);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      aliasField.setLayoutData(gd);

      connectorNameField = new LabeledCombo(dialogArea, SWT.NONE);
      connectorNameField.setLabel(i18n.tr("Connector"));
      connectorNameField.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      connectorNameField.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            onConnectorSelected();
         }
      });

      credentialsEditor = new TrafficCredentialsEditor(dialogArea, SWT.NONE);
      credentialsEditor.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      final NXCSession session = Registry.getSession();
      if (session.isZoningEnabled())
      {
         zoneSelector = new ZoneSelector(dialogArea, SWT.NONE, true);
         zoneSelector.setLabel(i18n.tr("Zone for host matching"));
         zoneSelector.setEmptySelectionText(i18n.tr("All zones"));
         zoneSelector.setZoneUIN(-1);
         zoneSelector.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      }

      new Job(i18n.tr("Reading list of available traffic connectors"), null) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final List<TrafficConnector> list = session.getTrafficConnectors();
            list.sort(Comparator.comparing(TrafficConnector::getName));
            runInUIThread(() -> {
               if (connectorNameField.isDisposed())
                  return;
               connectors = list;
               for(TrafficConnector c : connectors)
                  connectorNameField.add(c.getName());
               connectorNameField.select(0);
               onConnectorSelected();
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot read list of available traffic connectors");
         }
      }.start();

      return dialogArea;
   }

   /**
    * Rebuild credential form for currently selected connector
    */
   private void onConnectorSelected()
   {
      int index = connectorNameField.getSelectionIndex();
      credentialsEditor.setConnector(((index >= 0) && (index < connectors.size())) ? connectors.get(index) : null, null);
      WidgetHelper.adjustWindowSize(this);
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      if (!WidgetHelper.validateTextInput(nameField, new ObjectNameValidator()))
      {
         WidgetHelper.adjustWindowSize(this);
         return;
      }

      name = nameField.getText().trim();
      alias = aliasField.getText().trim();
      connectorName = connectorNameField.getText().trim();

      if (connectorName.isEmpty())
      {
         MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Please select traffic connector"));
         return;
      }

      if (!credentialsEditor.validate())
      {
         WidgetHelper.adjustWindowSize(this);
         return;
      }
      credentials = credentialsEditor.getCredentials();

      if (zoneSelector != null)
         zoneUIN = zoneSelector.getZoneUIN();

      super.okPressed();
   }

   /**
    * @return the name
    */
   public String getName()
   {
      return name;
   }

   /**
    * @return the alias
    */
   public String getAlias()
   {
      return alias;
   }

   /**
    * @return the connectorName
    */
   public String getConnectorName()
   {
      return connectorName;
   }

   /**
    * @return the credentials
    */
   public String getCredentials()
   {
      return credentials;
   }

   /**
    * @return zone UIN for host matching (-1 = all zones)
    */
   public int getZoneUIN()
   {
      return zoneUIN;
   }
}
