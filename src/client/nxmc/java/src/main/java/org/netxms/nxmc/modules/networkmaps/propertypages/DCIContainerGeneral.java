/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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

import org.eclipse.jface.preference.ColorSelector;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.maps.elements.NetworkMapDCIContainer;
import org.netxms.nxmc.base.propertypages.PropertyPage;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.ColorConverter;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * DCI container general property page
 */
public class DCIContainerGeneral extends PropertyPage
{
   private I18n i18n = LocalizationHelper.getI18n(DCIContainerGeneral.class);
   private NetworkMapDCIContainer container;
	private ColorSelector backgroundColorSelector;
   private ColorSelector textColorSelector;
   private ColorSelector borderColorSelector;
   private Button borderCheckbox;

   /**
    * Create new page for given DCI container.
    *
    * @param container DCI container to edit
    */
   public DCIContainerGeneral(NetworkMapDCIContainer container)
   {
      super(LocalizationHelper.getI18n(DCIContainerGeneral.class).tr("General"));
      this.container = container;
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      Composite dialogArea = new Composite(parent, SWT.NONE);
      
      GridLayout layout = new GridLayout();
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      dialogArea.setLayout(layout);    
      
      backgroundColorSelector = WidgetHelper.createLabeledColorSelector(dialogArea, i18n.tr("Background color"), WidgetHelper.DEFAULT_LAYOUT_DATA);
      backgroundColorSelector.setColorValue(ColorConverter.rgbFromInt(container.getBackgroundColor()));
      
      textColorSelector = WidgetHelper.createLabeledColorSelector(dialogArea, i18n.tr("Text color"), WidgetHelper.DEFAULT_LAYOUT_DATA);
      textColorSelector.setColorValue(ColorConverter.rgbFromInt(container.getTextColor()));
      
      final SelectionListener listener = new SelectionListener() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            borderColorSelector.setEnabled(borderCheckbox.getSelection());            
         }
         
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }
      };

      borderCheckbox = new Button(dialogArea, SWT.CHECK);
      borderCheckbox.setText(i18n.tr("Show &border"));
      borderCheckbox.setSelection(container.isBorderRequired());
      borderCheckbox.addSelectionListener(listener);      

      borderColorSelector = WidgetHelper.createLabeledColorSelector(dialogArea, i18n.tr("Border color"), WidgetHelper.DEFAULT_LAYOUT_DATA);
      borderColorSelector.setColorValue(ColorConverter.rgbFromInt(container.getBorderColor()));
      borderColorSelector.setEnabled(borderCheckbox.getSelection());

      return dialogArea;
   }
   
   /**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(final boolean isApply)
   {
      container.setBackgroundColor(ColorConverter.rgbToInt(backgroundColorSelector.getColorValue()));
      container.setTextColor(ColorConverter.rgbToInt(textColorSelector.getColorValue()));
      container.setBorderRequired(borderCheckbox.getSelection());
      container.setBorderColor(ColorConverter.rgbToInt(borderColorSelector.getColorValue()));
      return true;
   }
}
