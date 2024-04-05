/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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

import java.util.Collections;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.InputDialog;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowData;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Zone;
import org.netxms.client.snmp.SnmpUsmCredential;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.serverconfig.NetworkCredentials;
import org.netxms.nxmc.modules.serverconfig.dialogs.EditSnmpUsmCredentialsDialog;
import org.netxms.nxmc.modules.serverconfig.views.helpers.SnmpUsmCredentialsLabelProvider;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "SNMP Credentials" property page for zone objects
 */
public class ZoneSNMPCredentials extends ObjectPropertyPage
{
   private I18n i18n = LocalizationHelper.getI18n(ZoneSNMPCredentials.class);

   private Zone zone;
   private List<String> communities;
   private List<SnmpUsmCredential> usmCredentials;
   private List<Integer> ports;
   private TableViewer communityList;
   private Button commMoveUpButton;
   private Button commMoveDownButton;
   private Button commAddButton;
   private Button commDeleteButton;
   private SortableTableViewer usmCredentialList;
   private Button usmMoveUpButton;
   private Button usmMoveDownButton;
   private Button usmAddButton;
   private Button usmEditButton;
   private Button usmDeleteButton;
   private TableViewer portList;
   private Button portMoveUpButton;
   private Button portMoveDownButton;
   private Button portAddButton;
   private Button portDeleteButton;
   private int modified = 0;

