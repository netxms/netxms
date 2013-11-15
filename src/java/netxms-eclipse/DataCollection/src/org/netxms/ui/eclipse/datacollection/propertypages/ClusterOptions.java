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
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.ClusterResource;
import org.netxms.ui.eclipse.datacollection.Messages;
import org.netxms.ui.eclipse.datacollection.api.DataCollectionObjectEditor;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * "Cluster" property page for data collection object
 */
public class ClusterOptions extends PropertyPage
{
	private DataCollectionObjectEditor editor;
	private AbstractObject owner;
	private Cluster cluster = null;
	private Combo clusterResource;
	private Map<Integer, Long> clusterResourceMap;
	private Button checkAggregate;
	private Combo aggregationFunction;

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		editor = (DataCollectionObjectEditor)getElement().getAdapter(DataCollectionObjectEditor.class);

		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
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
		
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
		layout.horizontalSpacing = WidgetHelper.DIALOG_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
      dialogArea.setLayout(layout);
      
      clusterResource = WidgetHelper.createLabeledCombo(dialogArea, SWT.BORDER | SWT.READ_ONLY, Messages.get().General_ClRes, WidgetHelper.DEFAULT_LAYOUT_DATA);
      if (cluster != null)
      {
      	clusterResourceMap = new HashMap<Integer, Long>();
      	clusterResourceMap.put(0, 0L);
      	
	      clusterResource.add(Messages.get().General_None);
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
	      clusterResource.add(Messages.get().General_None);
	      clusterResource.select(0);
	      clusterResource.setEnabled(false);
      }
      
   	Group aggregationGroup = new Group(dialogArea, SWT.NONE);
   	aggregationGroup.setText(Messages.get().ClusterOptions_DataAggregation);
   	aggregationGroup.setLayoutData(new GridData(SWT.FILL, SWT.TOP, true, false));
   	
   	layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
		layout.horizontalSpacing = WidgetHelper.DIALOG_SPACING;
		aggregationGroup.setLayout(layout);
   	
   	checkAggregate = new Button(aggregationGroup, SWT.CHECK);
   	checkAggregate.setText(Messages.get().ClusterOptions_AggregateFromNodes);
   	checkAggregate.setSelection(editor.getObject().isAggregateOnCluster());
      	
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
      	
      	aggregationFunction = WidgetHelper.createLabeledCombo(aggregationGroup, SWT.BORDER | SWT.READ_ONLY, Messages.get().ClusterOptions_AggrFunction, WidgetHelper.DEFAULT_LAYOUT_DATA);
      	aggregationFunction.add(Messages.get().ClusterOptions_Total);
      	aggregationFunction.add(Messages.get().ClusterOptions_Average);
      	aggregationFunction.add(Messages.get().ClusterOptions_Min);
      	aggregationFunction.add(Messages.get().ClusterOptions_Max);
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
	protected boolean applyChanges(final boolean isApply)
	{
		if (cluster != null)
		{
			editor.getObject().setResourceId(clusterResourceMap.get(clusterResource.getSelectionIndex()));
		}
		editor.getObject().setAggregateOnCluster(checkAggregate.getSelection());
		if (editor.getObject() instanceof DataCollectionItem)
		{
			editor.getObjectAsItem().setAggregationFunction(aggregationFunction.getSelectionIndex());
		}
		editor.modify();
		return true;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performOk()
	 */
	@Override
	public boolean performOk()
	{
		return applyChanges(false);
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
		clusterResource.select(0);
		checkAggregate.setSelection(false);
		aggregationFunction.select(0);
		aggregationFunction.setEnabled(false);
	}
}
