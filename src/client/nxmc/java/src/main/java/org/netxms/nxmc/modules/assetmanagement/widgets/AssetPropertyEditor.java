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
package org.netxms.nxmc.modules.assetmanagement.widgets;

import java.util.Map.Entry;
import java.util.UUID;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.asset.AssetAttribute;
import org.netxms.client.objects.GenericObject;
import org.netxms.nxmc.base.widgets.LabeledCombo;
import org.netxms.nxmc.base.widgets.LabeledSpinner;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.logwatch.widgets.helpers.LogParser;
import org.netxms.nxmc.modules.objects.widgets.ObjectSelector;
import org.netxms.nxmc.tools.IPAddressValidator;
import org.netxms.nxmc.tools.MacAddressValidator;
import org.netxms.nxmc.tools.NumericTextFieldValidator;
import org.netxms.nxmc.tools.WidgetHelper;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * Base class for all asset property editors
 */
public class AssetPropertyEditor extends Composite
{
   private static final Logger logger = LoggerFactory.getLogger(AssetPropertyEditor.class);

   private final I18n i18n = LocalizationHelper.getI18n(AssetPropertyEditor.class);

   private AssetAttribute attribute;
   private Control editorControl;
   private LabeledSpinner spinner;
   private LabeledText text;
   private LabeledCombo combo;
   private ObjectSelector objectSelector;

   /**
    * @param parent
    * @param style
    */
   public AssetPropertyEditor(Composite parent, int style, AssetAttribute attribute)
   {
      super(parent, style);
      this.attribute = attribute;

      setLayout(new FillLayout());

      String attributeDisplayName = attribute.getEffectiveDisplayName();
      switch(attribute.getDataType())
      {
         case INTEGER:
            spinner = new LabeledSpinner(this, SWT.NONE);
            spinner.setLabel(attributeDisplayName);
            if (attribute.getRangeMax() != 0 || attribute.getRangeMin() != 0)
            {
               spinner.setRange(attribute.getRangeMin(), attribute.getRangeMax());
            }
            editorControl = spinner.getControl();
            break;
         case BOOLEAN:
            combo = new LabeledCombo(this, SWT.NONE);
            combo.setLabel(attributeDisplayName);
            combo.add("Yes");
            combo.add("No");
            editorControl = combo.getControl();
            break;
         case ENUM:
            combo = new LabeledCombo(this, SWT.NONE);
            combo.setLabel(attributeDisplayName);
            for(Entry<String, String> element : attribute.getEnumValues().entrySet())
               combo.add(element.getValue().isBlank() ? element.getKey() : element.getValue());
            editorControl = combo.getControl();
            break;
         case OBJECT_REFERENCE:
            objectSelector = new ObjectSelector(this, SWT.NONE, false);
            objectSelector.setLabel(attributeDisplayName);
            objectSelector.setObjectClass(GenericObject.class);
            editorControl = objectSelector;
            break;
         default:
            text = new LabeledText(this, SWT.NONE);
            text.setLabel(attributeDisplayName);
            editorControl = text.getControl();
            break;
      }
   }

   /**
    * Set new value to the editor.
    *
    * @param value new value
    */
   public void setValue(String value)
   {
      switch(attribute.getDataType())
      {
         case INTEGER:
            if (value != null)
            {
               int integer = 0;
               try
               {
                  integer = Integer.parseInt(value);
               }
               catch(NumberFormatException e)
               {
                  logger.error("Failed to parse integer", e);
                  spinner.setSelection(attribute.getRangeMin());
               }
               spinner.setSelection(integer);
            }
            else
            {
               spinner.setSelection(attribute.getRangeMin());
            }
            break;
         case BOOLEAN:
            combo.select(LogParser.stringToBoolean(value) ? 0 : 1);
            break;
         case ENUM:
            int currentIndex = 0;
            for(Entry<String, String> element : attribute.getEnumValues().entrySet())
            {
               if (value != null && value.equals(element.getKey()))
                  break;
               currentIndex++;
            }
            combo.select(currentIndex);
            break;
         case OBJECT_REFERENCE:
            if (value != null)
            {
               int objectId = 0;
               try
               {
                  objectId = Integer.parseInt(value);
               }
               catch(NumberFormatException e)
               {
                  logger.error("Cannot parse object ID", e);
               }
               objectSelector.setObjectId(objectId);
            }
            else
            {
               objectSelector.setObjectId(0);
            }
            break;
         case NUMBER:
            text.setText((value != null) ? value : "0");
            break;
         default:
            text.setText((value != null) ? value : "");
            break;
      }
   }

