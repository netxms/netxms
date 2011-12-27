/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.datacollection.propertypages;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Listener;
import org.eclipse.swt.widgets.Text;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.client.objects.Node;
import org.netxms.client.snmp.SnmpObjectId;
import org.netxms.client.snmp.SnmpObjectIdFormatException;
import org.netxms.ui.eclipse.datacollection.Activator;
import org.netxms.ui.eclipse.datacollection.dialogs.IParameterSelectionDialog;
import org.netxms.ui.eclipse.datacollection.dialogs.SelectAgentParamDlg;
import org.netxms.ui.eclipse.datacollection.dialogs.SelectInternalParamDlg;
import org.netxms.ui.eclipse.datacollection.dialogs.SelectSnmpParamDlg;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectbrowser.widgets.ObjectSelector;
import org.netxms.ui.eclipse.shared.SharedColors;
import org.netxms.ui.eclipse.tools.NumericTextFieldValidator;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * "General" property page for DCI
 *
 */
public class General extends PropertyPage
{
	private static final String[] snmpRawTypes = 
	{ 
		"None", 
		"32-bit signed integer", 
		"32-bit unsigned integer",
		"64-bit signed integer", 
		"64-bit unsigned integer",
		"Floating point number", 
		"IP address",
		"MAC address"
	};
	
