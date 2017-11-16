/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2017 Raden Solutions
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

import java.util.Arrays;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Spinner;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.ui.eclipse.datacollection.Messages;
import org.netxms.ui.eclipse.datacollection.api.DataCollectionObjectEditor;
import org.netxms.ui.eclipse.datacollection.propertypages.helpers.DCIPropertyPageDialog;
import org.netxms.ui.eclipse.nxsl.widgets.ScriptEditor;
import org.netxms.ui.eclipse.tools.WidgetFactory;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * "Instance Discovery" property page for DCO
 */
public class InstanceDiscovery extends DCIPropertyPageDialog
{
	private static final String[] DCI_FUNCTIONS = { "FindDCIByName", "FindDCIByDescription", "GetDCIObject", "GetDCIValue", "GetDCIValueByDescription", "GetDCIValueByName" }; //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$ //$NON-NLS-4$ //$NON-NLS-5$ //$NON-NLS-6$
	private static final String[] DCI_VARIABLES = { "$dci", "$node" }; //$NON-NLS-1$ //$NON-NLS-2$
	
	private DataCollectionObjectEditor editor;
	private DataCollectionObject dco;
	private Combo discoveryMethod;
	private LabeledText discoveryData;
	private ScriptEditor filterScript;
	private Group groupRetention;
   private Combo instanceRetentionMode;
	private Spinner instanceRetentionTime;
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
	   Composite dialogArea = (Composite)super.createContents(parent);
		editor = (DataCollectionObjectEditor)getElement().getAdapter(DataCollectionObjectEditor.class);
		dco = editor.getObject();
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
      dialogArea.setLayout(layout);

