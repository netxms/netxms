/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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
package org.netxms.nxmc.modules.objects.dialogs;

import java.util.Map.Entry;
import java.util.UUID;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.asset.AssetManagementAttribute;
import org.netxms.client.objects.GenericObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.widgets.LabeledSpinner;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.logwatch.widgets.helpers.LogParser;
import org.netxms.nxmc.modules.objects.widgets.ObjectSelector;
import org.netxms.nxmc.tools.IPAddressValidator;
import org.netxms.nxmc.tools.MacAddressValidator;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * Editor dialog for asset management attribute instance
 */
public class EditAssetInstanceDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(EditAssetInstanceDialog.class);
   private final Logger log = LoggerFactory.getLogger(EditAssetInstanceDialog.class);
   
   private String attributeDisplayName;
   private String value;
   private AssetManagementAttribute config;
   private Composite mainElement;

   /**
    * Constructor
    * 
    * @param parentShell shell
    * @param attributeName attribute name
    * @param value attribute value
    */
   public EditAssetInstanceDialog(Shell parentShell, String attributeName, String attributeDisplayName, String value)
   {
      super(parentShell);   
      config = Registry.getSession().getAssetManagementAttributes().get(attributeName);
      this.attributeDisplayName = attributeDisplayName;
      this.value = value;
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(i18n.tr("Edit asset amangement attribute instance"));
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      GridLayout layout = new GridLayout();
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
      dialogArea.setLayout(layout);
      
      switch (config.getDataType())
      {
         case INTEGER:
         {
            LabeledSpinner spiner = new LabeledSpinner(dialogArea, SWT.NONE);
            spiner.setLabel(attributeDisplayName);
            if (value != null)
            {
               int integer = 0;
               try
               {
                  integer = Integer.parseInt(value);
               }
               catch (NumberFormatException e)
               {
                  log.error("Failed to parse integer", e);
               }
               spiner.setSelection(integer);
            }
            if (config.getRangeMax() != 0 || config.getRangeMin() != 0)
            {
               spiner.getSpinnerControl().setMaximum(config.getRangeMax());
               spiner.getSpinnerControl().setMinimum(config.getRangeMin());
            }
            mainElement = spiner;
            break;
         }
         case NUMBER:
         {
            LabeledText number = new LabeledText(dialogArea, SWT.NONE);
            number.setLabel(attributeDisplayName); 
            if (value != null)
            {
               number.setText(value);  
            }
            mainElement = number;
            break;
         }
         case BOOLEAN:
         {
            Combo booleanCombo = WidgetHelper.createLabeledCombo(dialogArea, SWT.BORDER | SWT.READ_ONLY, attributeDisplayName, WidgetHelper.DEFAULT_LAYOUT_DATA);
            booleanCombo.add("Yes");
            booleanCombo.add("No");
            if (value != null)
            {
               booleanCombo.select(LogParser.stringToBoolean(value) ? 0 : 1);
            }
            break;
         }
         case ENUM:
         {
            Combo enumCombo = WidgetHelper.createLabeledCombo(dialogArea, SWT.BORDER | SWT.READ_ONLY, attributeDisplayName, WidgetHelper.DEFAULT_LAYOUT_DATA);
            int currentIndex = 0;
            for (Entry<String, String> element : config.getEnumMapping().entrySet())
            {
               enumCombo.add(element.getValue().isBlank() ? element.getKey() : element.getValue());
               if (value != null && value.equals(element.getValue()))
                  enumCombo.select(currentIndex);
               currentIndex++;
            }
            mainElement = enumCombo;            
            break;
         }
         default:
         case STRING:
         case MAC_ADDRESS:
         case IP_ADDRESS:
         case UUID:
         {
            LabeledText text = new LabeledText(dialogArea, SWT.NONE);
            text.setLabel(attributeDisplayName);
            if (value != null)
            {
               text.setText(value);
            }
            mainElement = text;
            break;
         }
         case OBJECT_REFERENCE:
         {
            ObjectSelector selector = new ObjectSelector(dialogArea, SWT.NONE, false);
            selector.setLabel(attributeDisplayName);
            selector.setObjectClass(GenericObject.class);
            if (value != null)
            {
               int objectId = 0;
               try
               {
                  objectId = Integer.parseInt(value);
               }
               catch (NumberFormatException e)
               {
                  log.error("Failed to parse object id", e);
               }
               selector.setObjectId(objectId);    
            }
            mainElement = selector;
            break;
         }
      }
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      mainElement.setLayoutData(gd);
      
      return dialogArea;
   }
   
   /**
    * Get selected value
    * 
    * @return selected value
    */
   public String getValue()
   {
      return value;
   }
   
   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      switch (config.getDataType())
      {
         case INTEGER:
         {
            value = Integer.toString(((LabeledSpinner)mainElement).getSelection());
            break;
         }
         case BOOLEAN:
         {
            value = ((Combo)mainElement).getSelectionIndex() == 0 ? "Yes" : "No";
            break;
         }
         case ENUM:
         {
            int selectionIndex = ((Combo)mainElement).getSelectionIndex();
            if (selectionIndex == -1)
            {
               MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("One of options should be selected"));
               return;
            }
            value = (String)config.getEnumMapping().keySet().toArray()[selectionIndex];                       
            break;
         }
         default:
         case NUMBER:
         case STRING:
         case MAC_ADDRESS:
         case IP_ADDRESS:
         case UUID:
         {
            value = ((LabeledText)mainElement).getText();
            break;
         }
         case OBJECT_REFERENCE:
         {
            value = Long.toString(((ObjectSelector)mainElement).getObjectId());
            break;
         }
      }
      
      //validate
      switch (config.getDataType())
      {
         case NUMBER:
            try
            {
               Double.parseDouble(value);
            }
            catch (NumberFormatException e)
            {
               MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Value must be double"));
               return;
            }
            break;
         case STRING:
            if (config.getRangeMax() != 0 && config.getRangeMin() != 0)
            {
               if (value.length() < config.getRangeMin())
               {
                  MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("String value too short"));  
                  return;                
               }
            }
            break;
         case MAC_ADDRESS:  
            if (!WidgetHelper.validateTextInput(((LabeledText)mainElement), new MacAddressValidator(false), null))
               return;
            break;
         case IP_ADDRESS:   
            if (!WidgetHelper.validateTextInput(((LabeledText)mainElement), new IPAddressValidator(false), null))
               return;
            break;
         case UUID:       
            try
            {
               UUID.fromString(value);
            }
            catch(IllegalArgumentException e)
            {
               MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Invalid UUID format")); 
               return;               
            }
            break;
      }
         
      super.okPressed();
   }
}
