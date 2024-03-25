/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.networkmaps.propertypages;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.preference.ColorSelector;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Spinner;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.maps.MapObjectDisplayMode;
import org.netxms.client.objects.NetworkMap;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.networkmaps.Activator;
import org.netxms.ui.eclipse.networkmaps.Messages;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.ColorConverter;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Map options property page
 */
public class MapOptions extends PropertyPage
{
   private static final int FLAG_MASK = NetworkMap.MF_SHOW_END_NODES | NetworkMap.MF_SHOW_STATUS_ICON | NetworkMap.MF_SHOW_STATUS_FRAME | NetworkMap.MF_SHOW_STATUS_BKGND |
         NetworkMap.MF_CALCULATE_STATUS | NetworkMap.MF_SHOW_LINK_DIRECTION | NetworkMap.MF_TRANSLUCENT_LABEL_BKGND | NetworkMap.MF_USE_L1_TOPOLOGY;

   private NetworkMap object;
	private Button checkShowStatusIcon;
	private Button checkShowStatusFrame;
   private Button checkShowStatusBackground;
   private Button checkShowLinkDirection;
   private Button checkTranslucentLabelBackground;
   private Button checkUseL1Topology;
   private Button checkDontUpdateLinkText;
   private Combo objectDisplayMode;
	private Combo routingAlgorithm;
	private Button radioColorDefault;
	private Button radioColorCustom;
	private ColorSelector linkColor;
	private Button checkIncludeEndNodes;
	private Button checkCustomRadius;
	private Spinner topologyRadius;
	private Button checkCalculateStatus;

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
		Composite dialogArea = new Composite(parent, SWT.NONE);

		object = (NetworkMap)getElement().getAdapter(NetworkMap.class);

		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		layout.numColumns = 2;
		dialogArea.setLayout(layout);

		/**** object display ****/
		Group objectDisplayGroup = new Group(dialogArea, SWT.NONE);
		objectDisplayGroup.setText(Messages.get().MapOptions_DefaultDispOptions);
      GridData gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      objectDisplayGroup.setLayoutData(gd);
      layout = new GridLayout();
      objectDisplayGroup.setLayout(layout);

      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      objectDisplayMode = WidgetHelper.createLabeledCombo(objectDisplayGroup, SWT.READ_ONLY, Messages.get().MapOptions_DisplayObjectsAs, gd);
      objectDisplayMode.add(Messages.get().MapOptions_Icons);
      objectDisplayMode.add(Messages.get().MapOptions_SmallLabels);
      objectDisplayMode.add(Messages.get().MapOptions_LargeLabels);
      objectDisplayMode.add(Messages.get().MapOptions_StatusIcons);
      objectDisplayMode.add("Floor plan");
      objectDisplayMode.select(object.getObjectDisplayMode().getValue());

      checkShowStatusIcon = new Button(objectDisplayGroup, SWT.CHECK);
      checkShowStatusIcon.setText(Messages.get().MapOptions_ShowStatusIcon);
      checkShowStatusIcon.setSelection(object.isShowStatusIcon());

      checkShowStatusFrame = new Button(objectDisplayGroup, SWT.CHECK);
      checkShowStatusFrame.setText(Messages.get().MapOptions_ShowStatusFrame);
      checkShowStatusFrame.setSelection(object.isShowStatusFrame());

      checkShowStatusBackground = new Button(objectDisplayGroup, SWT.CHECK);
      checkShowStatusBackground.setText(Messages.get().MapOptions_ShowStatusBkgnd);
      checkShowStatusBackground.setSelection(object.isShowStatusBackground());

      checkShowLinkDirection = new Button(objectDisplayGroup, SWT.CHECK);
      checkShowLinkDirection.setText("Show link direction");
      checkShowLinkDirection.setSelection((object.getFlags() & NetworkMap.MF_SHOW_LINK_DIRECTION) != 0);

