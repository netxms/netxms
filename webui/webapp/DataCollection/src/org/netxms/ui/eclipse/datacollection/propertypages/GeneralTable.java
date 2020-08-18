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
package org.netxms.ui.eclipse.datacollection.propertypages;

import java.util.HashMap;
import java.util.Map;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Spinner;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.AgentCacheMode;
import org.netxms.client.constants.DataOrigin;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.datacollection.DataCollectionTable;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.ClusterResource;
import org.netxms.client.objects.Node;
import org.netxms.client.snmp.SnmpObjectId;
import org.netxms.client.snmp.SnmpObjectIdFormatException;
import org.netxms.client.snmp.SnmpVersion;
import org.netxms.ui.eclipse.datacollection.Messages;
import org.netxms.ui.eclipse.datacollection.dialogs.IParameterSelectionDialog;
import org.netxms.ui.eclipse.datacollection.dialogs.SelectAgentParamDlg;
import org.netxms.ui.eclipse.datacollection.dialogs.SelectInternalParamDlg;
import org.netxms.ui.eclipse.datacollection.dialogs.SelectParameterScriptDialog;
import org.netxms.ui.eclipse.datacollection.dialogs.SelectSnmpParamDlg;
import org.netxms.ui.eclipse.datacollection.propertypages.helpers.DCIPropertyPageDialog;
import org.netxms.ui.eclipse.objectbrowser.widgets.ObjectSelector;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * "General" property page for table DCI
 */
public class GeneralTable extends DCIPropertyPageDialog
{
	private DataCollectionTable dci;
	private AbstractObject owner;
	private Cluster cluster = null;
	private Map<Integer, Long> clusterResourceMap;
	private Text description;
	private LabeledText parameter;
	private Button selectButton;
	private Combo origin;
	private Button checkUseCustomSnmpPort;
	private Spinner customSnmpPort;
   private Button checkUseCustomSnmpVersion;
   private Combo customSnmpVersion;
	private ObjectSelector sourceNode;
   private Combo agentCacheMode;
	private Combo schedulingMode;
   private Combo retentionMode;
	private LabeledText pollingInterval;
	private LabeledText retentionTime;
	private Combo clusterResource;
	
   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
	   Composite dialogArea = (Composite)super.createContents(parent);
	   
		dci = editor.getObjectAsTable();
		
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		owner = session.findObjectById(dci.getNodeId());
		
		if (owner instanceof Cluster)
		{
			cluster = (Cluster)owner;
		}
		else if (owner instanceof AbstractNode)
		{
			for(AbstractObject o : owner.getParentsAsArray())
			{
				if (o instanceof Cluster)
				{
					cluster = (Cluster)o;
					break;
				}
			}
		}
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		layout.numColumns = 2;
      dialogArea.setLayout(layout);
      
      /** description area **/
      Group groupDescription = new Group(dialogArea, SWT.NONE);
      groupDescription.setText(Messages.get().GeneralTable_Description);
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
      groupData.setText(Messages.get().GeneralTable_Data);
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
      parameter.setLabel(Messages.get().GeneralTable_Parameter);
      parameter.getTextControl().setTextLimit(255);
      parameter.setText(dci.getName());
      
      selectButton = new Button(groupData, SWT.PUSH);
      selectButton.setText(Messages.get().GeneralTable_Select);
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
      fd.right = new FormAttachment(40, -WidgetHelper.OUTER_SPACING / 2);
      origin = WidgetHelper.createLabeledCombo(groupData, SWT.READ_ONLY, Messages.get().GeneralTable_Origin, fd);
      origin.add(Messages.get().DciLabelProvider_SourceInternal);
      origin.add(Messages.get().DciLabelProvider_SourceAgent);
      origin.add(Messages.get().DciLabelProvider_SourceSNMP);
      origin.add(Messages.get().DciLabelProvider_SourceWebService);
      origin.add(Messages.get().DciLabelProvider_SourcePush);
      origin.add(Messages.get().DciLabelProvider_SourceWinPerf);
      origin.add(Messages.get().DciLabelProvider_SourceILO);
      origin.add(Messages.get().DciLabelProvider_SourceScript);
      origin.add(Messages.get().DciLabelProvider_SourceSSH);
      origin.add(Messages.get().DciLabelProvider_SourceMQTT);
      origin.select(dci.getOrigin().getValue());
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
      
