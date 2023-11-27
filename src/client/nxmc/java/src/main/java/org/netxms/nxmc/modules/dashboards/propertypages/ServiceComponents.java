/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
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
package org.netxms.nxmc.modules.dashboards.propertypages;

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
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Scale;
import org.eclipse.swt.widgets.Spinner;
import org.netxms.client.maps.MapLayoutAlgorithm;
import org.netxms.client.maps.MapObjectDisplayMode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.NetworkMap;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.dashboards.config.DashboardElementConfig;
import org.netxms.nxmc.modules.dashboards.config.ServiceComponentsConfig;
import org.netxms.nxmc.modules.dashboards.widgets.TitleConfigurator;
import org.netxms.nxmc.modules.objects.dialogs.ObjectSelectionDialog;
import org.netxms.nxmc.modules.objects.widgets.ObjectSelector;
import org.netxms.nxmc.tools.ColorConverter;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Service component map properties
 */
public class ServiceComponents extends DashboardElementPropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(ServiceComponents.class);

   private ServiceComponentsConfig config;
   private ObjectSelector objectSelector;
   private TitleConfigurator title;
   private Scale zoomLevelScale;
   private Spinner zoomLevelSpinner;
   private Button enableObjectDoubleClick;
   private Button checkShowStatusIcon;
   private Button checkShowStatusFrame;
   private Button checkShowStatusBkgnd;
   private Button checkTranslucentLabelBkgnd;
   private Combo objectDisplayMode;
   private Combo routingAlgorithm;
   private Combo layoutAlgorithm;
   private Button radioColorDefault;
   private Button radioColorCustom;
   private ColorSelector linkColor;

   /**
    * Create page.
    *
    * @param elementConfig element configuration
    */
   public ServiceComponents(DashboardElementConfig elementConfig)
   {
      super(LocalizationHelper.getI18n(ServiceComponents.class).tr("Service Components"), elementConfig);
   }

   /**
    * @see org.netxms.nxmc.modules.dashboards.propertypages.DashboardElementPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "service-components";
   }

   /**
    * @see org.netxms.nxmc.modules.dashboards.propertypages.DashboardElementPropertyPage#isVisible()
    */
   @Override
   public boolean isVisible()
   {
      return elementConfig instanceof ServiceComponentsConfig;
   }

   /**
    * @see org.netxms.nxmc.modules.dashboards.propertypages.DashboardElementPropertyPage#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 0;
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      config = (ServiceComponentsConfig)elementConfig;

      Composite dialogArea = new Composite(parent, SWT.NONE);
      
      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      dialogArea.setLayout(layout);
      
      title = new TitleConfigurator(dialogArea, config);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      title.setLayoutData(gd);

      objectSelector = new ObjectSelector(dialogArea, SWT.NONE, false, true);
      objectSelector.setLabel(i18n.tr("Container"));
      objectSelector.setClassFilter(ObjectSelectionDialog.createContainerSelectionFilter());
      objectSelector.setObjectClass(AbstractObject.class);
      objectSelector.setObjectId(config.getObjectId());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      objectSelector.setLayoutData(gd);

      Label label = new Label(dialogArea, SWT.NONE);
      label.setText(i18n.tr("Zoom level (%)"));
      gd = new GridData();
      gd.horizontalSpan = 2;
      label.setLayoutData(gd);
      
      zoomLevelScale = new Scale(dialogArea, SWT.HORIZONTAL);
      zoomLevelScale.setMinimum(10);
      zoomLevelScale.setMaximum(400);
      zoomLevelScale.setSelection(config.getZoomLevel());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      zoomLevelScale.setLayoutData(gd);
      zoomLevelScale.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            zoomLevelSpinner.setSelection(zoomLevelScale.getSelection());
         }
      });
      
      zoomLevelSpinner = new Spinner(dialogArea, SWT.BORDER);
      zoomLevelSpinner.setMinimum(10);
      zoomLevelSpinner.setMaximum(400);
      zoomLevelSpinner.setSelection(config.getZoomLevel());
      zoomLevelSpinner.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            zoomLevelScale.setSelection(zoomLevelSpinner.getSelection());
         }
      });
      
      enableObjectDoubleClick = new Button(dialogArea, SWT.CHECK);
      enableObjectDoubleClick.setText(i18n.tr("Enable double click action on objects"));
      enableObjectDoubleClick.setSelection(config.isObjectDoubleClickEnabled());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      enableObjectDoubleClick.setLayoutData(gd);
      
      gd = new GridData();
      gd.horizontalAlignment = SWT.LEFT;
      gd.horizontalSpan = 2;
      layoutAlgorithm = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, i18n.tr("Layout algorithm"), gd);
      layoutAlgorithm.add(i18n.tr("Spring"));
      layoutAlgorithm.add(i18n.tr("Radial"));
      layoutAlgorithm.add(i18n.tr("Horizontal tree"));
      layoutAlgorithm.add(i18n.tr("Vertical tree"));
      layoutAlgorithm.add(i18n.tr("Sparse vertical tree"));
      layoutAlgorithm.select(config.getDefaultLayoutAlgorithm().getValue());

      /**** object display ****/
      Group objectDisplayGroup = new Group(dialogArea, SWT.NONE);
      objectDisplayGroup.setText(i18n.tr("Default display options"));
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      objectDisplayGroup.setLayoutData(gd);
      layout = new GridLayout();
      objectDisplayGroup.setLayout(layout);
      
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      objectDisplayMode = WidgetHelper.createLabeledCombo(objectDisplayGroup, SWT.READ_ONLY, i18n.tr("Display object as"), gd);
      objectDisplayMode.add(i18n.tr("Icons"));
      objectDisplayMode.add(i18n.tr("Small labels"));
      objectDisplayMode.add(i18n.tr("Large labels"));
      objectDisplayMode.add(i18n.tr("Status icons"));
      objectDisplayMode.select(config.getObjectDisplayMode().getValue());

      checkShowStatusIcon = new Button(objectDisplayGroup, SWT.CHECK);
      checkShowStatusIcon.setText(i18n.tr("Show status icon"));
      checkShowStatusIcon.setSelection((config.getFlags() & NetworkMap.MF_SHOW_STATUS_ICON) != 0);

      checkShowStatusFrame = new Button(objectDisplayGroup, SWT.CHECK);
      checkShowStatusFrame.setText(i18n.tr("Show status frame"));
      checkShowStatusFrame.setSelection((config.getFlags() & NetworkMap.MF_SHOW_STATUS_FRAME) != 0);

      checkShowStatusBkgnd = new Button(objectDisplayGroup, SWT.CHECK);
      checkShowStatusBkgnd.setText(i18n.tr("Show status background"));
      checkShowStatusBkgnd.setSelection((config.getFlags() & NetworkMap.MF_SHOW_STATUS_BKGND) != 0);
      
      checkTranslucentLabelBkgnd = new Button(objectDisplayGroup, SWT.CHECK);
      checkTranslucentLabelBkgnd.setText("Translucent label background");
      checkTranslucentLabelBkgnd.setSelection((config.getFlags() & NetworkMap.MF_TRANSLUCENT_LABEL_BKGND) != 0);

      /**** default link appearance ****/
      Group linkGroup = new Group(dialogArea, SWT.NONE);
      linkGroup.setText(i18n.tr("Default connection options"));
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
      routingAlgorithm.select(config.getDefaultLinkRouting() - 1);

      final SelectionListener listener = new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            linkColor.setEnabled(radioColorCustom.getSelection());
         }
      };

      radioColorDefault = new Button(linkGroup, SWT.RADIO);
      radioColorDefault.setText(i18n.tr("Default color"));
      radioColorDefault.setSelection(config.getDefaultLinkColor() < 0);
      radioColorDefault.addSelectionListener(listener);
      gd = new GridData();
      gd.verticalIndent = WidgetHelper.OUTER_SPACING * 2;
      radioColorDefault.setLayoutData(gd);

      radioColorCustom = new Button(linkGroup, SWT.RADIO);
      radioColorCustom.setText(i18n.tr("Custom color"));
      radioColorCustom.setSelection(config.getDefaultLinkColor() >= 0);
      radioColorCustom.addSelectionListener(listener);

      linkColor = new ColorSelector(linkGroup);
      linkColor.setColorValue(ColorConverter.rgbFromInt(config.getDefaultLinkColor()));
      linkColor.setEnabled(config.getDefaultLinkColor() >= 0);
      gd = new GridData();
      gd.horizontalIndent = 20;
      linkColor.getButton().setLayoutData(gd);

      return dialogArea;
   }

   /**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(boolean isApply)
   {
      title.updateConfiguration(config);
      config.setObjectId(objectSelector.getObjectId());
      config.setZoomLevel(zoomLevelSpinner.getSelection());
      config.setObjectDoubleClickEnabled(enableObjectDoubleClick.getSelection());
      config.setObjectDisplayMode(MapObjectDisplayMode.getByValue(objectDisplayMode.getSelectionIndex()));
      config.setDefaultLayoutAlgorithm(MapLayoutAlgorithm.getByValue(layoutAlgorithm.getSelectionIndex()));
      if (checkShowStatusIcon.getSelection())
         config.setFlags(config.getFlags() | NetworkMap.MF_SHOW_STATUS_ICON);
      else
         config.setFlags(config.getFlags() & ~NetworkMap.MF_SHOW_STATUS_ICON);
      if (checkShowStatusFrame.getSelection())
         config.setFlags(config.getFlags() | NetworkMap.MF_SHOW_STATUS_FRAME);
      else
         config.setFlags(config.getFlags() & ~NetworkMap.MF_SHOW_STATUS_FRAME);
      if (checkShowStatusBkgnd.getSelection())
         config.setFlags(config.getFlags() | NetworkMap.MF_SHOW_STATUS_BKGND);
      else
         config.setFlags(config.getFlags() & ~NetworkMap.MF_SHOW_STATUS_BKGND);
      if (checkTranslucentLabelBkgnd.getSelection())
         config.setFlags(config.getFlags() | NetworkMap.MF_TRANSLUCENT_LABEL_BKGND);
      else
         config.setFlags(config.getFlags() & ~NetworkMap.MF_TRANSLUCENT_LABEL_BKGND);

      config.setDefaultLinkRouting(routingAlgorithm.getSelectionIndex() + 1);
      if (radioColorDefault.getSelection())
         config.setDefaultLinkColor(-1);
      else
         config.setDefaultLinkColor(ColorConverter.rgbToInt(linkColor.getColorValue()));
      return true;
   }
}
