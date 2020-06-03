/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Raden Solutions
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
package org.netxms.ui.eclipse.snmp.propertypages;

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
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.Zone;
import org.netxms.client.snmp.SnmpUsmCredential;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.snmp.Activator;
import org.netxms.ui.eclipse.snmp.Messages;
import org.netxms.ui.eclipse.snmp.dialogs.AddUsmCredDialog;
import org.netxms.ui.eclipse.snmp.views.helpers.NetworkConfig;
import org.netxms.ui.eclipse.snmp.views.helpers.SnmpUsmLabelProvider;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * "SNMP Configuration" property page for zone objects
 */
public class ZoneSNMPConfiguration extends PropertyPage
{
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
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
		zone = (Zone)getElement().getAdapter(Zone.class);
		noDefaultAndApplyButton();

		Composite dialogArea = new Composite(parent, SWT.NONE);	
      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.numColumns = 2;	
      layout.makeColumnsEqualWidth = true;
		dialogArea.setLayout(layout);
      
      createSnmpCommunitySection(dialogArea);
      createSnmpPortList(dialogArea);
      createSnmpUsmCredSection(dialogArea);
      
      loadConfig();
		return dialogArea;
	}

	/**
	 * Create SNMP coumnity section
	 * 
	 * @param dialogArea
	 */
   private void createSnmpCommunitySection(Composite dialogArea)
	{      
      Composite clientArea = new Composite(dialogArea, SWT.NONE);
      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.numColumns = 2;
      clientArea.setLayout(layout);
      
      communityList = new TableViewer(clientArea, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      GridData gridData = new GridData();
      gridData.verticalAlignment = GridData.FILL;
      gridData.grabExcessVerticalSpace = true;
      gridData.horizontalAlignment = GridData.FILL;
      gridData.grabExcessHorizontalSpace = true;
      gridData.horizontalSpan = 2;
      gridData.heightHint = 150;
      communityList.getTable().setLayoutData(gridData);
      communityList.setContentProvider(new ArrayContentProvider());
      
      Composite buttonsLeft = new Composite(clientArea, SWT.NONE);
      RowLayout buttonLayout = new RowLayout();
      buttonLayout.type = SWT.HORIZONTAL;
      buttonLayout.pack = false;
      buttonLayout.marginWidth = 0;
      buttonLayout.marginRight = 0;
      buttonsLeft.setLayout(buttonLayout);
      gridData = new GridData();
      gridData.horizontalAlignment = SWT.LEFT;
      buttonsLeft.setLayoutData(gridData);
      
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
      buttonLayout.marginWidth = 0;
      buttonLayout.marginRight = 0;
      buttonsRight.setLayout(buttonLayout);
      gridData = new GridData();
      gridData.horizontalAlignment = SWT.RIGHT;
      buttonsRight.setLayoutData(gridData);

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
      final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
      
	   ConsoleJob job = new ConsoleJob(Messages.get().NetworkCredentials_LoadingConfig, null, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            communities = session.getSnmpCommunities(zone.getUIN());
            usmCredentials = session.getSnmpUsmCredentials(zone.getUIN());
            ports = session.getSNMPPors(zone.getUIN());
            
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  updateFields();
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return Messages.get().NetworkCredentials_ErrorLoadingConfig;
         }
      };
      job.setUser(false);
      job.start();
	}
	
	/**
	 * Update fileds
	 */
	private void updateFields()
	{
      communityList.setInput(communities);
      usmCredentialList.setInput(usmCredentials);
      portList.setInput(ports);
      //sharedSecretList.setInput(sharedSecrets);
	   
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
      InputDialog dlg = new InputDialog(getShell(), Messages.get().SnmpConfigurator_AddCommunity, 
            Messages.get().SnmpConfigurator_AddCommunityDescr, "", null); //$NON-NLS-1$
      if (dlg.open() == Window.OK)
      {
         String s = dlg.getValue();
         communities.add(s);
         communityList.setInput(communities);
         setModified(NetworkConfig.COMMUNITIES);
      }
   }

   /**
    * Remove selected SNMP communities
    */
   private void removeCommunity()
   {
      IStructuredSelection selection = (IStructuredSelection)communityList.getSelection();
      if (selection.size() > 0)
      {
         for(Object o : selection.toList())
         {
            communities.remove(o);
         }
         communityList.setInput(communities);
         setModified(NetworkConfig.COMMUNITIES);
      }
   }
   
   /**
    * Move community string in priority
    * 
    * @param up true to move up or false to move down
    */
   protected void moveCommunity(boolean up)
   {
      IStructuredSelection selection = (IStructuredSelection)communityList.getSelection();
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
         setModified(NetworkConfig.COMMUNITIES);
      }
   }

   /**
    * Create SNMP USM credential section 
    * 
    * @param dialogArea
    */
   private void createSnmpUsmCredSection(Composite dialogArea)
   {      
      Composite clientArea = new Composite(dialogArea, SWT.NONE);
      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      clientArea.setLayout(layout);
      GridData gridData = new GridData();
      gridData.horizontalSpan = 2;
      gridData.verticalAlignment = GridData.FILL;
      gridData.grabExcessHorizontalSpace = true;
      clientArea.setLayoutData(gridData);
      
      
      final String[] names = { "User name", "Auth type", "Priv type", "Auth secret", "Priv secret", "Comments" };
      final int[] widths = { 100, 100, 100, 100, 100, 100 };
      usmCredentialList = new SortableTableViewer(clientArea, names, widths, 0, SWT.DOWN, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      gridData = new GridData();
      gridData.verticalAlignment = GridData.FILL;
      gridData.grabExcessVerticalSpace = true;
      gridData.horizontalAlignment = GridData.FILL;
      gridData.grabExcessHorizontalSpace = true;
      gridData.horizontalSpan = 2;
      gridData.heightHint = 150;
      usmCredentialList.getTable().setLayoutData(gridData);
      usmCredentialList.setContentProvider(new ArrayContentProvider());
      usmCredentialList.setLabelProvider(new SnmpUsmLabelProvider());
      usmCredentialList.addDoubleClickListener(new IDoubleClickListener() {
         
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            editUsmCredzantial();
         }
      }); 
      
      Composite buttonsLeft = new Composite(clientArea, SWT.NONE);
      RowLayout buttonLayout = new RowLayout();
      buttonLayout.type = SWT.HORIZONTAL;
      buttonLayout.pack = false;
      buttonLayout.marginWidth = 0;
      buttonLayout.marginRight = 0;
      buttonsLeft.setLayout(buttonLayout);
      gridData = new GridData();
      gridData.horizontalAlignment = SWT.LEFT;
      buttonsLeft.setLayoutData(gridData);
      
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
      buttonLayout.marginWidth = 0;
      buttonLayout.marginRight = 0;
      buttonsRight.setLayout(buttonLayout);
      gridData = new GridData();
      gridData.horizontalAlignment = SWT.RIGHT;
      buttonsRight.setLayoutData(gridData);

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
            editUsmCredzantial();
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
      AddUsmCredDialog dlg = new AddUsmCredDialog(getShell(), null);
      if (dlg.open() == Window.OK)
      {
         SnmpUsmCredential cred = dlg.getValue();
         cred.setZoneId(zone.getUIN());
         usmCredentials.add(cred);
         usmCredentialList.setInput(usmCredentials.toArray());
         setModified(NetworkConfig.USM);
      }
   }
   
   /**
    * Edit SNMP USM credential
    */
   private void editUsmCredzantial()
   {
      IStructuredSelection selection = (IStructuredSelection)usmCredentialList.getSelection();
      if (selection.size() != 1)
         return;
      SnmpUsmCredential cred = (SnmpUsmCredential)selection.getFirstElement();
      AddUsmCredDialog dlg = new AddUsmCredDialog(getShell(), cred);
      if (dlg.open() == Window.OK)
      {
         usmCredentialList.setInput(usmCredentials.toArray());
         setModified(NetworkConfig.USM);
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
         setModified(NetworkConfig.USM);
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
         setModified(NetworkConfig.USM);
      }
   }

   /**
    * Create SNMP port section 
    * 
    * @param dialogArea
    */
   private void createSnmpPortList(Composite dialogArea)
   {     
      Composite clientArea = new Composite(dialogArea, SWT.NONE);
      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      clientArea.setLayout(layout);
      
      portList = new TableViewer(clientArea, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      GridData gridData = new GridData();
      gridData.horizontalAlignment = SWT.FILL;
      gridData.grabExcessHorizontalSpace = true;
      gridData.verticalAlignment = SWT.FILL;
      gridData.grabExcessVerticalSpace = true;
      gridData.horizontalSpan = 2;
      gridData.heightHint = 150;
      portList.getTable().setLayoutData(gridData);
      portList.setContentProvider(new ArrayContentProvider());      
      
      Composite buttonsLeft = new Composite(clientArea, SWT.NONE);
      RowLayout buttonLayout = new RowLayout();
      buttonLayout.type = SWT.HORIZONTAL;
      buttonLayout.pack = false;
      buttonLayout.marginWidth = 0;
      buttonLayout.marginRight = 0;
      buttonsLeft.setLayout(buttonLayout);
      gridData = new GridData();
      gridData.horizontalAlignment = SWT.LEFT;
      buttonsLeft.setLayoutData(gridData);
      
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
            moveSnmpPort(true);
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
            moveSnmpPort(false);
         }
      });
      
      Composite buttonsRight = new Composite(clientArea, SWT.NONE);
      buttonLayout = new RowLayout();
      buttonLayout.type = SWT.HORIZONTAL;
      buttonLayout.pack = false;
      buttonLayout.marginWidth = 0;
      buttonLayout.marginRight = 0;
      buttonsRight.setLayout(buttonLayout);
      gridData = new GridData();
      gridData.horizontalAlignment = SWT.RIGHT;
      buttonsRight.setLayoutData(gridData);

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
            addSnmpPort();
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
            removeSnmpPort();
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
   * Add SNMP port to the list
   */
  private void addSnmpPort()
  {
     InputDialog dlg = new InputDialog(getShell(), Messages.get().NetworkCredentials_AddPort, 
           Messages.get().NetworkCredentials_PleaseEnterPort, "", null); //$NON-NLS-1$
     if (dlg.open() == Window.OK)
     {
        String value = dlg.getValue();
        ports.add(Integer.parseInt(value));
        portList.setInput(ports.toArray());
        setModified(NetworkConfig.PORTS);
     }
  }
  
  /**
   * Remove selected SNMP port
   */
  private void removeSnmpPort()
  {
     IStructuredSelection selection = (IStructuredSelection)portList.getSelection();
     if (selection.size() > 0)
     {
        for(Object o : selection.toList())
        {
           ports.remove((Integer)o);
        }
        portList.setInput(ports.toArray());
        setModified(NetworkConfig.PORTS);
     }
  }

  /**
   * Move SNMP port
   * 
   * @param up true if up, false if down
   */
  private void moveSnmpPort(boolean up)
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
        setModified(NetworkConfig.PORTS);
     }
  }  
	
	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
   private boolean applyChanges(final boolean isApply)
	{
      if (modified == 0)
         return true;     // Nothing to apply

		if (isApply)
			setValid(false);

		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		new ConsoleJob(String.format(Messages.get().NetworkCredentials_SaveConfig, zone.getObjectName()), null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
            if ((modified & NetworkConfig.COMMUNITIES) != 0)
	         {
	            session.updateSnmpCommunities(zone.getUIN(), communities);   
	         }

            if ((modified & NetworkConfig.USM) != 0)
	         {
	            session.updateSnmpUsmCredentials(zone.getUIN(), usmCredentials);    
	         }

            if ((modified & NetworkConfig.PORTS) != 0)
	         {
	            session.updateSNMPPorts(zone.getUIN(), ports);
	         }
			}

			@Override
			protected String getErrorMessage()
			{
				return Messages.get().NetworkCredentials_ErrorSavingConfig;
			}
		}.start();
		return true;
	}
	
   /**
    * @see org.eclipse.jface.preference.PreferencePage#performOk()
    */
	@Override
	public boolean performOk()
	{
		return applyChanges(false);
	}
}
