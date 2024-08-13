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
package org.netxms.ui.eclipse.dashboard.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Text;
import org.netxms.ui.eclipse.dashboard.widgets.internal.DashboardElementConfig;
import org.netxms.ui.eclipse.tools.ColorConverter;
import org.netxms.ui.eclipse.widgets.ExtendedColorSelector;
import org.netxms.ui.eclipse.widgets.LabeledSpinner;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Widget for configuring element title
 */
public class TitleConfigurator extends Composite
{
   private Text text;
   private ExtendedColorSelector backgroundColor;
   private ExtendedColorSelector foregroundColor;
   private LabeledSpinner fontHeight;
   private LabeledText fontName;

   /**
    * Create new title configurator.
    *
    * @param parent parent composite
    * @param config element configuration
    */
   public TitleConfigurator(Composite parent, DashboardElementConfig config)
   {
      this(parent, config, false);
   }

   /**
    * Create new title configurator.
    *
    * @param parent parent composite
    * @param config element configuration
    * @param standalone create as standalone widget (without wrapping group)
    */
   public TitleConfigurator(Composite parent, DashboardElementConfig config, boolean standalone)
   {
      super(parent, SWT.NONE);

      setLayout(new FillLayout());

      Composite group;
      if (standalone)
      {
         group = new Composite(this, SWT.NONE);
      }
      else
      {
         group = new Group(this, SWT.NONE);
         ((Group)group).setText("Title");
      }

      GridLayout layout = new GridLayout();
      layout.numColumns = 3;
      layout.makeColumnsEqualWidth = true;
      group.setLayout(layout);

      if (standalone)
      {
         Label label = new Label(group, SWT.NONE);
         label.setText("Title");
         GridData gd = new GridData();
         gd.horizontalAlignment = SWT.FILL;
         gd.grabExcessHorizontalSpace = true;
         gd.horizontalSpan = layout.numColumns;
         label.setLayoutData(gd);
      }

      text = new Text(group, SWT.BORDER);
      text.setText(config.getTitle());
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = layout.numColumns;
      text.setLayoutData(gd);

      backgroundColor = createColorSelector(group, "Background");
      backgroundColor.setColorValue(ColorConverter.parseColorDefinition(config.getTitleBackground()));
      backgroundColor.setLayoutData(new GridData(SWT.FILL, SWT.FILL, false, false, 1, 2));

      foregroundColor = createColorSelector(group, "Foreground");
      foregroundColor.setColorValue(ColorConverter.parseColorDefinition(config.getTitleForeground()));
      foregroundColor.setLayoutData(new GridData(SWT.FILL, SWT.FILL, false, false, 1, 2));

      fontHeight = new LabeledSpinner(group, SWT.NONE);
      fontHeight.setLabel("Font size adjustment");
      fontHeight.setRange(-20, 20);
      fontHeight.setSelection(config.getTitleFontSize());
      fontHeight.setLayoutData(new GridData(SWT.FILL, SWT.TOP, true, false));

      fontName = new LabeledText(group, SWT.NONE);
      fontName.setLabel("Font name");
      fontName.setText((config.getTitleFontName() != null) ? config.getTitleFontName() : "");
      fontName.setLayoutData(new GridData(SWT.FILL, SWT.TOP, true, false));
   }

   /**
    * Create color selector.
    *
    * @param parent parent composite
    * @param name name for this selector
    * @return new color selector widget
    */
   private static ExtendedColorSelector createColorSelector(Composite parent, String name)
   {
      ExtendedColorSelector selector = new ExtendedColorSelector(parent);
      selector.setLabels(name, "Default", "Custom");
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.TOP;
      selector.setLayoutData(gd);
      return selector;
   }

   /**
    * Update element configuration from widget.
    * 
    * @param config element configuration to update
    */
   public void updateConfiguration(DashboardElementConfig config)
   {
      config.setTitle(text.getText());
      config.setTitleBackground(ColorConverter.rgbToCss(backgroundColor.getColorValue()));
      config.setTitleForeground(ColorConverter.rgbToCss(foregroundColor.getColorValue()));
      config.setTitleFontSize(fontHeight.getSelection());
      config.setTitleFontName(fontName.getText());
   }
}
