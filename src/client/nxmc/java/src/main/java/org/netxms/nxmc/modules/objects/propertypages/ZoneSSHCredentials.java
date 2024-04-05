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
import org.netxms.client.SSHCredentials;
import org.netxms.client.SshKeyPair;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Zone;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.serverconfig.NetworkCredentials;
import org.netxms.nxmc.modules.serverconfig.dialogs.EditSSHCredentialsDialog;
import org.netxms.nxmc.modules.serverconfig.views.helpers.SshCredentialsLabelProvider;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "SSH Credentials" property page for zone objects
 */
public class ZoneSSHCredentials extends ObjectPropertyPage
{
   private I18n i18n = LocalizationHelper.getI18n(ZoneSSHCredentials.class);

   private Zone zone;
   private List<SshKeyPair> sshKeys;
   private List<SSHCredentials> credentials;
   private List<Integer> ports;
   private TableViewer credentialsList;
   private SshCredentialsLabelProvider credentialsLabelProvider = new SshCredentialsLabelProvider();
   private Button secretMoveUpButton;
   private Button secretMoveDownButton;
   private Button secretAddButton;
   private Button secretDeleteButton;
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
   public ZoneSSHCredentials(AbstractObject object)
   {
      super(LocalizationHelper.getI18n(ZoneSSHCredentials.class).tr("SSH Credentials"), object);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "zoneCommunications.agentCredentials";
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

      createCredentialsSection(dialogArea);
      createPortSection(dialogArea);

      loadConfig();
		return dialogArea;
	}

