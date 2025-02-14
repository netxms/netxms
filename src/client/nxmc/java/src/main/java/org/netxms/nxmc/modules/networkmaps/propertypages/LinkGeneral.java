/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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
package org.netxms.nxmc.modules.networkmaps.propertypages;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Scale;
import org.eclipse.swt.widgets.Spinner;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.Interface;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.widgets.LabeledCombo;
import org.netxms.nxmc.base.widgets.LabeledSpinner;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.networkmaps.views.helpers.LinkEditor;
import org.netxms.nxmc.modules.objects.widgets.ObjectSelector;
import org.xnap.commons.i18n.I18n;

/**
 * "General" property page for map link
 */
public class LinkGeneral extends LinkPropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(LinkGeneral.class);

	private LabeledText name;
	private LabeledText connector1;
	private LabeledText connector2;
   private ObjectSelector interface1;
   private ObjectSelector interface2;
   private LabeledCombo routingAlgorithm;
   private LabeledCombo comboLinkStyle;
   private LabeledSpinner spinerLineWidth;
	private Spinner spinnerLabelPositon;
	private Scale scaleLabelPositon;
   private Button checkExcludeFromAutomaticUpdate;

   /**
    * Create new page.
    *
    * @param linkEditor link to edit
    */
   public LinkGeneral(LinkEditor linkEditor)
   {
      super(linkEditor, LocalizationHelper.getI18n(LinkGeneral.class).tr("General"));
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
      Composite dialogArea = (Composite)super.createContents(parent);

      name = new LabeledText(dialogArea, SWT.NONE);
      name.setLabel(i18n.tr("Label"));
      name.setText(linkEditor.getName());
      GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 3;
		name.setLayoutData(gd);

      final Group labelPositionGroup = new Group(dialogArea, SWT.NONE);
      labelPositionGroup.setText(i18n.tr("Label position"));
      labelPositionGroup.setLayout(new GridLayout(2, false));
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 3;
      labelPositionGroup.setLayoutData(gd);

      scaleLabelPositon = new Scale(labelPositionGroup, SWT.HORIZONTAL);
      scaleLabelPositon.setMinimum(0);
      scaleLabelPositon.setMaximum(100);
      scaleLabelPositon.setSelection(linkEditor.getLabelPosition());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      scaleLabelPositon.setLayoutData(gd);
      scaleLabelPositon.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            spinnerLabelPositon.setSelection(scaleLabelPositon.getSelection());
         }
      });

      spinnerLabelPositon = new Spinner(labelPositionGroup, SWT.NONE);
      spinnerLabelPositon.setMinimum(0);
      spinnerLabelPositon.setMaximum(100);
      spinnerLabelPositon.setSelection(linkEditor.getLabelPosition());
      spinnerLabelPositon.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            scaleLabelPositon.setSelection(spinnerLabelPositon.getSelection());
         }
      });
      
      NXCSession session = Registry.getSession();

      Group connectorGroup1 = new Group(dialogArea, SWT.BORDER);
      connectorGroup1.setText(i18n.tr("Connector 1 \u2013 {0}", session.getObjectNameWithAlias(linkEditor.getElement1())));
      connectorGroup1.setLayout(new GridLayout());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 3;
      connectorGroup1.setLayoutData(gd);

      connector1 = new LabeledText(connectorGroup1, SWT.NONE);
      connector1.setLabel(i18n.tr("Name (leave empty to use interface name when interface is set)"));
      connector1.setText(linkEditor.getConnectorName1());
      connector1.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      interface1 = new ObjectSelector(connectorGroup1, SWT.NONE, true);
      interface1.setLabel(i18n.tr("Interface"));
      interface1.setObjectClass(Interface.class);
      interface1.setObjectId(linkEditor.getInterfaceId1());
      interface1.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      Group connectorGroup2 = new Group(dialogArea, SWT.BORDER);
      connectorGroup2.setText(i18n.tr("Connector 2 \u2013 {0}", session.getObjectNameWithAlias(linkEditor.getElement2())));
      connectorGroup2.setLayout(new GridLayout());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 3;
      connectorGroup2.setLayoutData(gd);

      connector2 = new LabeledText(connectorGroup2, SWT.NONE);
      connector2.setLabel(i18n.tr("Name (leave empty to use interface name when interface is set)"));
      connector2.setText(linkEditor.getConnectorName2());
      connector2.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      interface2 = new ObjectSelector(connectorGroup2, SWT.NONE, true);
      interface2.setLabel(i18n.tr("Interface"));
      interface2.setObjectClass(Interface.class);
      interface2.setObjectId(linkEditor.getInterfaceId2());
      interface2.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      routingAlgorithm = new LabeledCombo(dialogArea, SWT.NONE);
      routingAlgorithm.setLabel(i18n.tr("Routing algorithm"));
      routingAlgorithm.add(i18n.tr("Map default"));
      routingAlgorithm.add(i18n.tr("Direct"));
      routingAlgorithm.add(i18n.tr("Manhattan"));
      routingAlgorithm.add(i18n.tr("Bend points"));
      routingAlgorithm.select(linkEditor.getRoutingAlgorithm());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      routingAlgorithm.setLayoutData(gd);

      comboLinkStyle = new LabeledCombo(dialogArea, SWT.NONE);
      comboLinkStyle.setLabel(i18n.tr("Line style"));
      comboLinkStyle.add(i18n.tr("Map default"));
      comboLinkStyle.add(i18n.tr("Solid"));
      comboLinkStyle.add(i18n.tr("Dash"));
      comboLinkStyle.add(i18n.tr("Dot"));
      comboLinkStyle.add(i18n.tr("Dashdot"));
      comboLinkStyle.add(i18n.tr("Dashdotdot"));
      comboLinkStyle.select(linkEditor.getLineStyle());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      comboLinkStyle.setLayoutData(gd);      

      spinerLineWidth = new LabeledSpinner(dialogArea, SWT.NONE);
      spinerLineWidth.setLabel(i18n.tr("Line width (0 for map default)"));
      spinerLineWidth.setRange(0, 100);
      spinerLineWidth.setSelection(linkEditor.getLineWidth());

      final Group optionsGroup = new Group(dialogArea, SWT.NONE);
      optionsGroup.setText(i18n.tr("Options"));
      optionsGroup.setLayout(new GridLayout());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 3;
      optionsGroup.setLayoutData(gd);

      checkExcludeFromAutomaticUpdate = new Button(optionsGroup, SWT.CHECK);
      checkExcludeFromAutomaticUpdate.setText("Exclude from automatic updates");
      checkExcludeFromAutomaticUpdate.setSelection(linkEditor.isExcludeFromAutomaticUpdates());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 3;
      checkExcludeFromAutomaticUpdate.setLayoutData(gd);

		return dialogArea;
	}

   /**
    * @see org.netxms.nxmc.modules.networkmaps.propertypages.LinkPropertyPage#setupLayout(org.eclipse.swt.layout.GridLayout)
    */
   @Override
   protected void setupLayout(GridLayout layout)
   {
      layout.numColumns = 3;
   }

   /**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(final boolean isApply)
	{
      linkEditor.setName(name.getText());
      linkEditor.setConnectorName1(connector1.getText());
      linkEditor.setConnectorName2(connector2.getText());
      linkEditor.setInterfaceId1(interface1.getObjectId());
      linkEditor.setInterfaceId2(interface2.getObjectId());
      linkEditor.setRoutingAlgorithm(routingAlgorithm.getSelectionIndex());
      linkEditor.setLineStyle(comboLinkStyle.getSelectionIndex());
      linkEditor.setLineWidth(spinerLineWidth.getSelection());
      linkEditor.setLabelPosition(spinnerLabelPositon.getSelection());
      linkEditor.setExcludeFromAutomaticUpdates(checkExcludeFromAutomaticUpdate.getSelection());
      linkEditor.setModified();
		return true;
	} 
}
