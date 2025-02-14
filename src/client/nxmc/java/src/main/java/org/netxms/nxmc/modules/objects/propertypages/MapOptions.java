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
package org.netxms.nxmc.modules.objects.propertypages;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.preference.ColorSelector;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Spinner;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.maps.MapObjectDisplayMode;
import org.netxms.client.maps.MapType;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.NetworkMap;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.LabeledCombo;
import org.netxms.nxmc.base.widgets.LabeledSpinner;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.ColorConverter;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Map options property page
 */
public class MapOptions extends ObjectPropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(MapOptions.class);
   private static final int FLAG_MASK = (NetworkMap.MF_SHOW_END_NODES | NetworkMap.MF_SHOW_STATUS_ICON | NetworkMap.MF_SHOW_STATUS_FRAME | NetworkMap.MF_SHOW_STATUS_BKGND |
         NetworkMap.MF_CALCULATE_STATUS | NetworkMap.MF_SHOW_LINK_DIRECTION | NetworkMap.MF_TRANSLUCENT_LABEL_BKGND | NetworkMap.MF_USE_L1_TOPOLOGY | 
         NetworkMap.MF_DONT_UPDATE_LINK_TEXT | NetworkMap.MF_SHOW_TRAFFIC);

   private NetworkMap map;
	private Button checkShowStatusIcon;
	private Button checkShowStatusFrame;
   private Button checkShowStatusBkgnd;
   private Button checkShowLinkDirection;
   private Button checkShowTraffic;
   private Button checkTranslucentLabelBkgnd;
   private Button checkUseL1Topology;
   private Button checkDontUpdateLinkText;
   private Combo objectDisplayMode;
	private Combo routingAlgorithm;
	private Button radioColorDefault;
	private Button radioColorCustom;
	private ColorSelector linkColor;
   private LabeledCombo comboLinkStyle;
   private LabeledSpinner spinerLineWidth;
	private Button checkIncludeEndNodes;
	private Button checkCustomRadius;
	private Spinner topologyRadius;
	private Button checkCalculateStatus;

   /**
    * Create new page.
    *
    * @param object object to edit
    */
   public MapOptions(AbstractObject object)
   {
      super(LocalizationHelper.getI18n(MapOptions.class).tr("Map Options"), object);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "mapOptions";
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#isVisible()
    */
   @Override
   public boolean isVisible()
   {
      return (object instanceof NetworkMap);
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
		Composite dialogArea = new Composite(parent, SWT.NONE);

      map = (NetworkMap)object;

		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		layout.numColumns = 2;
		dialogArea.setLayout(layout);

		/**** object display ****/
		Group objectDisplayGroup = new Group(dialogArea, SWT.NONE);
		objectDisplayGroup.setText(i18n.tr("Object display options"));
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
      objectDisplayMode = WidgetHelper.createLabeledCombo(objectDisplayGroup, SWT.READ_ONLY, i18n.tr("Display objects as"), gd);
      objectDisplayMode.add(i18n.tr("Icons"));
      objectDisplayMode.add(i18n.tr("Small labels"));
      objectDisplayMode.add(i18n.tr("Large labels"));
      objectDisplayMode.add(i18n.tr("Status icons"));
      objectDisplayMode.add(i18n.tr("Floor plan"));
      objectDisplayMode.select(map.getObjectDisplayMode().getValue());

      checkShowStatusIcon = new Button(objectDisplayGroup, SWT.CHECK);
      checkShowStatusIcon.setText(i18n.tr("Show status &icon"));
      checkShowStatusIcon.setSelection((map.getFlags() & NetworkMap.MF_SHOW_STATUS_ICON) != 0);

      checkShowStatusFrame = new Button(objectDisplayGroup, SWT.CHECK);
      checkShowStatusFrame.setText(i18n.tr("Show status &frame"));
      checkShowStatusFrame.setSelection((map.getFlags() & NetworkMap.MF_SHOW_STATUS_FRAME) != 0);

      checkShowStatusBkgnd = new Button(objectDisplayGroup, SWT.CHECK);
      checkShowStatusBkgnd.setText(i18n.tr("Show status &background"));
      checkShowStatusBkgnd.setSelection((map.getFlags() & NetworkMap.MF_SHOW_STATUS_BKGND) != 0);

      checkTranslucentLabelBkgnd = new Button(objectDisplayGroup, SWT.CHECK);
      checkTranslucentLabelBkgnd.setText(i18n.tr("Translucent label background"));
      checkTranslucentLabelBkgnd.setSelection(map.isTranslucentLabelBackground());

		/**** default link appearance ****/
		Group linkGroup = new Group(dialogArea, SWT.NONE);
		linkGroup.setText(i18n.tr("Default link options"));
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
		routingAlgorithm = WidgetHelper.createLabeledCombo(linkGroup, SWT.READ_ONLY, i18n.tr("Routing algorithm"), gd);
		routingAlgorithm.add(i18n.tr("Direct"));
		routingAlgorithm.add(i18n.tr("Manhattan"));
      routingAlgorithm.select(map.getDefaultLinkRouting() - 1);

      final SelectionListener listener = new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				linkColor.setEnabled(radioColorCustom.getSelection());
			}
		};

		radioColorDefault = new Button(linkGroup, SWT.RADIO);
		radioColorDefault.setText(i18n.tr("&Default color"));
      radioColorDefault.setSelection(map.getDefaultLinkColor() < 0);
		radioColorDefault.addSelectionListener(listener);
		gd = new GridData();
		gd.verticalIndent = WidgetHelper.OUTER_SPACING * 2;
		radioColorDefault.setLayoutData(gd);

		radioColorCustom = new Button(linkGroup, SWT.RADIO);
		radioColorCustom.setText(i18n.tr("&Custom color"));
      radioColorCustom.setSelection(map.getDefaultLinkColor() >= 0);
		radioColorCustom.addSelectionListener(listener);

		linkColor = new ColorSelector(linkGroup);
      linkColor.setColorValue(ColorConverter.rgbFromInt(map.getDefaultLinkColor()));
      linkColor.setEnabled(map.getDefaultLinkColor() >= 0);
		gd = new GridData();
		gd.horizontalIndent = 20;
		linkColor.getButton().setLayoutData(gd);

      comboLinkStyle = new LabeledCombo(linkGroup, SWT.NONE);
      comboLinkStyle.setLabel(i18n.tr("Line style"));
      comboLinkStyle.add(i18n.tr("Solid"));
      comboLinkStyle.add(i18n.tr("Dash"));
      comboLinkStyle.add(i18n.tr("Dot"));
      comboLinkStyle.add(i18n.tr("Dashdot"));
      comboLinkStyle.add(i18n.tr("Dashdotdot"));
      comboLinkStyle.select(map.getDefaultLinkStyle() - 1);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      comboLinkStyle.setLayoutData(gd);      

      spinerLineWidth = new LabeledSpinner(linkGroup, SWT.NONE);
      spinerLineWidth.setLabel(i18n.tr("Line width (0 for client default)"));
      spinerLineWidth.setRange(0, 100);
      spinerLineWidth.setSelection(map.getDefaultLinkWidth());    

		/**** topology options ****/
      if (map.getMapType() != MapType.CUSTOM)
      {
			Group topoGroup = new Group(dialogArea, SWT.NONE);
			topoGroup.setText(i18n.tr("Topology options"));
	      gd = new GridData();
	      gd.grabExcessHorizontalSpace = true;
	      gd.horizontalAlignment = SWT.FILL;
	      topoGroup.setLayoutData(gd);
	      layout = new GridLayout();
	      topoGroup.setLayout(layout);

         checkIncludeEndNodes = new Button(topoGroup, SWT.CHECK);
         checkIncludeEndNodes.setText(i18n.tr("Include &end nodes"));
         checkIncludeEndNodes.setSelection((map.getFlags() & NetworkMap.MF_SHOW_END_NODES) != 0);

         checkUseL1Topology = new Button(topoGroup, SWT.CHECK);
         checkUseL1Topology.setText(i18n.tr("Use &physical link information"));
         checkUseL1Topology.setSelection((map.getFlags() & NetworkMap.MF_USE_L1_TOPOLOGY) != 0);
         
         checkCustomRadius = new Button(topoGroup, SWT.CHECK);
         checkCustomRadius.setText(i18n.tr("Custom discovery &radius"));
         checkCustomRadius.setSelection(map.getDiscoveryRadius() > 0);
         checkCustomRadius.addSelectionListener(new SelectionAdapter() {
				@Override
				public void widgetSelected(SelectionEvent e)
				{
			      topologyRadius.setEnabled(checkCustomRadius.getSelection());
				}
			});

	      topologyRadius = WidgetHelper.createLabeledSpinner(topoGroup, SWT.BORDER, i18n.tr("Topology discovery radius"), 1, 255, WidgetHelper.DEFAULT_LAYOUT_DATA);
         topologyRadius.setSelection(map.getDiscoveryRadius());
         topologyRadius.setEnabled(map.getDiscoveryRadius() > 0);
      }      
      
      /**** Link options ****/
      Group linkDisplayGroup = new Group(dialogArea, SWT.NONE);
      linkDisplayGroup.setText(i18n.tr("Link options"));
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      linkDisplayGroup.setLayoutData(gd);
      layout = new GridLayout();
      linkDisplayGroup.setLayout(layout);

      checkShowLinkDirection = new Button(linkDisplayGroup, SWT.CHECK);
      checkShowLinkDirection.setText("Show link direction");
      checkShowLinkDirection.setSelection((map.getFlags() & NetworkMap.MF_SHOW_LINK_DIRECTION) != 0);

      checkShowTraffic = new Button(linkDisplayGroup, SWT.CHECK);
      checkShowTraffic.setText("Display traffic data");
      checkShowTraffic.setSelection((map.getFlags() & NetworkMap.MF_SHOW_TRAFFIC) != 0);

      checkDontUpdateLinkText = new Button(linkDisplayGroup, SWT.CHECK);
      checkDontUpdateLinkText.setText(i18n.tr("Disable link &texts update"));
      checkDontUpdateLinkText.setSelection(map.isDontUpdateLinkText());

		/**** advanced options ****/
		Group advGroup = new Group(dialogArea, SWT.NONE);
		advGroup.setText(i18n.tr("Advanced options"));
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.horizontalSpan = 2;
      advGroup.setLayoutData(gd);
      layout = new GridLayout();
      advGroup.setLayout(layout);

      checkCalculateStatus = new Button(advGroup, SWT.CHECK);
      checkCalculateStatus.setText(i18n.tr("&Calculate map status based on contained object status"));
      checkCalculateStatus.setSelection((map.getFlags() & NetworkMap.MF_CALCULATE_STATUS) != 0);

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
		if (checkShowStatusBkgnd.getSelection())
			flags |= NetworkMap.MF_SHOW_STATUS_BKGND;
		if (checkCalculateStatus.getSelection())
			flags |= NetworkMap.MF_CALCULATE_STATUS;
		if (checkShowLinkDirection.getSelection())
		   flags |= NetworkMap.MF_SHOW_LINK_DIRECTION;
      if (checkTranslucentLabelBkgnd.getSelection())
         flags |= NetworkMap.MF_TRANSLUCENT_LABEL_BKGND;
      if ((checkUseL1Topology != null) && checkUseL1Topology.getSelection())
         flags |= NetworkMap.MF_USE_L1_TOPOLOGY;
      if ((checkDontUpdateLinkText != null) && checkDontUpdateLinkText.getSelection())
         flags |= NetworkMap.MF_DONT_UPDATE_LINK_TEXT;
      if (checkShowTraffic.getSelection())
         flags |= NetworkMap.MF_SHOW_TRAFFIC;
      md.setObjectFlags(flags, FLAG_MASK);

		md.setMapObjectDisplayMode(MapObjectDisplayMode.getByValue(objectDisplayMode.getSelectionIndex()));
		md.setConnectionRouting(routingAlgorithm.getSelectionIndex() + 1);
      md.setNetworkMapLinkWidth(spinerLineWidth.getSelection());
      md.setNetworkMapLinkStyle(comboLinkStyle.getSelectionIndex() + 1);

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

      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Updating map options for map object {0}", object.getObjectName()), null, messageArea) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
			{
				session.modifyObject(md);
			}

			@Override
			protected String getErrorMessage()
			{
            return String.format(i18n.tr("Cannot modify options for map object %s"), object.getObjectName());
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
}