      checkTranslucentLabelBackground = new Button(objectDisplayGroup, SWT.CHECK);
      checkTranslucentLabelBackground.setText(Messages.get().MapOptions_TranslucentLabelBkgnd);
      checkTranslucentLabelBackground.setSelection(object.isTranslucentLabelBackground());

		/**** default link appearance ****/
		Group linkGroup = new Group(dialogArea, SWT.NONE);
		linkGroup.setText(Messages.get().MapOptions_DefaultConnOptions);
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      linkGroup.setLayoutData(gd);
      layout = new GridLayout();
      linkGroup.setLayout(layout);

      gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		routingAlgorithm = WidgetHelper.createLabeledCombo(linkGroup, SWT.READ_ONLY, Messages.get().MapOptions_RoutingAlg, gd);
		routingAlgorithm.add(Messages.get().MapOptions_Direct);
		routingAlgorithm.add(Messages.get().MapOptions_Manhattan);
		routingAlgorithm.select(object.getDefaultLinkRouting() - 1);

      final SelectionAdapter listener = new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				linkColor.setEnabled(radioColorCustom.getSelection());
			}
		};

		radioColorDefault = new Button(linkGroup, SWT.RADIO);
		radioColorDefault.setText(Messages.get().MapOptions_DefColor);
		radioColorDefault.setSelection(object.getDefaultLinkColor() < 0);
		radioColorDefault.addSelectionListener(listener);
		gd = new GridData();
		gd.verticalIndent = WidgetHelper.OUTER_SPACING * 2;
		radioColorDefault.setLayoutData(gd);

		radioColorCustom = new Button(linkGroup, SWT.RADIO);
		radioColorCustom.setText(Messages.get().MapOptions_CustColor);
		radioColorCustom.setSelection(object.getDefaultLinkColor() >= 0);
		radioColorCustom.addSelectionListener(listener);

		linkColor = new ColorSelector(linkGroup);
		linkColor.setColorValue(ColorConverter.rgbFromInt(object.getDefaultLinkColor()));
		linkColor.setEnabled(object.getDefaultLinkColor() >= 0);
		gd = new GridData();
		gd.horizontalIndent = 20;
		linkColor.getButton().setLayoutData(gd);

		/**** topology options ****/
      if (object.getMapType() != NetworkMap.TYPE_CUSTOM)
      {
			Group topoGroup = new Group(dialogArea, SWT.NONE);
			topoGroup.setText(Messages.get().MapOptions_TopologyOptions);
	      gd = new GridData();
	      gd.grabExcessHorizontalSpace = true;
	      gd.horizontalAlignment = SWT.FILL;
	      gd.horizontalSpan = 2;
	      topoGroup.setLayoutData(gd);
	      layout = new GridLayout();
	      topoGroup.setLayout(layout);

         checkIncludeEndNodes = new Button(topoGroup, SWT.CHECK);
         checkIncludeEndNodes.setText(Messages.get().MapOptions_IncludeEndNodes);
         checkIncludeEndNodes.setSelection(object.isShowEndNodes());

         checkUseL1Topology = new Button(topoGroup, SWT.CHECK);
         checkUseL1Topology.setText("Use &physical link information");
         checkUseL1Topology.setSelection((object.getFlags() & NetworkMap.MF_USE_L1_TOPOLOGY) != 0);

         checkDontUpdateLinkText = new Button(topoGroup, SWT.CHECK);
         checkDontUpdateLinkText.setText("Disable link &texts update");
         checkDontUpdateLinkText.setSelection(object.isDontUpdateLinkText());

         checkCustomRadius = new Button(topoGroup, SWT.CHECK);
         checkCustomRadius.setText(Messages.get().MapOptions_CustomDiscoRadius);
         checkCustomRadius.setSelection(object.getDiscoveryRadius() > 0);
         checkCustomRadius.addSelectionListener(new SelectionAdapter() {
				@Override
				public void widgetSelected(SelectionEvent e)
				{
			      topologyRadius.setEnabled(checkCustomRadius.getSelection());
				}
			});

	      topologyRadius = WidgetHelper.createLabeledSpinner(topoGroup, SWT.BORDER, Messages.get().MapOptions_TopoDiscoRadius, 1, 255, WidgetHelper.DEFAULT_LAYOUT_DATA);
	      topologyRadius.setSelection(object.getDiscoveryRadius());
	      topologyRadius.setEnabled(object.getDiscoveryRadius() > 0);
      }      

		/**** advanced options ****/
		Group advGroup = new Group(dialogArea, SWT.NONE);
		advGroup.setText(Messages.get().MapOptions_AdvOptions);
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.horizontalSpan = 2;
      advGroup.setLayoutData(gd);
      layout = new GridLayout();
      advGroup.setLayout(layout);

      checkCalculateStatus = new Button(advGroup, SWT.CHECK);
      checkCalculateStatus.setText(Messages.get().MapOptions_CalcStatusFromObjects);
      checkCalculateStatus.setSelection((object.getFlags() & NetworkMap.MF_CALCULATE_STATUS) != 0);

		return dialogArea;
	}

	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
	protected boolean applyChanges(final boolean isApply)
	{
		final NXCObjectModificationData md = new NXCObjectModificationData(object.getObjectId());
		
		int flags = 0;

		if ((checkIncludeEndNodes != null) && checkIncludeEndNodes.getSelection())
			flags |= NetworkMap.MF_SHOW_END_NODES;
		if (checkShowStatusIcon.getSelection())
			flags |= NetworkMap.MF_SHOW_STATUS_ICON;
		if (checkShowStatusFrame.getSelection())
			flags |= NetworkMap.MF_SHOW_STATUS_FRAME;
		if (checkShowStatusBackground.getSelection())
			flags |= NetworkMap.MF_SHOW_STATUS_BKGND;
		if (checkCalculateStatus.getSelection())
			flags |= NetworkMap.MF_CALCULATE_STATUS;
		if (checkShowLinkDirection.getSelection())
		   flags |= NetworkMap.MF_SHOW_LINK_DIRECTION;
      if (checkTranslucentLabelBackground.getSelection())
         flags |= NetworkMap.MF_TRANSLUCENT_LABEL_BKGND;
      if ((checkUseL1Topology != null) && checkUseL1Topology.getSelection())
         flags |= NetworkMap.MF_USE_L1_TOPOLOGY;
      if ((checkDontUpdateLinkText != null) && checkDontUpdateLinkText.getSelection())
         flags |= NetworkMap.MF_DONT_UPDATE_LINK_TEXT;
      md.setObjectFlags(flags, FLAG_MASK);

		md.setMapObjectDisplayMode(MapObjectDisplayMode.getByValue(objectDisplayMode.getSelectionIndex()));
		md.setConnectionRouting(routingAlgorithm.getSelectionIndex() + 1);

		if (radioColorCustom.getSelection())
		{
			md.setLinkColor(ColorConverter.rgbToInt(linkColor.getColorValue()));
		}
		else
		{
			md.setLinkColor(-1);
		}

		if (checkCustomRadius != null)
		{
			if (checkCustomRadius.getSelection())
				md.setDiscoveryRadius(topologyRadius.getSelection());
			else
            md.setDiscoveryRadius(0);
		}

		if (isApply)
			setValid(false);

		final NXCSession session = ConsoleSharedData.getSession();
		new ConsoleJob(Messages.get().MapOptions_JobTitle + object.getObjectName(), null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				session.modifyObject(md);
			}

			@Override
			protected String getErrorMessage()
			{
				return Messages.get().MapOptions_JobError + object.getObjectName();
			}

			@Override
			protected void jobFinalize()
			{
				if (isApply)
               runInUIThread(() -> MapOptions.this.setValid(true));
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

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performApply()
    */
	@Override
	protected void performApply()
	{
		applyChanges(true);
	}
}