	private DataCollectionItem dci;
	private Text description;
	private LabeledText parameter;
	private Button selectButton;
	private Combo origin;
	private Combo dataType;
	private Button checkInterpretRawSnmpValue;
	private Combo snmpRawType;
	private Button checkUseCustomSnmpPort;
	private Text customSnmpPort;
	private ObjectSelector proxyNode;
	private Combo schedulingMode;
	private LabeledText pollingInterval;
	private LabeledText retentionTime;
	private Combo clusterResource;
	private Button statusActive;
	private Button statusDisabled;
	private Button statusUnsupported;
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{		
		dci = (DataCollectionItem)getElement().getAdapter(DataCollectionItem.class);
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		layout.numColumns = 2;
      dialogArea.setLayout(layout);
      
      /** description area **/
      Group groupDescription = new Group(dialogArea, SWT.NONE);
      groupDescription.setText("Description");
      FillLayout descriptionLayout = new FillLayout();
      descriptionLayout.marginWidth = WidgetHelper.OUTER_SPACING;
      descriptionLayout.marginHeight = WidgetHelper.OUTER_SPACING;
      groupDescription.setLayout(descriptionLayout);
      description = new Text(groupDescription, SWT.BORDER);
      description.setTextLimit(255);
      description.setText(dci.getDescription());
      GridData gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.horizontalSpan = 2;
      groupDescription.setLayoutData(gd);
      
      /** data area **/
      Group groupData = new Group(dialogArea, SWT.NONE);
      groupData.setText("Data");
      FormLayout dataLayout = new FormLayout();
      dataLayout.marginHeight = WidgetHelper.OUTER_SPACING;
      dataLayout.marginWidth = WidgetHelper.OUTER_SPACING;
      groupData.setLayout(dataLayout);
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.horizontalSpan = 2;
      groupData.setLayoutData(gd);
      
      parameter = new LabeledText(groupData, SWT.NONE);
      parameter.setLabel("Parameter");
      parameter.getTextControl().setTextLimit(255);
      parameter.setText(dci.getName());
      
      selectButton = new Button(groupData, SWT.PUSH);
      selectButton.setText("&Select...");
      selectButton.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@Override
			public void widgetSelected(SelectionEvent e)
			{
				selectParameter();
			}
      });

      FormData fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(0, 0);
      fd.right = new FormAttachment(selectButton, -WidgetHelper.INNER_SPACING, SWT.LEFT);
      parameter.setLayoutData(fd);

      fd = new FormData();
      fd.right = new FormAttachment(100, 0);
      fd.bottom = new FormAttachment(parameter, 0, SWT.BOTTOM);
      fd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      selectButton.setLayoutData(fd);
      
      fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(parameter, WidgetHelper.OUTER_SPACING, SWT.BOTTOM);
      fd.right = new FormAttachment(50, -WidgetHelper.OUTER_SPACING / 2);
      origin = WidgetHelper.createLabeledCombo(groupData, SWT.READ_ONLY, "Origin", fd);
      origin.add("Internal");
      origin.add("NetXMS Agent");
      origin.add("SNMP");
      origin.add("Check Point SNMP");
      origin.add("Push");
      origin.select(dci.getOrigin());
      origin.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@Override
			public void widgetSelected(SelectionEvent e)
			{
				onOriginChange();
			}
      });
      
      fd = new FormData();
      fd.left = new FormAttachment(50, WidgetHelper.OUTER_SPACING / 2);
      fd.top = new FormAttachment(parameter, WidgetHelper.OUTER_SPACING, SWT.BOTTOM);
      fd.right = new FormAttachment(100, 0);
      dataType = WidgetHelper.createLabeledCombo(groupData, SWT.READ_ONLY, "Data Type", fd);
      dataType.add("Integer");
      dataType.add("Unsigned Integer");
      dataType.add("Integer 64 bit");
      dataType.add("Unsigned Integer 64 bit");
      dataType.add("String");
      dataType.add("Floating Point Number");
      dataType.select(dci.getDataType());

      checkInterpretRawSnmpValue = new Button(groupData, SWT.CHECK);
      checkInterpretRawSnmpValue.setText("Interpret SNMP octet string raw value as");
      checkInterpretRawSnmpValue.setSelection(dci.isSnmpRawValueInOctetString());
      checkInterpretRawSnmpValue.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
		      snmpRawType.setEnabled(checkInterpretRawSnmpValue.getSelection());
			}
			
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		});
      fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(origin.getParent(), WidgetHelper.OUTER_SPACING, SWT.BOTTOM);
      checkInterpretRawSnmpValue.setLayoutData(fd);
      checkInterpretRawSnmpValue.setEnabled(dci.getOrigin() == DataCollectionItem.SNMP);

      snmpRawType = new Combo(groupData, SWT.BORDER | SWT.READ_ONLY);
      for(int i = 0; i < snmpRawTypes.length; i++)
      	snmpRawType.add(snmpRawTypes[i]);
      snmpRawType.select(dci.getSnmpRawValueType());
      snmpRawType.setEnabled((dci.getOrigin() == DataCollectionItem.SNMP) && dci.isSnmpRawValueInOctetString());
      fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(checkInterpretRawSnmpValue, WidgetHelper.OUTER_SPACING, SWT.BOTTOM);
      fd.right = new FormAttachment(checkInterpretRawSnmpValue, 0, SWT.RIGHT);
      snmpRawType.setLayoutData(fd);
      
      checkUseCustomSnmpPort = new Button(groupData, SWT.CHECK);
      checkUseCustomSnmpPort.setText("Use custom SNMP port:");
      checkUseCustomSnmpPort.setSelection(dci.getSnmpPort() != 0);
      checkUseCustomSnmpPort.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
		      customSnmpPort.setEnabled(checkUseCustomSnmpPort.getSelection());
			}
			
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		});
      fd = new FormData();
      fd.left = new FormAttachment(checkInterpretRawSnmpValue, WidgetHelper.OUTER_SPACING, SWT.RIGHT);
      fd.right = new FormAttachment(100, 0);
      fd.top = new FormAttachment(dataType.getParent(), WidgetHelper.OUTER_SPACING, SWT.BOTTOM);
      checkUseCustomSnmpPort.setLayoutData(fd);
      checkUseCustomSnmpPort.setEnabled(dci.getOrigin() == DataCollectionItem.SNMP);

      customSnmpPort = new Text(groupData, SWT.BORDER);
      if ((dci.getOrigin() == DataCollectionItem.SNMP) && (dci.getSnmpPort() != 0))
      {
      	customSnmpPort.setEnabled(true);
      	customSnmpPort.setText(Integer.toString(dci.getSnmpPort()));
      }
      else
      {
      	customSnmpPort.setEnabled(false);
      }
      fd = new FormData();
      fd.left = new FormAttachment(checkInterpretRawSnmpValue, WidgetHelper.OUTER_SPACING, SWT.RIGHT);
      fd.right = new FormAttachment(100, 0);
      fd.top = new FormAttachment(checkUseCustomSnmpPort, WidgetHelper.OUTER_SPACING, SWT.BOTTOM);
      customSnmpPort.setLayoutData(fd);
      
      proxyNode = new ObjectSelector(groupData, SWT.NONE);
      proxyNode.setLabel("Proxy node");
      fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(snmpRawType, WidgetHelper.OUTER_SPACING, SWT.BOTTOM);
      fd.right = new FormAttachment(100, 0);
      proxyNode.setLayoutData(fd);
      proxyNode.setObjectClass(Node.class);
      proxyNode.setObjectId(dci.getProxyNode());
      proxyNode.setEnabled(dci.getOrigin() != DataCollectionItem.PUSH);
      
      /** polling area **/
      Group groupPolling = new Group(dialogArea, SWT.NONE);
      groupPolling.setText("Polling");
      FormLayout pollingLayout = new FormLayout();
      pollingLayout.marginHeight = WidgetHelper.OUTER_SPACING;
      pollingLayout.marginWidth = WidgetHelper.OUTER_SPACING;
      groupPolling.setLayout(pollingLayout);
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      groupPolling.setLayoutData(gd);
      
      fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.right = new FormAttachment(50, -WidgetHelper.OUTER_SPACING / 2);
      fd.top = new FormAttachment(0, 0);
      schedulingMode = WidgetHelper.createLabeledCombo(groupPolling, SWT.READ_ONLY, "Polling mode", fd);
      schedulingMode.add("Fixed intervals");
      schedulingMode.add("Custom schedule");
      schedulingMode.select(dci.isUseAdvancedSchedule() ? 1 : 0);
      schedulingMode.setEnabled(dci.getOrigin() != DataCollectionItem.PUSH);
      schedulingMode.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@Override
			public void widgetSelected(SelectionEvent e)
			{
				pollingInterval.getTextControl().setEnabled(schedulingMode.getSelectionIndex() == 0);
			}
      });
      
      pollingInterval = new LabeledText(groupPolling, SWT.NONE);
      pollingInterval.getTextControl().setTextLimit(5);
      pollingInterval.setLabel("Polling interval (seconds)");
      pollingInterval.setText(Integer.toString(dci.getPollingInterval()));
      pollingInterval.setEnabled(!dci.isUseAdvancedSchedule() && (dci.getOrigin() != DataCollectionItem.PUSH));
      fd = new FormData();
      fd.left = new FormAttachment(50, WidgetHelper.OUTER_SPACING / 2);
      fd.right = new FormAttachment(100, 0);
      fd.top = new FormAttachment(0, 0);
      pollingInterval.setLayoutData(fd);
      pollingInterval.getTextControl().addListener(SWT.Modify, new Listener() {
			@Override
			public void handleEvent(Event event)
			{
				try
				{
					new Integer(pollingInterval.getText());
					General.this.setErrorMessage(null);
					pollingInterval.getTextControl().setBackground(null);
				}
				catch(NumberFormatException e)
				{
					General.this.setErrorMessage("Invalid number entered in \"Polling interval\" field");
					pollingInterval.getTextControl().setBackground(SharedColors.RED);
				}
			}
      });
      
      fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.right = new FormAttachment(100, 0);
      fd.top = new FormAttachment(schedulingMode.getParent(), WidgetHelper.OUTER_SPACING, SWT.BOTTOM);
      clusterResource = WidgetHelper.createLabeledCombo(groupPolling, SWT.READ_ONLY, "Associate with cluster resource", fd);
      clusterResource.add("<none>");
      clusterResource.select(0);
      clusterResource.setEnabled(dci.getResourceId() != 0);
      	
      /** status **/
      Group groupStatus = new Group(dialogArea, SWT.NONE);
      groupStatus.setText("Status");
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      groupStatus.setLayoutData(gd);
      RowLayout statusLayout = new RowLayout();
      statusLayout.type = SWT.VERTICAL;
      groupStatus.setLayout(statusLayout);
      
      statusActive = new Button(groupStatus, SWT.RADIO);
      statusActive.setText("&Active");
      statusActive.setSelection(dci.getStatus() == DataCollectionItem.ACTIVE);
      
      statusDisabled = new Button(groupStatus, SWT.RADIO);
      statusDisabled.setText("&Disabled");
      statusDisabled.setSelection(dci.getStatus() == DataCollectionItem.DISABLED);
      
      statusUnsupported = new Button(groupStatus, SWT.RADIO);
      statusUnsupported.setText("&Not supported");
      statusUnsupported.setSelection(dci.getStatus() == DataCollectionItem.NOT_SUPPORTED);
      
      /** storage **/
      Group groupStorage = new Group(dialogArea, SWT.NONE);
      groupStorage.setText("Storage");
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.horizontalSpan = 2;
      groupStorage.setLayoutData(gd);
      FillLayout storageLayout = new FillLayout();
      storageLayout.marginWidth = WidgetHelper.OUTER_SPACING;
      storageLayout.marginHeight = WidgetHelper.OUTER_SPACING;
      groupStorage.setLayout(storageLayout);
      
      retentionTime = new LabeledText(groupStorage, SWT.NONE);
      retentionTime.setLabel("Retention time (days)");
      retentionTime.getTextControl().setTextLimit(5);
      retentionTime.setText(Integer.toString(dci.getRetentionTime()));
      retentionTime.getTextControl().addListener(SWT.Modify, new Listener() {
			@Override
			public void handleEvent(Event event)
			{
				try
				{
					new Integer(retentionTime.getText());
					General.this.setErrorMessage(null);
					retentionTime.getTextControl().setBackground(null);
				}
				catch(NumberFormatException e)
				{
					General.this.setErrorMessage("Invalid number entered in \"Retention time\" field");
					retentionTime.getTextControl().setBackground(SharedColors.RED);
				}
			}
      });
      
      return dialogArea;
	}

	/**
	 * Handler for changing item origin
	 */
	private void onOriginChange()
	{
		int index = origin.getSelectionIndex();
		proxyNode.setEnabled(index != DataCollectionItem.PUSH);
		schedulingMode.setEnabled(index != DataCollectionItem.PUSH);
		pollingInterval.getTextControl().setEnabled((index != DataCollectionItem.PUSH) && (schedulingMode.getSelectionIndex() == 0));
		checkInterpretRawSnmpValue.setEnabled(index == DataCollectionItem.SNMP);
		snmpRawType.setEnabled((index == DataCollectionItem.SNMP) && checkInterpretRawSnmpValue.getSelection());
		checkUseCustomSnmpPort.setEnabled(index == DataCollectionItem.SNMP);
		customSnmpPort.setEnabled((index == DataCollectionItem.SNMP) && checkUseCustomSnmpPort.getSelection());
	}
	
	/**
	 * Select parameter
	 */
	private void selectParameter()
	{
		Dialog dlg;
		switch(origin.getSelectionIndex())
		{
			case DataCollectionItem.INTERNAL:
				dlg = new SelectInternalParamDlg(getShell(), dci.getNodeId());
				break;
			case DataCollectionItem.AGENT:
				dlg = new SelectAgentParamDlg(getShell(), dci.getNodeId());
				break;
			case DataCollectionItem.SNMP:
			case DataCollectionItem.CHECKPOINT_SNMP:
				SnmpObjectId oid;
				try
				{
					oid = SnmpObjectId.parseSnmpObjectId(parameter.getText());
				}
				catch(SnmpObjectIdFormatException e)
				{
					oid = null;
				}
				dlg = new SelectSnmpParamDlg(getShell(), oid);
				break;
			default:
				dlg = null;
				break;
		}
		
		if ((dlg != null) && (dlg.open() == Window.OK))
		{
			IParameterSelectionDialog pd = (IParameterSelectionDialog)dlg;
			description.setText(pd.getParameterDescription());
			parameter.setText(pd.getParameterName());
			dataType.select(pd.getParameterDataType());
		}
	}
	
	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
	protected void applyChanges(final boolean isApply)
	{
		if (!WidgetHelper.validateTextInput(customSnmpPort, "Custom SNMP port", new NumericTextFieldValidator(1, 65535)) ||
		    !WidgetHelper.validateTextInput(pollingInterval, new NumericTextFieldValidator(1, 1000000)) ||
		    !WidgetHelper.validateTextInput(retentionTime, new NumericTextFieldValidator(1, 65535)))
			return;
		
		if (isApply)
			setValid(false);
		
		dci.setDescription(description.getText());
		dci.setName(parameter.getText());
		dci.setOrigin(origin.getSelectionIndex());
		dci.setDataType(dataType.getSelectionIndex());
		dci.setProxyNode(proxyNode.getObjectId());
		dci.setUseAdvancedSchedule(schedulingMode.getSelectionIndex() == 1);
		dci.setPollingInterval(Integer.parseInt(pollingInterval.getText()));
		dci.setRetentionTime(Integer.parseInt(retentionTime.getText()));
		dci.setSnmpRawValueInOctetString(checkInterpretRawSnmpValue.getSelection());
		dci.setSnmpRawValueType(snmpRawType.getSelectionIndex());
		if (checkUseCustomSnmpPort.getSelection())
		{
			dci.setSnmpPort(Integer.parseInt(customSnmpPort.getText()));
		}
		else
		{
			dci.setSnmpPort(0);
		}
		
		if (statusActive.getSelection())
			dci.setStatus(DataCollectionItem.ACTIVE);
		else if (statusDisabled.getSelection())
			dci.setStatus(DataCollectionItem.DISABLED);
		else if (statusUnsupported.getSelection())
			dci.setStatus(DataCollectionItem.NOT_SUPPORTED);

		new ConsoleJob("Update general settings for DCI " + dci.getId(), null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				dci.getOwner().modifyItem(dci);
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						General.this.setValid(true);
					}
				});
			}

			@Override
			protected String getErrorMessage()
			{
				return "Cannot update general DCI settings";
			}
		}.start();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performOk()
	 */
	@Override
	public boolean performOk()
	{
		applyChanges(false);
		return true;
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
		schedulingMode.select(0);
		pollingInterval.setText("60");
		statusActive.setSelection(true);
		statusDisabled.setSelection(false);
		statusUnsupported.setSelection(false);
		retentionTime.setText("30");
		checkInterpretRawSnmpValue.setSelection(false);
		checkUseCustomSnmpPort.setSelection(false);
	}
}
