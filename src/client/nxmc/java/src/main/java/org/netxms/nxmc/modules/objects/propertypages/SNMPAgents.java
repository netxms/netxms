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

import java.util.ArrayList;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowData;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.snmp.SnmpAgentConfiguration;
import org.netxms.client.snmp.SnmpVersion;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.dialogs.SnmpAgentEditDialog;
import org.xnap.commons.i18n.I18n;

/**
 * "SNMP Agents" property page for node - manages list of additional SNMP agents
 */
public class SNMPAgents extends ObjectPropertyPage
{
   private I18n i18n = LocalizationHelper.getI18n(SNMPAgents.class);

   public static final int COLUMN_NAME = 0;
   public static final int COLUMN_ADDRESS = 1;
   public static final int COLUMN_PORT = 2;
   public static final int COLUMN_VERSION = 3;
   public static final int COLUMN_AUTH_NAME = 4;
   public static final int COLUMN_CONTEXT = 5;

   private AbstractNode node;
   private List<SnmpAgentConfiguration> agents;
   private SortableTableViewer viewer;
   private Button addButton;
   private Button editButton;
   private Button deleteButton;

   /**
    * Create new page.
    *
    * @param object object to edit
    */
   public SNMPAgents(AbstractObject object)
   {
      super(LocalizationHelper.getI18n(SNMPAgents.class).tr("SNMP Agents"), object);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "communication.snmpAgents";
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

      agents = new ArrayList<SnmpAgentConfiguration>();
      if (node.getSnmpAgents() != null)
      {
         for(SnmpAgentConfiguration a : node.getSnmpAgents())
            agents.add(new SnmpAgentConfiguration(a));
      }

      Composite dialogArea = new Composite(parent, SWT.NONE);
      GridLayout layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      dialogArea.setLayout(layout);

      final String[] names = { i18n.tr("Name"), i18n.tr("Address"), i18n.tr("Port"), i18n.tr("Version"), i18n.tr("User/community"), i18n.tr("Context") };
      final int[] widths = { 150, 120, 60, 60, 120, 120 };
      viewer = new SortableTableViewer(dialogArea, names, widths, 0, SWT.UP, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      viewer.setContentProvider(ArrayContentProvider.getInstance());
      viewer.setLabelProvider(new SnmpAgentLabelProvider());
      viewer.setInput(agents);
      GridData gd = new GridData(SWT.FILL, SWT.FILL, true, true);
      gd.heightHint = 200;
      viewer.getControl().setLayoutData(gd);
      viewer.addDoubleClickListener((e) -> editAgent());
      viewer.addSelectionChangedListener((e) -> {
         IStructuredSelection selection = viewer.getStructuredSelection();
         editButton.setEnabled(selection.size() == 1);
         deleteButton.setEnabled(!selection.isEmpty());
      });

      Composite buttons = new Composite(dialogArea, SWT.NONE);
      RowLayout buttonLayout = new RowLayout();
      buttonLayout.type = SWT.HORIZONTAL;
      buttonLayout.pack = false;
      buttonLayout.marginWidth = 0;
      buttons.setLayout(buttonLayout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.RIGHT;
      buttons.setLayoutData(gd);

      addButton = new Button(buttons, SWT.PUSH);
      addButton.setText(i18n.tr("&Add..."));
      addButton.setLayoutData(new RowData(90, SWT.DEFAULT));
      addButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            addAgent();
         }
      });

      editButton = new Button(buttons, SWT.PUSH);
      editButton.setText(i18n.tr("&Edit..."));
      editButton.setLayoutData(new RowData(90, SWT.DEFAULT));
      editButton.setEnabled(false);
      editButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            editAgent();
         }
      });

      deleteButton = new Button(buttons, SWT.PUSH);
      deleteButton.setText(i18n.tr("&Delete"));
      deleteButton.setLayoutData(new RowData(90, SWT.DEFAULT));
      deleteButton.setEnabled(false);
      deleteButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            deleteAgents();
         }
      });

      return dialogArea;
   }

   /**
    * Add new agent
    */
   private void addAgent()
   {
      SnmpAgentConfiguration configuration = new SnmpAgentConfiguration();
      SnmpAgentEditDialog dlg = new SnmpAgentEditDialog(getShell(), configuration, true);
      while(dlg.open() == Window.OK)
      {
         if (findAgentByName(configuration.getName(), null) != null)
         {
            MessageDialog.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Agent with name \"{0}\" already exists", configuration.getName()));
            continue;
         }
         agents.add(configuration);
         viewer.refresh();
         break;
      }
   }

   /**
    * Edit selected agent
    */
   private void editAgent()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      SnmpAgentConfiguration configuration = (SnmpAgentConfiguration)selection.getFirstElement();
      SnmpAgentEditDialog dlg = new SnmpAgentEditDialog(getShell(), configuration, false);
      while(dlg.open() == Window.OK)
      {
         if (findAgentByName(configuration.getName(), configuration) != null)
         {
            MessageDialog.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Agent with name \"{0}\" already exists", configuration.getName()));
            continue;
         }
         viewer.refresh();
         break;
      }
   }

   /**
    * Delete selected agents
    */
   private void deleteAgents()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      for(Object o : selection.toList())
         agents.remove(o);
      viewer.refresh();
   }

   /**
    * Find agent by name.
    *
    * @param name agent name
    * @param skip agent object to skip (can be null)
    * @return agent with given name or null
    */
   private SnmpAgentConfiguration findAgentByName(String name, SnmpAgentConfiguration skip)
   {
      for(SnmpAgentConfiguration a : agents)
         if ((a != skip) && a.getName().equalsIgnoreCase(name))
            return a;
      return null;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(final boolean isApply)
   {
      final NXCObjectModificationData md = new NXCObjectModificationData(node.getObjectId());
      md.setSnmpAgents(new ArrayList<SnmpAgentConfiguration>(agents));

      if (isApply)
         setValid(false);

      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Updating SNMP agent list for node {0}", node.getObjectName()), null, messageArea) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.modifyObject(md);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot update SNMP agent list for node {0}", node.getObjectName());
         }

         @Override
         protected void jobFinalize()
         {
            if (isApply)
               runInUIThread(() -> SNMPAgents.this.setValid(true));
         }
      }.start();
      return true;
   }

   /**
    * Label provider for list of additional SNMP agents
    */
   private static class SnmpAgentLabelProvider extends LabelProvider implements ITableLabelProvider
   {
      /**
       * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
       */
      @Override
      public Image getColumnImage(Object element, int columnIndex)
      {
         return null;
      }

      /**
       * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
       */
      @Override
      public String getColumnText(Object element, int columnIndex)
      {
         SnmpAgentConfiguration a = (SnmpAgentConfiguration)element;
         switch(columnIndex)
         {
            case COLUMN_NAME:
               return a.getName();
            case COLUMN_ADDRESS:
               return (a.getAddress() != null) ? a.getAddress().getHostAddress() : "";
            case COLUMN_PORT:
               return Integer.toString(a.getPort());
            case COLUMN_VERSION:
               return (a.getVersion() == SnmpVersion.V1) ? "1" : (a.getVersion() == SnmpVersion.V3) ? "3" : "2c";
            case COLUMN_AUTH_NAME:
               return a.getAuthName();
            case COLUMN_CONTEXT:
               return a.getContextName();
         }
         return null;
      }
   }
}