      discoveryMethod = WidgetHelper.createLabeledCombo(dialogArea, SWT.BORDER | SWT.READ_ONLY, Messages.get().InstanceDiscovery_Method,
                                                         WidgetHelper.DEFAULT_LAYOUT_DATA);
      discoveryMethod.add(Messages.get().InstanceDiscovery_None);
      discoveryMethod.add(Messages.get().InstanceDiscovery_AgentList);
      discoveryMethod.add(Messages.get().InstanceDiscovery_AgentTable);
      discoveryMethod.add(Messages.get().InstanceDiscovery_SnmpWalkValues);
      discoveryMethod.add(Messages.get().InstanceDiscovery_SnmpWalkOids);
      discoveryMethod.add(Messages.get().InstanceDiscovery_Script);
      discoveryMethod.select(dco.getInstanceDiscoveryMethod());
      discoveryMethod.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				int method = discoveryMethod.getSelectionIndex();
		      discoveryData.setLabel(getDataLabel(method));
		      discoveryData.setEnabled(method != DataCollectionObject.IDM_NONE);
		      filterScript.setEnabled(method != DataCollectionObject.IDM_NONE);
            instanceRetentionMode.setEnabled(method != DataCollectionObject.IDM_NONE);
		      instanceRetentionTime.setEnabled(method != DataCollectionObject.IDM_NONE && instanceRetentionMode.getSelectionIndex() > 0);
			}
			
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		});
      
      discoveryData = new LabeledText(dialogArea, SWT.NONE);
      discoveryData.setLabel(getDataLabel(dco.getInstanceDiscoveryMethod()));
      discoveryData.setText(dco.getInstanceDiscoveryData());
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      discoveryData.setLayoutData(gd);
      discoveryData.setEnabled(dco.getInstanceDiscoveryMethod() != DataCollectionObject.IDM_NONE);
      
      groupRetention = new Group(dialogArea, SWT.NONE);
      groupRetention.setText("Instance retention");
      gd = new GridData();
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.horizontalSpan = 2;
      groupRetention.setLayoutData(gd);
      GridLayout retentionLayout = new GridLayout();
      retentionLayout.numColumns = 2;
      retentionLayout.horizontalSpacing = WidgetHelper.OUTER_SPACING;
      groupRetention.setLayout(retentionLayout);
      
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      instanceRetentionMode = WidgetHelper.createLabeledCombo(groupRetention, SWT.READ_ONLY, "Instance retention mode", gd);
      instanceRetentionMode.add("Server default");
      instanceRetentionMode.add("Custom");
      instanceRetentionMode.select(dco.getInstanceRetentionTime() == -1 ? 0 : 1);
      instanceRetentionMode.setEnabled(dco.getInstanceDiscoveryMethod() != DataCollectionObject.IDM_NONE);
      instanceRetentionMode.addSelectionListener(new SelectionListener() {
          /* (non-Javadoc)
          * @see org.eclipse.swt.events.SelectionListener#widgetSelected(org.eclipse.swt.events.SelectionEvent)
          */
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            instanceRetentionTime.setEnabled(instanceRetentionMode.getSelectionIndex() == 1);
         }
         /* (non-Javadoc)
          * @see org.eclipse.swt.events.SelectionListener#widgetDefaultSelected(org.eclipse.swt.events.SelectionEvent)
          */
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
           widgetSelected(e); 
         }
      });
      
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      instanceRetentionTime = WidgetHelper.createLabeledSpinner(groupRetention, SWT.BORDER, "Instance retention time (days)", 0, 100,  new GridData());
      instanceRetentionTime.setSelection(dco.getInstanceRetentionTime());
      instanceRetentionTime.setEnabled(instanceRetentionMode.getSelectionIndex() > 0);
      
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.widthHint = 0;
      gd.heightHint = 0;
      final WidgetFactory factory = new WidgetFactory() {
			@Override
			public Control createControl(Composite parent, int style)
			{
				return new ScriptEditor(parent, style, SWT.H_SCROLL | SWT.V_SCROLL, false, 
				      "Variables:\r\n\t$1\tInstance to test\r\n\r\nReturn value:\r\n\ttrue/false to accept or reject instance without additional changes or\r\n\tarray of two or three elements to modify instance:\r\n\t\t1st element - true/false to indicate acceptance;\r\n\t\t2nd element - new instance name;\r\n\t\t3rd element - new instance display name.");
			}
      };
      filterScript = (ScriptEditor)WidgetHelper.createLabeledControl(dialogArea, SWT.BORDER,
                                                                             factory, Messages.get().InstanceDiscovery_FilterScript, gd);
      filterScript.addFunctions(Arrays.asList(DCI_FUNCTIONS));
      filterScript.addVariables(Arrays.asList(DCI_VARIABLES));
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      filterScript.setLayoutData(gd);
      filterScript.setText(dco.getInstanceDiscoveryFilter());
      filterScript.setEnabled(dco.getInstanceDiscoveryMethod() != DataCollectionObject.IDM_NONE);
      
		return dialogArea;
	}
	
	/**
	 * Get label for data field
	 * 
	 * @param method
	 * @return
	 */
	private static String getDataLabel(int method)
	{
		switch(method)
		{
			case DataCollectionObject.IDM_NONE:
				return Messages.get().InstanceDiscovery_DiscoveryData;
			case DataCollectionObject.IDM_AGENT_LIST:
				return Messages.get().InstanceDiscovery_ListName;
			case DataCollectionObject.IDM_AGENT_TABLE:
				return Messages.get().InstanceDiscovery_TableName;
			case DataCollectionObject.IDM_SNMP_WALK_VALUES:
			case DataCollectionObject.IDM_SNMP_WALK_OIDS:
				return Messages.get().InstanceDiscovery_BaseOid;
		}
		return ""; //$NON-NLS-1$
	}
	
	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
	protected void applyChanges(final boolean isApply)
	{
	   dco.setInstanceDiscoveryMethod(discoveryMethod.getSelectionIndex());
	   dco.setInstanceDiscoveryData(discoveryData.getText());
	   dco.setInstanceDiscoveryFilter(filterScript.getText());
	   if (instanceRetentionMode.getSelectionIndex() == 0)
	      dco.setInstanceRetentionTime(-1);
	   else
	      dco.setInstanceRetentionTime(instanceRetentionTime.getSelection());
		editor.modify();
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
		discoveryMethod.select(DataCollectionObject.IDM_NONE);
		discoveryData.setLabel(getDataLabel(DataCollectionObject.IDM_NONE));
		discoveryData.setText(""); //$NON-NLS-1$
		discoveryData.setEnabled(false);
		filterScript.setText(""); //$NON-NLS-1$
		filterScript.setEnabled(false);
		instanceRetentionMode.select(0);
		instanceRetentionTime.setSelection(0);
	}
}