      checkUseCustomSnmpPort = new Button(groupData, SWT.CHECK);
      checkUseCustomSnmpPort.setText(Messages.get().GeneralTable_UseCustomSNMPPort);
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
      fd.left = new FormAttachment(origin.getParent(), WidgetHelper.OUTER_SPACING, SWT.RIGHT);
      fd.top = new FormAttachment(parameter, WidgetHelper.OUTER_SPACING, SWT.BOTTOM);
      checkUseCustomSnmpPort.setLayoutData(fd);
      checkUseCustomSnmpPort.setEnabled(dci.getOrigin() == DataOrigin.SNMP);

      customSnmpPort = new Spinner(groupData, SWT.BORDER);
      customSnmpPort.setMinimum(1);
      customSnmpPort.setMaximum(65535);
      if ((dci.getOrigin() == DataOrigin.SNMP) && (dci.getSnmpPort() != 0))
      {
         customSnmpPort.setEnabled(true);
         customSnmpPort.setSelection(dci.getSnmpPort());
      }
      else
      {
         customSnmpPort.setEnabled(false);
      }
      fd = new FormData();
      fd.left = new FormAttachment(origin.getParent(), WidgetHelper.OUTER_SPACING, SWT.RIGHT);
      fd.right = new FormAttachment(checkUseCustomSnmpPort, 0, SWT.RIGHT);
      fd.top = new FormAttachment(checkUseCustomSnmpPort, WidgetHelper.OUTER_SPACING, SWT.BOTTOM);
      customSnmpPort.setLayoutData(fd);

