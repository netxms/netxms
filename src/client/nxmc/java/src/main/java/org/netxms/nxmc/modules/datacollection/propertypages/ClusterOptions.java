/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Raden Solutions
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

import java.util.HashMap;
import java.util.Map;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.ClusterResource;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.DataCollectionObjectEditor;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Cluster" property page for data collection object
 */
public class ClusterOptions extends AbstractDCIPropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(ClusterOptions.class);
   
   private AbstractObject owner;
	private Cluster cluster = null;
	private Combo clusterResource;
	private Map<Integer, Long> clusterResourceMap;
	private Button checkAggregate;
   private Button checkAggregateWithErrors;
	private Button checkRunScript;
	private Combo aggregationFunction;

	/**
	 * Constructor
	 * 
	 * @param editor
	 */
   public ClusterOptions(DataCollectionObjectEditor editor)
   {
      super(LocalizationHelper.getI18n(ClusterOptions.class).tr("Cluster Options"), editor);
   }

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{      
      Composite dialogArea = (Composite)super.createContents(parent);

		final NXCSession session = Registry.getSession();
		owner = session.findObjectById(editor.getObject().getNodeId());		
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
		layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
		layout.horizontalSpacing = WidgetHelper.DIALOG_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
      dialogArea.setLayout(layout);

      clusterResource = WidgetHelper.createLabeledCombo(dialogArea, SWT.BORDER | SWT.READ_ONLY, i18n.tr("Associate with cluster resource"), WidgetHelper.DEFAULT_LAYOUT_DATA);
      if (cluster != null)
      {
      	clusterResourceMap = new HashMap<Integer, Long>();
      	clusterResourceMap.put(0, 0L);
      	
	      clusterResource.add(i18n.tr("<none>"));
	      if (editor.getObject().getResourceId() == 0)
	      	clusterResource.select(0);
	      
	      int index = 1;
	      for (ClusterResource r : cluster.getResources())
	      {
	      	clusterResource.add(r.getName());
	      	clusterResourceMap.put(index, r.getId());
		      if (editor.getObject().getResourceId() == r.getId())
		      	clusterResource.select(index);
	      	index++;
	      }
      }
      else
      {
	      clusterResource.add(i18n.tr("<none>"));
	      clusterResource.select(0);
	      clusterResource.setEnabled(false);
      }
      
   	Group aggregationGroup = new Group(dialogArea, SWT.NONE);
   	aggregationGroup.setText(i18n.tr("Data aggregation"));
   	aggregationGroup.setLayoutData(new GridData(SWT.FILL, SWT.TOP, true, false));
   	
   	layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
		layout.horizontalSpacing = WidgetHelper.DIALOG_SPACING;
		aggregationGroup.setLayout(layout);
   	
   	checkAggregate = new Button(aggregationGroup, SWT.CHECK);
   	checkAggregate.setText(i18n.tr("Aggregate values from cluster nodes"));
   	checkAggregate.setSelection(editor.getObject().isAggregateOnCluster());

      checkAggregateWithErrors = new Button(aggregationGroup, SWT.CHECK);
      checkAggregateWithErrors.setText("Use last known value for aggregation in case of data collection error");
      checkAggregateWithErrors.setSelection(editor.getObject().isAggregateWithErrors());
         
      checkRunScript = new Button(aggregationGroup, SWT.CHECK);
      checkRunScript.setText(i18n.tr("Run transformation script on aggregated data"));
      checkRunScript.setSelection(editor.getObject().isTransformAggregated());
         
      if (editor.getObject() instanceof DataCollectionItem)
      {
      	checkAggregate.addSelectionListener(new SelectionListener() {
   			@Override
   			public void widgetSelected(SelectionEvent e)
   			{
   				aggregationFunction.setEnabled(checkAggregate.getSelection());
   			}
   			
   			@Override
   			public void widgetDefaultSelected(SelectionEvent e)
   			{
   				widgetSelected(e);
   			}
   		});
      	
      	aggregationFunction = WidgetHelper.createLabeledCombo(aggregationGroup, SWT.BORDER | SWT.READ_ONLY, i18n.tr("Aggregation function"), WidgetHelper.DEFAULT_LAYOUT_DATA);
      	aggregationFunction.add(i18n.tr("Total"));
      	aggregationFunction.add(i18n.tr("Average"));
      	aggregationFunction.add(i18n.tr("Min"));
      	aggregationFunction.add(i18n.tr("Max"));
      	aggregationFunction.select(editor.getObjectAsItem().getAggregationFunction());
      	aggregationFunction.setEnabled(editor.getObjectAsItem().isAggregateOnCluster());
      }

      return dialogArea;
	}

	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
   @Override
	protected boolean applyChanges(final boolean isApply)
	{
		if (cluster != null)
		{
			editor.getObject().setResourceId(clusterResourceMap.get(clusterResource.getSelectionIndex()));
		}
		editor.getObject().setAggregateOnCluster(checkAggregate.getSelection());
      editor.getObject().setAggregateWithErrors(checkAggregateWithErrors.getSelection());
      editor.getObject().setTransformAggregated(checkRunScript.getSelection());
		if (editor.getObject() instanceof DataCollectionItem)
		{
			editor.getObjectAsItem().setAggregationFunction(aggregationFunction.getSelectionIndex());
		}
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
		clusterResource.select(0);
		checkAggregate.setSelection(false);
		aggregationFunction.select(0);
		aggregationFunction.setEnabled(false);
	}
}