   /**
    * Create new page.
    *
    * @param object object to edit
    */
   public ZoneSNMPCredentials(AbstractObject object)
   {
      super(LocalizationHelper.getI18n(ZoneSNMPCredentials.class).tr("SNMP Credentials"), object);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "zoneCommunications.snmpCredentials";
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getParentId()
    */
   @Override
   public String getParentId()
   {
      return "zoneCommunications";
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#isVisible()
    */
   @Override
   public boolean isVisible()
   {
      return (object instanceof Zone);
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      zone = (Zone)object;

      Composite dialogArea = new Composite(parent, SWT.NONE);
      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.numColumns = 2;
      layout.makeColumnsEqualWidth = true;
      dialogArea.setLayout(layout);

      createCommunitySection(dialogArea);
      createPortSection(dialogArea);
      createUSMCredentialsSection(dialogArea);

      loadConfig();
      return dialogArea;
   }

   /**
    * Create SNMP coumnity section
    * 
    * @param dialogArea
    */
   private void createCommunitySection(Composite dialogArea)
   {
      Group clientArea = new Group(dialogArea, SWT.NONE);
      clientArea.setText("Community strings");
      clientArea.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      layout.numColumns = 2;
      clientArea.setLayout(layout);

      communityList = new TableViewer(clientArea, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      GridData gd = new GridData(SWT.FILL, SWT.FILL, true, true);
      gd.horizontalSpan = 2;
      gd.heightHint = 150;
      communityList.getTable().setLayoutData(gd);
      communityList.setContentProvider(new ArrayContentProvider());

      Composite buttonsLeft = new Composite(clientArea, SWT.NONE);
      RowLayout buttonLayout = new RowLayout();
      buttonLayout.type = SWT.HORIZONTAL;
      buttonLayout.pack = false;
      buttonLayout.marginLeft = 0;
      buttonLayout.marginRight = 0;
      buttonsLeft.setLayout(buttonLayout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.LEFT;
      buttonsLeft.setLayoutData(gd);

      commMoveUpButton = new Button(buttonsLeft, SWT.PUSH);
      commMoveUpButton.setText("&Up");
      RowData rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      commMoveUpButton.setLayoutData(rd);
      commMoveUpButton.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }

         @Override
         public void widgetSelected(SelectionEvent e)
         {
            moveCommunity(true);
         }
      });

      commMoveDownButton = new Button(buttonsLeft, SWT.PUSH);
      commMoveDownButton.setText("&Down");
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      commMoveDownButton.setLayoutData(rd);
      commMoveDownButton.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }

         @Override
         public void widgetSelected(SelectionEvent e)
         {
            moveCommunity(false);
         }
      });

      Composite buttonsRight = new Composite(clientArea, SWT.NONE);
      buttonLayout = new RowLayout();
      buttonLayout.type = SWT.HORIZONTAL;
      buttonLayout.pack = false;
      buttonLayout.marginLeft = 0;
      buttonLayout.marginRight = 0;
      buttonsRight.setLayout(buttonLayout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.RIGHT;
      buttonsRight.setLayoutData(gd);

      commAddButton = new Button(buttonsRight, SWT.PUSH);
      commAddButton.setText("Add");
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      commAddButton.setLayoutData(rd);
      commAddButton.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }

         @Override
         public void widgetSelected(SelectionEvent e)
         {
            addCommunity();
         }
      });

      commDeleteButton = new Button(buttonsRight, SWT.PUSH);
      commDeleteButton.setText("Delete");
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      commDeleteButton.setLayoutData(rd);
      commDeleteButton.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }

         @Override
         public void widgetSelected(SelectionEvent e)
         {
            removeCommunity();
         }
      });

      communityList.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            IStructuredSelection selection = (IStructuredSelection)communityList.getSelection();
            commMoveUpButton.setEnabled(selection.size() == 1);
            commMoveDownButton.setEnabled(selection.size() == 1);
            commDeleteButton.setEnabled(selection.size() > 0);
         }
      });
   }

   /**
    * Load SNMP configuration
    */
   private void loadConfig()
   {
      final NXCSession session = Registry.getSession();
      Job job = new Job(i18n.tr("Loading SNMP credentials for zone {0}", zone.getObjectName()), null) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            communities = session.getSnmpCommunities(zone.getUIN());
            usmCredentials = session.getSnmpUsmCredentials(zone.getUIN());
            ports = session.getWellKnownPorts(zone.getUIN(), "snmp");
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  communityList.setInput(communities);
                  usmCredentialList.setInput(usmCredentials);
                  portList.setInput(ports);
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot load SNMP credentials for zone {0}", zone.getObjectName());
         }
      };
      job.start();
   }

   /**
    * Set modified
    * 
    * @param configId
    */
   private void setModified(int configId)
   {
      modified |= configId;
   }

   /**
    * Add SNMP community to the list
    */
   private void addCommunity()
   {
      InputDialog dlg = new InputDialog(getShell(), i18n.tr("Add SNMP Community"), i18n.tr("Enter SNMP community string"), "", null);
      if (dlg.open() == Window.OK)
      {
         String s = dlg.getValue();
         communities.add(s);
         communityList.setInput(communities);
         setModified(NetworkCredentials.SNMP_COMMUNITIES);
      }
   }

   /**
    * Remove selected SNMP communities
    */
   private void removeCommunity()
   {
      IStructuredSelection selection = communityList.getStructuredSelection();
      if (selection.size() > 0)
      {
         for(Object o : selection.toList())
         {
            communities.remove(o);
         }
         communityList.setInput(communities);
         setModified(NetworkCredentials.SNMP_COMMUNITIES);
      }
   }

   /**
    * Move community string in priority
    * 
    * @param up true to move up or false to move down
    */
   protected void moveCommunity(boolean up)
   {
      IStructuredSelection selection = communityList.getStructuredSelection();
      if (selection.size() > 0)
      {
         for(Object o : selection.toList())
         {
            int index = communities.indexOf(o);
            if (up)
            {
               if (index < 1)
                  return;

               Collections.swap(communities, index - 1, index);
            }
            else
            {
               if ((index + 1) == communities.size())
                  return;

               Collections.swap(communities, index + 1, index);
            }
         }
         communityList.setInput(communities);
         setModified(NetworkCredentials.SNMP_COMMUNITIES);
      }
   }

   /**
    * Create SNMP USM credential section
    * 
    * @param dialogArea
    */
   private void createUSMCredentialsSection(Composite dialogArea)
   {
      Group clientArea = new Group(dialogArea, SWT.NONE);
      clientArea.setText("USM credentials");
      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      clientArea.setLayout(layout);
      GridData gd = new GridData(SWT.FILL, SWT.FILL, true, true);
      gd.horizontalSpan = 2;
      clientArea.setLayoutData(gd);

      final String[] names = { "User name", "Auth type", "Priv type", "Auth secret", "Priv secret", "Comments" };
      final int[] widths = { 100, 100, 100, 100, 100, 100 };
      usmCredentialList = new SortableTableViewer(clientArea, names, widths, 0, SWT.DOWN, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      gd = new GridData(SWT.FILL, SWT.FILL, true, true);
      gd.horizontalSpan = 2;
      gd.heightHint = 150;
      usmCredentialList.getTable().setLayoutData(gd);
      usmCredentialList.setContentProvider(new ArrayContentProvider());
      usmCredentialList.setLabelProvider(new SnmpUsmCredentialsLabelProvider());
      usmCredentialList.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            editUsmCredentials();
         }
      });

      Composite buttonsLeft = new Composite(clientArea, SWT.NONE);
      RowLayout buttonLayout = new RowLayout();
      buttonLayout.type = SWT.HORIZONTAL;
      buttonLayout.pack = false;
      buttonLayout.marginLeft = 0;
      buttonLayout.marginRight = 0;
      buttonsLeft.setLayout(buttonLayout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.LEFT;
      buttonsLeft.setLayoutData(gd);

      usmMoveUpButton = new Button(buttonsLeft, SWT.PUSH);
      usmMoveUpButton.setText("&Up");
      RowData rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      usmMoveUpButton.setLayoutData(rd);
      usmMoveUpButton.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }

         @Override
         public void widgetSelected(SelectionEvent e)
         {
            moveUsmCredentials(true);
         }
      });

      usmMoveDownButton = new Button(buttonsLeft, SWT.PUSH);
      usmMoveDownButton.setText("&Down");
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      usmMoveDownButton.setLayoutData(rd);
      usmMoveDownButton.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }

         @Override
         public void widgetSelected(SelectionEvent e)
         {
            moveUsmCredentials(false);
         }
      });

      Composite buttonsRight = new Composite(clientArea, SWT.NONE);
      buttonLayout = new RowLayout();
      buttonLayout.type = SWT.HORIZONTAL;
      buttonLayout.pack = false;
      buttonLayout.marginLeft = 0;
      buttonLayout.marginRight = 0;
      buttonsRight.setLayout(buttonLayout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.RIGHT;
      buttonsRight.setLayoutData(gd);

      usmAddButton = new Button(buttonsRight, SWT.PUSH);
      usmAddButton.setText("Add");
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      usmAddButton.setLayoutData(rd);
      usmAddButton.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }

         @Override
         public void widgetSelected(SelectionEvent e)
         {
            addUsmCredentials();
         }
      });

      usmEditButton = new Button(buttonsRight, SWT.PUSH);
      usmEditButton.setText("Modify");
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      usmEditButton.setLayoutData(rd);
      usmEditButton.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }

         @Override
         public void widgetSelected(SelectionEvent e)
         {
            editUsmCredentials();
         }
      });

      usmDeleteButton = new Button(buttonsRight, SWT.PUSH);
      usmDeleteButton.setText("Delete");
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      usmDeleteButton.setLayoutData(rd);
      usmDeleteButton.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }

         @Override
         public void widgetSelected(SelectionEvent e)
         {
            removeUsmCredentials();
         }
      });

      usmCredentialList.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            IStructuredSelection selection = (IStructuredSelection)usmCredentialList.getSelection();
            usmMoveUpButton.setEnabled(selection.size() == 1);
            usmMoveDownButton.setEnabled(selection.size() == 1);
            usmEditButton.setEnabled(selection.size() == 1);
            usmDeleteButton.setEnabled(selection.size() > 0);
         }
      });
   }

   /**
    * Add SNMP USM credentials to the list
    */
   private void addUsmCredentials()
   {
      EditSnmpUsmCredentialsDialog dlg = new EditSnmpUsmCredentialsDialog(getShell(), null);
      if (dlg.open() == Window.OK)
      {
         SnmpUsmCredential cred = dlg.getCredentials();
         cred.setZoneId(zone.getUIN());
         usmCredentials.add(cred);
         usmCredentialList.setInput(usmCredentials.toArray());
         setModified(NetworkCredentials.SNMP_USM_CREDENTIALS);
      }
   }

   /**
    * Edit SNMP USM credential
    */
   private void editUsmCredentials()
   {
      IStructuredSelection selection = (IStructuredSelection)usmCredentialList.getSelection();
      if (selection.size() != 1)
         return;
      EditSnmpUsmCredentialsDialog dlg = new EditSnmpUsmCredentialsDialog(getShell(), (SnmpUsmCredential)selection.getFirstElement());
      if (dlg.open() == Window.OK)
      {
         usmCredentialList.setInput(usmCredentials.toArray());
         setModified(NetworkCredentials.SNMP_USM_CREDENTIALS);
      }
   }

   /**
    * Remove selected SNMP USM credentials
    */
   private void removeUsmCredentials()
   {
      IStructuredSelection selection = (IStructuredSelection)usmCredentialList.getSelection();
      if (selection.size() > 0)
      {
         for(Object o : selection.toList())
         {
            usmCredentials.remove(o);
         }
         usmCredentialList.setInput(usmCredentials.toArray());
         setModified(NetworkCredentials.SNMP_USM_CREDENTIALS);
      }
   }

   /**
    * Move SNMP USM credential
    * 
    * @param up true if up, false if down
    */
   private void moveUsmCredentials(boolean up)
   {
      IStructuredSelection selection = (IStructuredSelection)usmCredentialList.getSelection();
      if (selection.size() > 0)
      {
         for(Object o : selection.toList())
         {
            int index = usmCredentials.indexOf(o);
            if (up)
            {
               if (index < 1)
                  return;

               Collections.swap(usmCredentials, index - 1, index);
            }
            else
            {
               if ((index + 1) == usmCredentials.size())
                  return;

               Collections.swap(usmCredentials, index + 1, index);
            }
         }
         usmCredentialList.setInput(usmCredentials.toArray());
         setModified(NetworkCredentials.SNMP_USM_CREDENTIALS);
      }
   }

   /**
    * Create port section
    * 
    * @param dialogArea
    */
   private void createPortSection(Composite dialogArea)
   {
      Group clientArea = new Group(dialogArea, SWT.NONE);
      clientArea.setText("Ports");
      clientArea.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      clientArea.setLayout(layout);

      portList = new TableViewer(clientArea, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      GridData gd = new GridData(SWT.FILL, SWT.FILL, true, true);
      gd.horizontalSpan = 2;
      gd.heightHint = 150;
      portList.getTable().setLayoutData(gd);
      portList.setContentProvider(new ArrayContentProvider());

      Composite buttonsLeft = new Composite(clientArea, SWT.NONE);
      RowLayout buttonLayout = new RowLayout();
      buttonLayout.type = SWT.HORIZONTAL;
      buttonLayout.pack = false;
      buttonLayout.marginLeft = 0;
      buttonLayout.marginRight = 0;
      buttonsLeft.setLayout(buttonLayout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.LEFT;
      buttonsLeft.setLayoutData(gd);

      portMoveUpButton = new Button(buttonsLeft, SWT.PUSH);
      portMoveUpButton.setText("&Up");
      RowData rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      portMoveUpButton.setLayoutData(rd);
      portMoveUpButton.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }

         @Override
         public void widgetSelected(SelectionEvent e)
         {
            movePort(true);
         }
      });

      portMoveDownButton = new Button(buttonsLeft, SWT.PUSH);
      portMoveDownButton.setText("&Down");
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      portMoveDownButton.setLayoutData(rd);
      portMoveDownButton.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }

         @Override
         public void widgetSelected(SelectionEvent e)
         {
            movePort(false);
         }
      });

      Composite buttonsRight = new Composite(clientArea, SWT.NONE);
      buttonLayout = new RowLayout();
      buttonLayout.type = SWT.HORIZONTAL;
      buttonLayout.pack = false;
      buttonLayout.marginLeft = 0;
      buttonLayout.marginRight = 0;
      buttonsRight.setLayout(buttonLayout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.RIGHT;
      buttonsRight.setLayoutData(gd);

      portAddButton = new Button(buttonsRight, SWT.PUSH);
      portAddButton.setText("Add");
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      portAddButton.setLayoutData(rd);
      portAddButton.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }

         @Override
         public void widgetSelected(SelectionEvent e)
         {
            addPort();
         }
      });

      portDeleteButton = new Button(buttonsRight, SWT.PUSH);
      portDeleteButton.setText("Delete");
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      portDeleteButton.setLayoutData(rd);
      portDeleteButton.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }

         @Override
         public void widgetSelected(SelectionEvent e)
         {
            removePort();
         }
      });

      portList.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            IStructuredSelection selection = (IStructuredSelection)portList.getSelection();
            portMoveUpButton.setEnabled(selection.size() == 1);
            portMoveDownButton.setEnabled(selection.size() == 1);
            portDeleteButton.setEnabled(selection.size() > 0);
         }
      });
   }

   /**
    * Add port to the list
    */
   private void addPort()
   {
      InputDialog dlg = new InputDialog(getShell(), i18n.tr("Add SNMP Port"), i18n.tr("Please enter SNMP port"), "", null);
      if (dlg.open() == Window.OK)
      {
         String value = dlg.getValue();
         ports.add(Integer.parseInt(value));
         portList.setInput(ports.toArray());
         setModified(NetworkCredentials.SNMP_PORTS);
      }
   }

   /**
    * Remove selected port
    */
   private void removePort()
   {
      IStructuredSelection selection = (IStructuredSelection)portList.getSelection();
      if (selection.size() > 0)
      {
         for(Object o : selection.toList())
         {
            ports.remove((Integer)o);
         }
         portList.setInput(ports.toArray());
         setModified(NetworkCredentials.SNMP_PORTS);
      }
   }

   /**
    * Move port
    * 
    * @param up true if up, false if down
    */
   private void movePort(boolean up)
   {
      IStructuredSelection selection = (IStructuredSelection)portList.getSelection();
      if (selection.size() > 0)
      {
         for(Object o : selection.toList())
         {
            int index = ports.indexOf((Integer)o);
            if (up)
            {
               if (index >= 1)
                  Collections.swap(ports, index - 1, index);
            }
            else
            {
               if ((index + 1) != ports.size())
                  Collections.swap(ports, index + 1, index);
            }
         }
         portList.setInput(ports);
         setModified(NetworkCredentials.SNMP_PORTS);
      }
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(final boolean isApply)
   {
      if (modified == 0)
         return true; // Nothing to apply

      if (isApply)
         setValid(false);

      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Updating SNMP credentials for zone {0}", zone.getObjectName()), null, messageArea) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            if ((modified & NetworkCredentials.SNMP_COMMUNITIES) != 0)
            {
               session.updateSnmpCommunities(zone.getUIN(), communities);
            }

            if ((modified & NetworkCredentials.SNMP_USM_CREDENTIALS) != 0)
            {
               session.updateSnmpUsmCredentials(zone.getUIN(), usmCredentials);
            }

            if ((modified & NetworkCredentials.SNMP_PORTS) != 0)
            {
               session.updateWellKnownPorts(zone.getUIN(), "snmp", ports);
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot update SNMP credentials for zone {0}", zone.getObjectName());
         }

         @Override
         protected void jobFinalize()
         {
            if (isApply)
               runInUIThread(() -> ZoneSNMPCredentials.this.setValid(true));
         }
      }.start();
      return true;
   }
}
