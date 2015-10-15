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
package org.netxms.ui.eclipse.objectmanager.propertypages;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.base.NXCommon;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.Rack;
import org.netxms.ui.eclipse.imagelibrary.widgets.ImageSelector;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectbrowser.widgets.ObjectSelector;
import org.netxms.ui.eclipse.objectmanager.Activator;
import org.netxms.ui.eclipse.objectmanager.Messages;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledSpinner;

/**
 * "Comments" property page for NetXMS object
 *
 */
public class RackPlacement extends PropertyPage
{
	private AbstractNode node;
	private ObjectSelector rackSelector;
	private ImageSelector rackImageSelector;
	private LabeledSpinner rackHeight;
	private LabeledSpinner rackPosition;
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		node = (AbstractNode)getElement().getAdapter(AbstractNode.class);

		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		layout.numColumns = 2;
      dialogArea.setLayout(layout);

      rackSelector = new ObjectSelector(dialogArea, SWT.NONE, true);
      rackSelector.setLabel("Rack");
      rackSelector.setObjectClass(Rack.class);
      rackSelector.setObjectId(node.getRackId());
		GridData gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		gd.horizontalSpan = 2;
		rackSelector.setLayoutData(gd);
		
		rackImageSelector = new ImageSelector(dialogArea, SWT.NONE);
		rackImageSelector.setLabel("Rack image");
		rackImageSelector.setImageGuid(node.getRackImage(), false);
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.horizontalSpan = 2;
      rackImageSelector.setLayoutData(gd);
      
      rackPosition = new LabeledSpinner(dialogArea, SWT.NONE);
      rackPosition.setLabel("Position");
      rackPosition.setRange(1, 50);
      rackPosition.setSelection(node.getRackPosition());
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      rackPosition.setLayoutData(gd);
		
      rackHeight = new LabeledSpinner(dialogArea, SWT.NONE);
      rackHeight.setLabel("Height");
      rackHeight.setRange(1, 50);
      rackHeight.setSelection(node.getRackHeight());
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      rackHeight.setLayoutData(gd);
      
		return dialogArea;
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
		
		final NXCObjectModificationData md = new NXCObjectModificationData(node.getObjectId());
		md.setRackPlacement(rackSelector.getObjectId(), rackImageSelector.getImageGuid(), (short)rackPosition.getSelection(), (short)rackHeight.getSelection());
		
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		new ConsoleJob(String.format("Updating rack placement for node %s", node.getObjectName()), null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
			   session.modifyObject(md);
			}

			@Override
			protected String getErrorMessage()
			{
				return Messages.get().Comments_JobError;
			}

			@Override
			protected void jobFinalize()
			{
				if (isApply)
				{
					runInUIThread(new Runnable() {
						@Override
						public void run()
						{
							RackPlacement.this.setValid(true);
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
		rackSelector.setObjectId(0);
		rackImageSelector.setImageGuid(NXCommon.EMPTY_GUID, true);
		rackPosition.setSelection(1);
		rackHeight.setSelection(1);
	}
}
