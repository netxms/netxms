/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Raden Solutions
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
package org.netxms.nxmc.modules.datacollection.propertypages;

import java.util.Arrays;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Spinner;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.DataCollectionObjectEditor;
import org.netxms.nxmc.modules.nxsl.widgets.ScriptEditor;
import org.netxms.nxmc.tools.WidgetFactory;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Instance Discovery" property page for DCO
 */
public class InstanceDiscovery extends AbstractDCIPropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(InstanceDiscovery.class);   
	private static final String[] DCI_FUNCTIONS = { "FindDCIByName", "FindDCIByDescription", "GetDCIObject", "GetDCIValue", "GetDCIValueByDescription", "GetDCIValueByName" }; //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$ //$NON-NLS-4$ //$NON-NLS-5$ //$NON-NLS-6$
   private static final String[] DCI_VARIABLES = { "$dci", "$node" };

	private DataCollectionObject dco;
	private Combo discoveryMethod;
	private LabeledText discoveryData;
   private LabeledText instanceNameColumn;
	private ScriptEditor filterScript;
	private Group groupRetention;
   private Combo instanceRetentionMode;
	private Spinner instanceRetentionTime;
   
   /**
    * Constructor
    * 
    * @param editor
    */
   public InstanceDiscovery(DataCollectionObjectEditor editor)
   {
      super(LocalizationHelper.getI18n(InstanceDiscovery.class).tr("Instance Discovery"), editor);
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
	   Composite dialogArea = (Composite)super.createContents(parent);
		dco = editor.getObject();
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
      dialogArea.setLayout(layout);

      discoveryMethod = WidgetHelper.createLabeledCombo(dialogArea, SWT.BORDER | SWT.READ_ONLY, i18n.tr("Instance discovery method"),
                                                         WidgetHelper.DEFAULT_LAYOUT_DATA);
      discoveryMethod.add(i18n.tr("None"));
      discoveryMethod.add(i18n.tr("Agent List"));
      discoveryMethod.add(i18n.tr("Agent Table"));
      discoveryMethod.add(i18n.tr("SNMP Walk - Values"));
      discoveryMethod.add(i18n.tr("SNMP Walk - OIDs"));
      discoveryMethod.add(i18n.tr("Script"));
      discoveryMethod.add(i18n.tr("Windows Performance Counters"));
      discoveryMethod.add(i18n.tr("Web Service"));
      discoveryMethod.add(i18n.tr("Internal Table"));
      discoveryMethod.add(i18n.tr("SM-CLP Targets"));
      discoveryMethod.add(i18n.tr("SM-CLP Properties"));
      discoveryMethod.add(i18n.tr("Push"));
      discoveryMethod.select(dco.getInstanceDiscoveryMethod());
      discoveryMethod.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				int method = discoveryMethod.getSelectionIndex();
		      discoveryData.setLabel(getDataLabel(method));
		      discoveryData.setEnabled(method != DataCollectionObject.IDM_NONE);
            instanceNameColumn.setEnabled((method == DataCollectionObject.IDM_AGENT_TABLE) || (method == DataCollectionObject.IDM_INTERNAL_TABLE));
		      filterScript.setEnabled(method != DataCollectionObject.IDM_NONE);
            instanceRetentionMode.setEnabled(method != DataCollectionObject.IDM_NONE);
		      instanceRetentionTime.setEnabled(method != DataCollectionObject.IDM_NONE && instanceRetentionMode.getSelectionIndex() > 0);
			}
		});

      boolean isTableSource = (dco.getInstanceDiscoveryMethod() == DataCollectionObject.IDM_AGENT_TABLE) || 
            (dco.getInstanceDiscoveryMethod() == DataCollectionObject.IDM_INTERNAL_TABLE);

      discoveryData = new LabeledText(dialogArea, SWT.NONE);
      discoveryData.setLabel(getDataLabel(dco.getInstanceDiscoveryMethod()));
      discoveryData.setText(isTableSource ? getTableName(dco.getInstanceDiscoveryData()) : dco.getInstanceDiscoveryData());
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      discoveryData.setLayoutData(gd);
      discoveryData.setEnabled(dco.getInstanceDiscoveryMethod() != DataCollectionObject.IDM_NONE);

      instanceNameColumn = new LabeledText(dialogArea, SWT.NONE);
      instanceNameColumn.setLabel(i18n.tr("Instance name column"));
      instanceNameColumn.setText(isTableSource ? getColumnName(dco.getInstanceDiscoveryData()) : "");
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      instanceNameColumn.setLayoutData(gd);
      instanceNameColumn.setEnabled(isTableSource);

      groupRetention = new Group(dialogArea, SWT.NONE);
      groupRetention.setText("Instance retention");
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
      instanceRetentionMode.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            instanceRetentionTime.setEnabled(instanceRetentionMode.getSelectionIndex() == 1);
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
      gd.minimumHeight = 100;
      final WidgetFactory factory = new WidgetFactory() {
			@Override
			public Control createControl(Composite parent, int style)
			{
				return new ScriptEditor(parent, style, SWT.H_SCROLL | SWT.V_SCROLL,  
				      "Variables:\n\t$1\t\t\tInstance to test;\n\t$2\t\t\tInstance data (value for given OID for SNMP Walk - Values method, secondary value for Script method);\n\t$dci\t\tthis DCI object;\n\t$isCluster\ttrue if DCI is on cluster;\n\t$node\t\tcurrent node object (null if DCI is not on the node);\n\t$object\t\tcurrent object.\n\nReturn value:\n\ttrue/false to accept or reject instance without additional changes or\n\tarray of one, two or three elements to accept instance while modifying it:\n\t\t1st element - new instance name;\n\t\t2nd element - new instance display name;\n\t\t3rd element - object related to this DCI.");
			}
      };
      filterScript = (ScriptEditor)WidgetHelper.createLabeledControl(dialogArea, SWT.BORDER,
            factory, i18n.tr("Instance discovery filter script"), gd);
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
    * Get column name from table definition (in form table:column)
    *
    * @param tableDefinition table definition
    * @return column name or empty string if table definition is not in expected format
    */
   private static String getColumnName(String tableDefinition)
   {
      int i = tableDefinition.lastIndexOf(':');
      return (i != -1) ? tableDefinition.substring(i + 1) : "";
   }

   /**
    * Get table name from table definition (in form table:column)
    *
    * @param tableDefinition table definition
    * @return table name
    */
   private static String getTableName(String tableDefinition)
   {
      int i = tableDefinition.lastIndexOf(':');
      return (i != -1) ? tableDefinition.substring(0, i) : tableDefinition;
   }

	/**
	 * Get label for data field
	 * 
	 * @param method
	 * @return
	 */
	private static String getDataLabel(int method)
	{
	   I18n i18n = LocalizationHelper.getI18n(InstanceDiscovery.class);   
		switch(method)
		{
			case DataCollectionObject.IDM_NONE:
				return i18n.tr("Discovery data");
			case DataCollectionObject.IDM_AGENT_LIST:
				return i18n.tr("List name");
			case DataCollectionObject.IDM_AGENT_TABLE:
         case DataCollectionObject.IDM_INTERNAL_TABLE:
				return i18n.tr("Table name");
			case DataCollectionObject.IDM_SNMP_WALK_VALUES:
			case DataCollectionObject.IDM_SNMP_WALK_OIDS:
				return i18n.tr("Base SNMP OID");
			case DataCollectionObject.IDM_SCRIPT:
			   return i18n.tr("Script name");
         case DataCollectionObject.IDM_WEB_SERVICE:
            return i18n.tr("Web service request");
         case DataCollectionObject.IDM_WINPERF:
            return i18n.tr("Object name");
         case DataCollectionObject.IDM_SMCLP_TARGETS:
         case DataCollectionObject.IDM_SMCLP_PROPERTIES:
            return i18n.tr("Target");
         case DataCollectionObject.IDM_PUSH:
            return i18n.tr("Pattern");
		}
      return "";
	}

   /**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
	protected boolean applyChanges(final boolean isApply)
	{
      int method = discoveryMethod.getSelectionIndex();
      dco.setInstanceDiscoveryMethod(method);
      String data = discoveryData.getText().trim();
      if ((method == DataCollectionObject.IDM_AGENT_TABLE) || (method == DataCollectionObject.IDM_INTERNAL_TABLE))
      {
         // For table sources, data could be in form "table:column"
         String column = instanceNameColumn.getText().trim();
         dco.setInstanceDiscoveryData(column.isEmpty() ? data : data + ":" + column);
      }
      else
      {
         dco.setInstanceDiscoveryData(data);
      }
	   dco.setInstanceDiscoveryFilter(filterScript.getText());
	   if (instanceRetentionMode.getSelectionIndex() == 0)
	      dco.setInstanceRetentionTime(-1);
	   else
	      dco.setInstanceRetentionTime(instanceRetentionTime.getSelection());
		editor.modify();
		return true;
	}

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performDefaults()
    */
	@Override
	protected void performDefaults()
	{
		super.performDefaults();
		discoveryMethod.select(DataCollectionObject.IDM_NONE);
		discoveryData.setLabel(getDataLabel(DataCollectionObject.IDM_NONE));
      discoveryData.setText("");
		discoveryData.setEnabled(false);
      instanceNameColumn.setText("");
      instanceNameColumn.setEnabled(false);
      filterScript.setText("");
		filterScript.setEnabled(false);
		instanceRetentionMode.select(0);
		instanceRetentionTime.setSelection(0);
	}
}