      checkUseCustomSnmpVersion = new Button(groupData, SWT.CHECK);
      checkUseCustomSnmpVersion.setText("Use custom SNMP version:");
      checkUseCustomSnmpVersion.setSelection(dci.getSnmpVersion() != SnmpVersion.DEFAULT);
      checkUseCustomSnmpVersion.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            customSnmpVersion.setEnabled(checkUseCustomSnmpVersion.getSelection());
         }

         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }
      });
      fd = new FormData();
      fd.left = new FormAttachment(checkUseCustomSnmpPort, WidgetHelper.OUTER_SPACING, SWT.RIGHT);
      fd.top = new FormAttachment(parameter, WidgetHelper.OUTER_SPACING, SWT.BOTTOM);
      fd.right = new FormAttachment(100, 0);
      checkUseCustomSnmpVersion.setLayoutData(fd);
      checkUseCustomSnmpVersion.setEnabled(dci.getOrigin() == DataOrigin.SNMP);

      customSnmpVersion = new Combo(groupData, SWT.BORDER | SWT.READ_ONLY);
      customSnmpVersion.add("1");
      customSnmpVersion.add("2c");
      customSnmpVersion.add("3");
      customSnmpVersion.select(indexFromSnmpVersion(dci.getSnmpVersion()));
      customSnmpVersion.setEnabled(dci.getSnmpVersion() != SnmpVersion.DEFAULT);
      fd = new FormData();
      fd.left = new FormAttachment(checkUseCustomSnmpPort, WidgetHelper.OUTER_SPACING, SWT.RIGHT);
      fd.top = new FormAttachment(checkUseCustomSnmpVersion, WidgetHelper.OUTER_SPACING, SWT.BOTTOM);
      fd.right = new FormAttachment(100, 0);
      customSnmpVersion.setLayoutData(fd);

      sourceNode = new ObjectSelector(groupData, SWT.NONE, true);
      sourceNode.setLabel(Messages.get().GeneralTable_ProxyNode);
      sourceNode.setObjectClass(Node.class);
      sourceNode.setObjectId(dci.getSourceNode());
      sourceNode.setEnabled(dci.getOrigin() != DataOrigin.PUSH);
      sourceNode.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            editor.setSourceNode(sourceNode.getObjectId());
         }
      });
      
      fd = new FormData();
      fd.top = new FormAttachment(customSnmpPort, WidgetHelper.OUTER_SPACING, SWT.BOTTOM);
      fd.right = new FormAttachment(100, 0);
      agentCacheMode = WidgetHelper.createLabeledCombo(groupData, SWT.READ_ONLY, Messages.get().GeneralTable_AgentCacheMode, fd);
      agentCacheMode.add(Messages.get().GeneralTable_Default);
      agentCacheMode.add(Messages.get().GeneralTable_On);
      agentCacheMode.add(Messages.get().GeneralTable_Off);
      agentCacheMode.select(dci.getCacheMode().getValue());
      agentCacheMode.setEnabled((dci.getOrigin() == DataOrigin.AGENT) || (dci.getOrigin() == DataOrigin.SNMP));

      fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(customSnmpPort, WidgetHelper.OUTER_SPACING, SWT.BOTTOM);
      fd.right = new FormAttachment(agentCacheMode.getParent(), -WidgetHelper.OUTER_SPACING, SWT.LEFT);
      sourceNode.setLayoutData(fd);
      
      /** polling area **/
      Group groupPolling = new Group(dialogArea, SWT.NONE);
      groupPolling.setText(Messages.get().GeneralTable_Polling);
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
      schedulingMode = WidgetHelper.createLabeledCombo(groupPolling, SWT.READ_ONLY, Messages.get().GeneralTable_PollingMode, fd);
      schedulingMode.add(Messages.get().General_FixedIntervalsDefault);
      schedulingMode.add(Messages.get().General_FixedIntervalsCustom);
      schedulingMode.add(Messages.get().General_CustomSchedule);
      schedulingMode.select(dci.getPollingScheduleType());
      schedulingMode.setEnabled(dci.getOrigin() != DataOrigin.PUSH);
      schedulingMode.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@Override
			public void widgetSelected(SelectionEvent e)
			{
				pollingInterval.setEnabled(schedulingMode.getSelectionIndex() == 1);
			}
      });
      
      pollingInterval = new LabeledText(groupPolling, SWT.NONE);
      pollingInterval.setLabel(Messages.get().General_PollingInterval);
      pollingInterval.setText(dci.getPollingInterval());
      pollingInterval.setEnabled((dci.getPollingScheduleType() == DataCollectionObject.POLLING_SCHEDULE_CUSTOM) && (dci.getOrigin() != DataOrigin.PUSH));
      fd = new FormData();
      fd.left = new FormAttachment(50, WidgetHelper.OUTER_SPACING / 2);
      fd.right = new FormAttachment(100, 0);
      fd.top = new FormAttachment(0, 0);
      pollingInterval.setLayoutData(fd);
      
      fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.right = new FormAttachment(100, 0);
      fd.top = new FormAttachment(schedulingMode.getParent(), WidgetHelper.OUTER_SPACING, SWT.BOTTOM);
      clusterResource = WidgetHelper.createLabeledCombo(groupPolling, SWT.READ_ONLY, Messages.get().GeneralTable_ClRes, fd);
      if (cluster != null)
      {
      	clusterResourceMap = new HashMap<Integer, Long>();
      	clusterResourceMap.put(0, 0L);
      	
	      clusterResource.add(Messages.get().GeneralTable_None);
	      if (dci.getResourceId() == 0)
	      	clusterResource.select(0);
	      
	      int index = 1;
	      for (ClusterResource r : cluster.getResources())
	      {
	      	clusterResource.add(r.getName());
	      	clusterResourceMap.put(index, r.getId());
		      if (dci.getResourceId() == r.getId())
		      	clusterResource.select(index);
	      	index++;
	      }
      }
      else
      {
	      clusterResource.add(Messages.get().GeneralTable_None);
	      clusterResource.select(0);
	      clusterResource.setEnabled(false);
      }
      	
      /** storage **/
      Group groupStorage = new Group(dialogArea, SWT.NONE);
      groupStorage.setText(Messages.get().GeneralTable_Storage);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.horizontalSpan = 2;
      groupStorage.setLayoutData(gd);
      GridLayout storageLayout = new GridLayout();
      storageLayout.numColumns = 2;
      storageLayout.horizontalSpacing = WidgetHelper.OUTER_SPACING;
      groupStorage.setLayout(storageLayout);
      
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      retentionMode = WidgetHelper.createLabeledCombo(groupStorage, SWT.READ_ONLY, Messages.get().GeneralTable_RetentionMode, gd);
      retentionMode.add(Messages.get().GeneralTable_UseDefaultRetention);
      retentionMode.add(Messages.get().GeneralTable_UseCustomRetention);
      retentionMode.add(Messages.get().GeneralTable_NoStorage);
      retentionMode.select(dci.getRetentionType());
      retentionMode.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }

         @Override
         public void widgetSelected(SelectionEvent e)
         {
            int mode = retentionMode.getSelectionIndex();
            retentionTime.setEnabled(mode == 1);
         }
      });

      retentionTime = new LabeledText(groupStorage, SWT.NONE);
      retentionTime.setLabel(Messages.get().GeneralTable_RetentionTime);
      retentionTime.setText(dci.getRetentionTime());
      retentionTime.setEnabled(dci.getRetentionType() == DataCollectionObject.RETENTION_CUSTOM);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      retentionTime.setLayoutData(gd);
      
      return dialogArea;
	}

   /**
    * Calculate selection index from SNMP version
    * 
    * @param version
    * @return
    */
   private static int indexFromSnmpVersion(SnmpVersion version)
   {
      switch(version)
      {
         case V1:
            return 0;
         case V2C:
            return 1;
         case V3:
            return 2;
         default:
            return -1;
      }
   }

   /**
    * Get SNMP version value from selection index
    * 
    * @param index
    * @return
    */
   private static SnmpVersion indexToSnmpVersion(int index)
   {
      switch(index)
      {
         case 0:
            return SnmpVersion.V1;
         case 1:
            return SnmpVersion.V2C;
         case 2:
            return SnmpVersion.V3;
         default:
            return SnmpVersion.DEFAULT;
      }
   }

	/**
	 * Handler for changing item origin
	 */
	private void onOriginChange()
	{
      DataOrigin dataOrigin = DataOrigin.getByValue(origin.getSelectionIndex());
		sourceNode.setEnabled(dataOrigin != DataOrigin.PUSH);
      schedulingMode.setEnabled((dataOrigin != DataOrigin.PUSH) && (dataOrigin != DataOrigin.MQTT));
      pollingInterval.setEnabled((dataOrigin != DataOrigin.PUSH) && (dataOrigin != DataOrigin.MQTT) && (schedulingMode.getSelectionIndex() == 1));
		checkUseCustomSnmpPort.setEnabled(dataOrigin == DataOrigin.SNMP);
		customSnmpPort.setEnabled((dataOrigin == DataOrigin.SNMP) && checkUseCustomSnmpPort.getSelection());
      checkUseCustomSnmpVersion.setEnabled(dataOrigin == DataOrigin.SNMP);
      customSnmpVersion.setEnabled((dataOrigin == DataOrigin.SNMP) && checkUseCustomSnmpVersion.getSelection());
      agentCacheMode.setEnabled((dataOrigin == DataOrigin.AGENT) || (dataOrigin == DataOrigin.SNMP));
      selectButton.setEnabled(
            (dataOrigin == DataOrigin.AGENT) || 
            (dataOrigin == DataOrigin.SNMP) || 
            (dataOrigin == DataOrigin.INTERNAL) || 
            (dataOrigin == DataOrigin.WINPERF) || 
            (dataOrigin == DataOrigin.WEB_SERVICE) || 
            (dataOrigin == DataOrigin.SCRIPT));
	}
	
	/**
	 * Select parameter
	 */
	private void selectParameter()
	{
		Dialog dlg;
		editor.setSourceNode(sourceNode.getObjectId());
      DataOrigin dataOrigin = DataOrigin.getByValue(origin.getSelectionIndex());
      switch(dataOrigin)
		{
         case INTERNAL:
            if (sourceNode.getObjectId() != 0)
               dlg = new SelectInternalParamDlg(getShell(), sourceNode.getObjectId(), true);
            else
               dlg = new SelectInternalParamDlg(getShell(), dci.getNodeId(), true);
            break;            
         case AGENT:
			   if (sourceNode.getObjectId() != 0)
               dlg = new SelectAgentParamDlg(getShell(), sourceNode.getObjectId(), dataOrigin, true);
            else
               dlg = new SelectAgentParamDlg(getShell(), dci.getNodeId(), dataOrigin, true);
				break;
         case SNMP:
				SnmpObjectId oid;
				try
				{
					oid = SnmpObjectId.parseSnmpObjectId(parameter.getText());
				}
				catch(SnmpObjectIdFormatException e)
				{
					oid = null;
				}
				if (sourceNode.getObjectId() != 0)
               dlg = new SelectSnmpParamDlg(getShell(), oid, sourceNode.getObjectId());
            else
               dlg = new SelectSnmpParamDlg(getShell(), oid, dci.getNodeId());
				break;
         case SCRIPT:
            dlg = new SelectParameterScriptDialog(getShell());
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
         editor.fireOnSelectTableListeners(DataOrigin.getByValue(origin.getSelectionIndex()), pd.getParameterName(),
               pd.getParameterDescription());
		}
	}
	
	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
	protected boolean applyChanges(final boolean isApply)
	{
		dci.setDescription(description.getText().trim());
		dci.setName(parameter.getText().trim());
      dci.setOrigin(DataOrigin.getByValue(origin.getSelectionIndex()));
		dci.setSourceNode(sourceNode.getObjectId());
      dci.setCacheMode(AgentCacheMode.getByValue(agentCacheMode.getSelectionIndex()));
      dci.setPollingScheduleType(schedulingMode.getSelectionIndex());
      dci.setPollingInterval((schedulingMode.getSelectionIndex() == 1) ? pollingInterval.getText() : null);
      dci.setRetentionType(retentionMode.getSelectionIndex());
      dci.setRetentionTime((retentionMode.getSelectionIndex() == 1) ? retentionTime.getText() : null);
		if (checkUseCustomSnmpPort.getSelection())
		{
			dci.setSnmpPort(Integer.parseInt(customSnmpPort.getText()));
		}
		else
		{
			dci.setSnmpPort(0);
		}
      if (checkUseCustomSnmpVersion.getSelection())
      {
         dci.setSnmpVersion(indexToSnmpVersion(customSnmpVersion.getSelectionIndex()));
      }
      else
      {
         dci.setSnmpVersion(SnmpVersion.DEFAULT);
      }

		if (cluster != null)
		{
			dci.setResourceId(clusterResourceMap.get(clusterResource.getSelectionIndex()));
		}

		editor.modify();
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

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performApply()
    */
	@Override
	protected void performApply()
	{
		applyChanges(true);
	}

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performDefaults()
    */
	@Override
	protected void performDefaults()
	{
		super.performDefaults();

		NXCSession session = ConsoleSharedData.getSession();
      
		schedulingMode.select(0);
      pollingInterval.setText(Integer.toString(session.getDefaultDciPollingInterval()));
      retentionMode.select(0);
      retentionTime.setText(Integer.toString(session.getDefaultDciRetentionTime()));
		checkUseCustomSnmpPort.setSelection(false);
	}
}