	/**
    * Create credentials section
    * 
    * @param dialogArea
    */
   private void createCredentialsSection(Composite dialogArea)
	{      
      Group clientArea = new Group(dialogArea, SWT.NONE);
      clientArea.setText("Credentials");
      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      layout.numColumns = 2;
      clientArea.setLayout(layout);
      GridData gridData = new GridData();
      gridData.verticalAlignment = GridData.FILL;
      gridData.grabExcessVerticalSpace = true;
      gridData.horizontalAlignment = GridData.FILL;
      gridData.grabExcessHorizontalSpace = true;
      clientArea.setLayoutData(gridData);

      final String[] names = { "Login", "Password", "Key" };
      final int[] widths = { 150, 150, 150 };
      credentialsList = new SortableTableViewer(clientArea, names, widths, 0, SWT.DOWN, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      gridData = new GridData();
      gridData.verticalAlignment = GridData.FILL;
      gridData.grabExcessVerticalSpace = true;
      gridData.horizontalAlignment = GridData.FILL;
      gridData.grabExcessHorizontalSpace = true;
      gridData.horizontalSpan = 2;
      gridData.heightHint = 150;
      credentialsList.getTable().setLayoutData(gridData);
      credentialsList.setContentProvider(new ArrayContentProvider());
      credentialsList.setLabelProvider(credentialsLabelProvider);

      Composite buttonsLeft = new Composite(clientArea, SWT.NONE);
      RowLayout buttonLayout = new RowLayout();
      buttonLayout.type = SWT.HORIZONTAL;
      buttonLayout.pack = false;
      buttonLayout.marginLeft = 0;
      buttonLayout.marginRight = 0;
      buttonsLeft.setLayout(buttonLayout);
      gridData = new GridData();
      gridData.horizontalAlignment = SWT.LEFT;
      buttonsLeft.setLayoutData(gridData);

      secretMoveUpButton = new Button(buttonsLeft, SWT.PUSH);
      secretMoveUpButton.setText("&Up");
      RowData rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      secretMoveUpButton.setLayoutData(rd);
      secretMoveUpButton.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }

         @Override
         public void widgetSelected(SelectionEvent e)
         {
            moveCredentials(true);
         }
      });
      
      secretMoveDownButton = new Button(buttonsLeft, SWT.PUSH);
      secretMoveDownButton.setText("&Down");
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      secretMoveDownButton.setLayoutData(rd);
      secretMoveDownButton.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }

         @Override
         public void widgetSelected(SelectionEvent e)
         {
            moveCredentials(false);
         }
      });

      Composite buttonsRight = new Composite(clientArea, SWT.NONE);
      buttonLayout = new RowLayout();
      buttonLayout.type = SWT.HORIZONTAL;
      buttonLayout.pack = false;
      buttonLayout.marginLeft = 0;
      buttonLayout.marginRight = 0;
      buttonsRight.setLayout(buttonLayout);
      gridData = new GridData();
      gridData.horizontalAlignment = SWT.RIGHT;
      buttonsRight.setLayoutData(gridData);

      secretAddButton = new Button(buttonsRight, SWT.PUSH);
      secretAddButton.setText("Add");
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      secretAddButton.setLayoutData(rd);
      secretAddButton.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }

         @Override
         public void widgetSelected(SelectionEvent e)
         {
            addCredentials();
         }
      });
      
      secretDeleteButton = new Button(buttonsRight, SWT.PUSH);
      secretDeleteButton.setText("Delete");
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      secretDeleteButton.setLayoutData(rd);
      secretDeleteButton.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }

         @Override
         public void widgetSelected(SelectionEvent e)
         {
            removeCredentials();
         }
      });
      
      credentialsList.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            IStructuredSelection selection = (IStructuredSelection)credentialsList.getSelection();
            secretMoveUpButton.setEnabled(selection.size() == 1);
            secretMoveDownButton.setEnabled(selection.size() == 1);
            secretDeleteButton.setEnabled(selection.size() > 0);
         }
      });
	}
	
   /**
    * Load configured secrets from server
    */
	private void loadConfig()
	{
      final NXCSession session = Registry.getSession();
      Job job = new Job(i18n.tr("Loading SSH credentials for zone {0}", zone.getObjectName()), null) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            sshKeys = session.getSshKeys(false);
            credentials = session.getSshCredentials(zone.getUIN());
            ports = session.getWellKnownPorts(zone.getUIN(), "ssh");
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  credentialsLabelProvider.setKeyList(sshKeys);
                  credentialsList.setInput(credentials);
                  portList.setInput(ports);
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot load SSH credentials for zone {0}", zone.getObjectName());
         }
      };
      job.start();
	}

   /**
    * Add credentials
    */
   private void addCredentials()
   {
      EditSSHCredentialsDialog dlg = new EditSSHCredentialsDialog(getShell(), null, sshKeys);
      if (dlg.open() == Window.OK)
      {
         SSHCredentials cred = dlg.getCredentials();
         credentials.add(cred);
         credentialsList.refresh();
         setModified(NetworkCredentials.SSH_CREDENTIALS);
      }
   }

   /**
    * Remove selected credentials
    */
   protected void removeCredentials()
   {
      IStructuredSelection selection = credentialsList.getStructuredSelection();
      if (!selection.isEmpty())
      {
         for(Object o : selection.toList())
         {
            credentials.remove(o);
         }
         credentialsList.refresh();
         setModified(NetworkCredentials.SSH_CREDENTIALS);
      }
   }

   /**
    * Move credentials up or down
    * 
    * @param up true to move up, false to move down
    */
   private void moveCredentials(boolean up)
   {
      IStructuredSelection selection = credentialsList.getStructuredSelection();
      if (selection.size() > 0)
      {
         for(Object o : selection.toList())
         {
            int index = credentials.indexOf(o);
            if (up)
            {
               if (index >= 1)
                  Collections.swap(credentials, index - 1, index);
            }
            else
            {
               if ((index + 1) != credentials.size())
                  Collections.swap(credentials, index + 1, index);
            }
         }
         credentialsList.refresh();
         setModified(NetworkCredentials.SSH_CREDENTIALS);
      }
   }

   /**
    * Create SNMP port section
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
      InputDialog dlg = new InputDialog(getShell(), i18n.tr("Add SSH Port"), i18n.tr("Please enter SSH port"), "", null);
      if (dlg.open() == Window.OK)
      {
         String value = dlg.getValue();
         ports.add(Integer.parseInt(value));
         portList.refresh();
         setModified(NetworkCredentials.SSH_PORTS);
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
         portList.refresh();
         setModified(NetworkCredentials.SSH_PORTS);
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
         portList.refresh();
         setModified(NetworkCredentials.SSH_PORTS);
      }
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
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(final boolean isApply)
   {
      if (modified == 0)
         return true;     // Nothing to apply

		if (isApply)
			setValid(false);

      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Updating SSH credentials for zone {0}", zone.getObjectName()), null, messageArea) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
			{
            if ((modified & NetworkCredentials.SSH_CREDENTIALS) != 0)
            {
               session.updateSshCredentials(zone.getUIN(), credentials);
            }

            if ((modified & NetworkCredentials.SSH_PORTS) != 0)
            {
               session.updateWellKnownPorts(zone.getUIN(), "ssh", ports);
            }
			}

			@Override
			protected String getErrorMessage()
			{
            return i18n.tr("Cannot update SSH credentials for zone {0}", zone.getObjectName());
			}

         @Override
         protected void jobFinalize()
         {
            if (isApply)
               runInUIThread(() -> ZoneSSHCredentials.this.setValid(true));
         }
		}.start();
		return true;
	}
}
