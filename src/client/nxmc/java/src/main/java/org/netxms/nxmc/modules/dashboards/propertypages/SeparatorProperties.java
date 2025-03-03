/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Scale;
import org.eclipse.swt.widgets.Spinner;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.dashboards.config.DashboardElementConfig;
import org.netxms.nxmc.modules.dashboards.config.SeparatorConfig;
import org.netxms.nxmc.tools.ColorConverter;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Separator configuration page
 */
public class SeparatorProperties extends DashboardElementPropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(SeparatorProperties.class);

	private SeparatorConfig config;
	private ColorSelector foreground;
	private ColorSelector background;
	private Scale lineWidthScale;
	private Spinner lineWidthSpinner;
	private Spinner marginLeft;
	private Spinner marginRight;
	private Spinner marginTop;
	private Spinner marginBottom;

   /**
    * Create page.
    *
    * @param elementConfig element configuration
    */
   public SeparatorProperties(DashboardElementConfig elementConfig)
   {
      super(LocalizationHelper.getI18n(SeparatorProperties.class).tr("Separator"), elementConfig);
   }

   /**
    * @see org.netxms.nxmc.modules.dashboards.propertypages.DashboardElementPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "separator";
   }

   /**
    * @see org.netxms.nxmc.modules.dashboards.propertypages.DashboardElementPropertyPage#isVisible()
    */
   @Override
   public boolean isVisible()
   {
      return elementConfig instanceof SeparatorConfig;
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
      config = (SeparatorConfig)elementConfig;

		Composite dialogArea = new Composite(parent, SWT.NONE);

		GridLayout layout = new GridLayout();
		layout.numColumns = 2;
		dialogArea.setLayout(layout);

		Composite fgArea = new Composite(dialogArea, SWT.NONE);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		fgArea.setLayoutData(gd);
		RowLayout areaLayout = new RowLayout();
		areaLayout.type = SWT.HORIZONTAL;
		areaLayout.marginBottom = 0;
		areaLayout.marginTop = 0;
		areaLayout.marginLeft = 0;
		areaLayout.marginRight = 0;
		fgArea.setLayout(areaLayout);
      new Label(fgArea, SWT.NONE).setText(i18n.tr("Line color:"));
		foreground = new ColorSelector(fgArea);
		foreground.setColorValue(ColorConverter.rgbFromInt(config.getForegroundColorAsInt()));
		
		Composite bgArea = new Composite(dialogArea, SWT.NONE);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		bgArea.setLayoutData(gd);
		areaLayout = new RowLayout();
		areaLayout.type = SWT.HORIZONTAL;
		areaLayout.marginBottom = 0;
		areaLayout.marginTop = 0;
		areaLayout.marginLeft = 0;
		areaLayout.marginRight = 0;
		bgArea.setLayout(areaLayout);
      new Label(bgArea, SWT.NONE).setText(i18n.tr("Background color:"));
		background = new ColorSelector(bgArea);
		background.setColorValue(ColorConverter.rgbFromInt(config.getBackgroundColorAsInt()));

      Composite lineWidthGroup = new Composite(dialogArea, SWT.NONE);
      layout = new GridLayout();
      layout.numColumns = 2;
      layout.horizontalSpacing = WidgetHelper.OUTER_SPACING;
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.marginTop = WidgetHelper.OUTER_SPACING;
      lineWidthGroup.setLayout(layout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      lineWidthGroup.setLayoutData(gd);

      Label label = new Label(lineWidthGroup, SWT.NONE);
      label.setText(i18n.tr("Line width:"));
      gd = new GridData();
      gd.horizontalAlignment = SWT.LEFT;
      gd.horizontalSpan = 2;
      label.setLayoutData(gd);

      lineWidthScale = new Scale(lineWidthGroup, SWT.HORIZONTAL);
      lineWidthScale.setMinimum(0);
      lineWidthScale.setMaximum(15);
      lineWidthScale.setSelection(config.getLineWidth());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      lineWidthScale.setLayoutData(gd);
      lineWidthScale.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				lineWidthSpinner.setSelection(lineWidthScale.getSelection());
			}
      });
      
      lineWidthSpinner = new Spinner(lineWidthGroup, SWT.BORDER);
      lineWidthSpinner.setMinimum(1);
      lineWidthSpinner.setMaximum(600);
      lineWidthSpinner.setSelection(config.getLineWidth());
      lineWidthSpinner.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				lineWidthScale.setSelection(lineWidthSpinner.getSelection());
			}
		});

      marginLeft = WidgetHelper.createLabeledSpinner(dialogArea, SWT.BORDER, i18n.tr("Left margin"), 0, 255, WidgetHelper.DEFAULT_LAYOUT_DATA);
      marginLeft.setSelection(config.getLeftMargin());
		
      marginRight = WidgetHelper.createLabeledSpinner(dialogArea, SWT.BORDER, i18n.tr("Right margin"), 0, 255, WidgetHelper.DEFAULT_LAYOUT_DATA);
      marginRight.setSelection(config.getRightMargin());
		
      marginTop = WidgetHelper.createLabeledSpinner(dialogArea, SWT.BORDER, i18n.tr("Top margin"), 0, 255, WidgetHelper.DEFAULT_LAYOUT_DATA);
      marginTop.setSelection(config.getTopMargin());
		
      marginBottom = WidgetHelper.createLabeledSpinner(dialogArea, SWT.BORDER, i18n.tr("Bottom margin"), 0, 255, WidgetHelper.DEFAULT_LAYOUT_DATA);
      marginBottom.setSelection(config.getBottomMargin());

		return dialogArea;
	}

   /**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(boolean isApply)
	{
		config.setForeground("0x" + Integer.toHexString(ColorConverter.rgbToInt(foreground.getColorValue()))); //$NON-NLS-1$
		config.setBackground("0x" + Integer.toHexString(ColorConverter.rgbToInt(background.getColorValue()))); //$NON-NLS-1$
		config.setLineWidth(lineWidthScale.getSelection());
		config.setLeftMargin(marginLeft.getSelection());
		config.setRightMargin(marginRight.getSelection());
		config.setTopMargin(marginTop.getSelection());
		config.setBottomMargin(marginBottom.getSelection());
		return true;
	}
}