   /**
    * Validate input. Will update control error message accordingly.
    *
    * @return true if input is valid
    */
   public boolean validateInput()
   {
      switch(attribute.getDataType())
      {
         case ENUM:
            if ((combo.getSelectionIndex() == -1) && attribute.isMandatory())
            {
               combo.setErrorMessage(i18n.tr("This property is mandatory"));
            }
            return true;
         case IP_ADDRESS:
            if (attribute.isMandatory() && text.getText().isBlank())
            {
               text.setErrorMessage(i18n.tr("This property is mandatory"));
               return false;
            }
            return WidgetHelper.validateTextInput(text, new IPAddressValidator(false));
         case MAC_ADDRESS:
            if (attribute.isMandatory() && text.getText().isBlank())
            {
               text.setErrorMessage(i18n.tr("This property is mandatory"));
               return false;
            }
            return WidgetHelper.validateTextInput(text, new MacAddressValidator(false));
         case NUMBER:
            if (attribute.isMandatory() && text.getText().isBlank())
            {
               text.setErrorMessage(i18n.tr("This property is mandatory"));
               return false;
            }
            return WidgetHelper.validateTextInput(text, new NumericTextFieldValidator((double)attribute.getRangeMin(), (double)attribute.getRangeMax()));
         case STRING:
            if (attribute.isMandatory() && text.getText().isBlank())
            {
               text.setErrorMessage(i18n.tr("This property is mandatory"));
               return false;
            }
            if (attribute.getRangeMax() != 0 && attribute.getRangeMin() != 0)
            {
               String value = text.getText().trim();
               if (value.length() < attribute.getRangeMin())
               {
                  text.setErrorMessage(i18n.tr("Value is too short"));
                  return false;
               }
               if (value.length() > attribute.getRangeMax())
               {
                  text.setErrorMessage(i18n.tr("Value is too long"));
                  return false;
               }
            }
            return true;
         case UUID:
            if (attribute.isMandatory() && text.getText().isBlank())
            {
               text.setErrorMessage(i18n.tr("This property is mandatory"));
               return false;
            }
            try
            {
               UUID.fromString(text.getText().trim());
            }
            catch(IllegalArgumentException e)
            {
               text.setErrorMessage(i18n.tr("Invalid UUID"));
               return false;
            }
            return true;
         default:
            if (attribute.isMandatory() && text.getText().isBlank())
            {
               text.setErrorMessage(i18n.tr("This property is mandatory"));
               return false;
            }
            return true;
      }
   }

   /**
    * Get value from editor.
    *
    * @return property value
    */
   public String getValue()
   {
      switch(attribute.getDataType())
      {
         case INTEGER:
            return Integer.toString(spinner.getSelection());
         case BOOLEAN:
            return (combo.getSelectionIndex() == 0) ? "true" : "false";
         case ENUM:
            if (combo.getSelectionIndex() == -1)
               return "";
            return (String)attribute.getEnumValues().keySet().toArray()[combo.getSelectionIndex()];
         case OBJECT_REFERENCE:
            return Long.toString(objectSelector.getObjectId());
         default:
            return text.getText();
      }
   }

   /**
    * Get editor control.
    *
    * @return editor control
    */
   public Control getEditorControl()
   {
      return editorControl;
   }

   /**
    * Get attribute for the property being edited
    *
    * @return attribute for the property being edited
    */
   public AssetAttribute getAttribute()
   {
      return attribute;
   }
}
