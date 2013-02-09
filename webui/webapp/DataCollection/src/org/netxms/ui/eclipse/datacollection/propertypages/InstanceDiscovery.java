/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.ui.eclipse.datacollection.Activator;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.nxsl.widgets.ScriptEditor;
import org.netxms.ui.eclipse.tools.WidgetFactory;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * "Instance Discovery" property page for DCI
 */
public class InstanceDiscovery extends PropertyPage
{
	private static final long serialVersionUID = 1L;

	private static final String[] DCI_FUNCTIONS = { "FindDCIByName", "FindDCIByDescription", "GetDCIObject", "GetDCIValue", "GetDCIValueByDescription", "GetDCIValueByName" }; //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$ //$NON-NLS-4$ //$NON-NLS-5$ //$NON-NLS-6$
	private static final String[] DCI_VARIABLES = { "$dci", "$node" }; //$NON-NLS-1$
	
	private DataCollectionItem dci;
	private Combo discoveryMethod;
	private LabeledText discoveryData;
	private ScriptEditor filterScript;
	
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
      dialogArea.setLayout(layout);

      discoveryMethod = WidgetHelper.createLabeledCombo(dialogArea, SWT.BORDER | SWT.READ_ONLY, "Instance discovery method",
                                                         WidgetHelper.DEFAULT_LAYOUT_DATA);
      discoveryMethod.add("None");
      discoveryMethod.add("Agent List");
      discoveryMethod.add("Agent Table");
      discoveryMethod.add("SNMP Walk - Values");
      discoveryMethod.add("SNMP Walk - OIDs");
      discoveryMethod.select(dci.getInstanceDiscoveryMethod());
      discoveryMethod.addSelectionListener(new SelectionListener() {
			private static final long serialVersionUID = 1L;

			@Override
			public void widgetSelected(SelectionEvent e)
			{
				int method = discoveryMethod.getSelectionIndex();
		      discoveryData.setLabel(getDataLabel(method));
		      discoveryData.setEnabled(method != DataCollectionItem.IDM_NONE);
		      filterScript.setEnabled(method != DataCollectionItem.IDM_NONE);
			}
			
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		});
      
      discoveryData = new LabeledText(dialogArea, SWT.NONE);
      discoveryData.setLabel(getDataLabel(dci.getInstanceDiscoveryMethod()));
      discoveryData.setText(dci.getInstanceDiscoveryData());
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      discoveryData.setLayoutData(gd);
      discoveryData.setEnabled(dci.getInstanceDiscoveryMethod() != DataCollectionItem.IDM_NONE);
     
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
				return new ScriptEditor(parent, style,  SWT.H_SCROLL | SWT.V_SCROLL);
			}
      };
      filterScript = (ScriptEditor)WidgetHelper.createLabeledControl(dialogArea, SWT.BORDER,
                                                                             factory, "Instance discovery filter script", gd);
      filterScript.addFunctions(Arrays.asList(DCI_FUNCTIONS));
      filterScript.addVariables(Arrays.asList(DCI_VARIABLES));
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      filterScript.setLayoutData(gd);
      filterScript.setText(dci.getInstanceDiscoveryFilter());
      filterScript.setEnabled(dci.getInstanceDiscoveryMethod() != DataCollectionItem.IDM_NONE);
      
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
			case DataCollectionItem.IDM_NONE:
				return "Discovery data";
			case DataCollectionItem.IDM_AGENT_LIST:
				return "List name";
			case DataCollectionItem.IDM_AGENT_TABLE:
				return "List name";
			case DataCollectionItem.IDM_SNMP_WALK_VALUES:
			case DataCollectionItem.IDM_SNMP_WALK_OIDS:
				return "Base SNMP OID";
		}
		return "";
	}
	
	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
	protected void applyChanges(final boolean isApply)
	{
		if (isApply)
			setValid(false);
		
		dci.setInstanceDiscoveryMethod(discoveryMethod.getSelectionIndex());
		dci.setInstanceDiscoveryData(discoveryData.getText());
		dci.setInstanceDiscoveryFilter(filterScript.getText());
		
		new ConsoleJob("Updating instance discovery configuration for DCI " + dci.getId(), null, Activator.PLUGIN_ID, null) {
			@Override
			protected String getErrorMessage()
			{
				return "Cannot update instance discovery configuration";
			}

			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				dci.getOwner().modifyObject(dci);
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						((TableViewer)dci.getOwner().getUserData()).update(dci, null);
					}
				});
			}

			/* (non-Javadoc)
			 * @see org.netxms.ui.eclipse.jobs.ConsoleJob#jobFinalize()
			 */
			@Override
			protected void jobFinalize()
			{
				if (isApply)
				{
					runInUIThread(new Runnable() {
						@Override
						public void run()
						{
							InstanceDiscovery.this.setValid(true);
						}
					});
				}
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
		discoveryMethod.select(DataCollectionItem.IDM_NONE);
		discoveryData.setLabel(getDataLabel(DataCollectionItem.IDM_NONE));
		discoveryData.setText(""); //$NON-NLS-1$
		discoveryData.setEnabled(false);
		filterScript.setText(""); //$NON-NLS-1$
		filterScript.setEnabled(false);
	}
}
