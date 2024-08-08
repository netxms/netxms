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
package org.netxms.nxmc.modules.assetmanagement.widgets;

import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.List;
import java.util.Map.Entry;
import java.util.UUID;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.DateTime;
import org.netxms.base.Pair;
import org.netxms.client.asset.AssetAttribute;
import org.netxms.client.objects.GenericObject;
import org.netxms.nxmc.base.widgets.LabeledCombo;
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

   private List<Pair<String, String>> enumValues;
   private AssetAttribute attribute;
   private Control editorControl;
   private LabeledText text;
   private LabeledCombo combo;
   private ObjectSelector objectSelector;
   private DateTime dateSelector;

   /**
    * @param parent
    * @param style
    */
   public AssetPropertyEditor(Composite parent, int style, AssetAttribute attribute)
   {
      super(parent, style);
      this.attribute = attribute;

      setLayout(new FillLayout());

      String label = attribute.getEffectiveDisplayName();
      if (attribute.isMandatory())
         label = label + " *";

      switch(attribute.getDataType())
      {
         case BOOLEAN:
            combo = new LabeledCombo(this, SWT.NONE);
            combo.setLabel(label);
            combo.add("Yes");
            combo.add("No");
            editorControl = combo.getControl();
            break;
         case ENUM:
            combo = new LabeledCombo(this, SWT.NONE);
            combo.setLabel(label);
            enumValues = new ArrayList<Pair<String, String>>(attribute.getEnumValues().size());
            for(Entry<String, String> element : attribute.getEnumValues().entrySet())
               enumValues.add(new Pair<String, String>(element.getValue().isBlank() ? element.getKey() : element.getValue(), element.getKey()));
            enumValues.sort((a1, a2) -> a1.getFirst().compareToIgnoreCase(a2.getFirst()));
            for (Pair<String, String> element : enumValues)
               combo.add(element.getFirst());
            editorControl = combo.getControl();
            break;
         case OBJECT_REFERENCE:
            objectSelector = new ObjectSelector(this, SWT.NONE, !attribute.isMandatory());
            objectSelector.setLabel(label);
            objectSelector.setObjectClass(GenericObject.class);
            editorControl = objectSelector;
            break;
         case DATE:
            dateSelector = (DateTime)WidgetHelper.createLabeledControl(this, SWT.DATE | SWT.DROP_DOWN | SWT.BORDER, (p, s) -> new DateTime(p, s), label, null);
            editorControl = dateSelector;
            break;
         default:
            text = new LabeledText(this, SWT.NONE);
            text.setLabel(label);
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
         case BOOLEAN:
            combo.select(LogParser.stringToBoolean(value) ? 0 : 1);
            break;
         case ENUM:
            if (value != null)
            {
               int currentIndex = 0;
               for(Pair<String, String> element : enumValues)
               {
                  if (value.equals(element.getSecond()))
                     break;
                  currentIndex++;
               }
               combo.select(currentIndex);
            }
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
         case DATE:
            if (value != null)
            {
               final Calendar c = Calendar.getInstance();
               SimpleDateFormat sdf = new SimpleDateFormat("yyyyMMdd");
               try
               {
                  c.setTime(sdf.parse(value));
               }
               catch(ParseException e)
               {
                  logger.error("Cannot parse date", e);
               }
               dateSelector.setDate(c.get(Calendar.YEAR), c.get(Calendar.MONTH), c.get(Calendar.DAY_OF_MONTH));
            }
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
         case BOOLEAN:
            if ((combo.getSelectionIndex() == -1) && attribute.isMandatory())
            {
               combo.setErrorMessage(i18n.tr("This property is mandatory"));
            }
            combo.setErrorMessage(null);
            return true;
         case ENUM:
            if ((combo.getSelectionIndex() == -1) && attribute.isMandatory())
            {
               combo.setErrorMessage(i18n.tr("This property is mandatory"));
            }
            combo.setErrorMessage(null);
            return true;
         case IP_ADDRESS:
            if (attribute.isMandatory() && text.getText().isBlank())
            {
               text.setErrorMessage(i18n.tr("This property is mandatory"));
               return false;
            }
            return WidgetHelper.validateTextInput(text, new IPAddressValidator(true));
         case MAC_ADDRESS:
            if (attribute.isMandatory() && text.getText().isBlank())
            {
               text.setErrorMessage(i18n.tr("This property is mandatory"));
               return false;
            }
            return WidgetHelper.validateTextInput(text, new MacAddressValidator(true));
         case NUMBER:
            if (attribute.isMandatory() && text.getText().isBlank())
            {
               text.setErrorMessage(i18n.tr("This property is mandatory"));
               return false;
            }
            if (text.getText().isBlank())
            {
               text.setErrorMessage(null);
               return true; // Explicitly allow empty values
            }
            return WidgetHelper.validateTextInput(text, new NumericTextFieldValidator((double)attribute.getRangeMin(), (double)attribute.getRangeMax()));
         case INTEGER:
            if (attribute.isMandatory() && text.getText().isBlank())
            {
               text.setErrorMessage(i18n.tr("This property is mandatory"));
               return false;
            }
            if (text.getText().isBlank())
            {
               text.setErrorMessage(null);
               return true; // Explicitly allow empty values
            }
            return WidgetHelper.validateTextInput(text, new NumericTextFieldValidator((long)attribute.getRangeMin(), (long)attribute.getRangeMax()));
         case OBJECT_REFERENCE:
            if (attribute.isMandatory() && (objectSelector.getObjectId() == 0))
            {
               objectSelector.setErrorMessage(i18n.tr("This property is mandatory"));
               return false;
            }
            objectSelector.setErrorMessage(null);
            return true;
         case STRING:
            if (attribute.isMandatory() && text.getText().isBlank())
            {
               text.setErrorMessage(i18n.tr("This property is mandatory"));
               return false;
            }
            if (attribute.getRangeMax() != 0 && attribute.getRangeMin() != 0)
            {
               String value = text.getText().trim();
               if (value.isEmpty())
               {
                  text.setErrorMessage(null);
                  return true; // Explicitly allow empty values
               }
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
            text.setErrorMessage(null);
            return true;
         case UUID:
            if (attribute.isMandatory() && text.getText().isBlank())
            {
               text.setErrorMessage(i18n.tr("This property is mandatory"));
               return false;
            }
            if (text.getText().isBlank())
            {
               text.setErrorMessage(null);
               return true; // Explicitly allow empty values
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
            text.setErrorMessage(null);
            return true;
         case DATE:
            return true;
         default:
            if (attribute.isMandatory() && text.getText().isBlank())
            {
               text.setErrorMessage(i18n.tr("This property is mandatory"));
               return false;
            }
            text.setErrorMessage(null);
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
         case BOOLEAN:
            if (combo.getSelectionIndex() == -1)
               return "";
            return (combo.getSelectionIndex() == 0) ? "true" : "false";
         case ENUM:
            if (combo.getSelectionIndex() == -1)
               return "";
            return enumValues.get(combo.getSelectionIndex()).getSecond();
         case OBJECT_REFERENCE:
            return (objectSelector.getObjectId() == 0) ? "" : Long.toString(objectSelector.getObjectId());
         case DATE:
            return Integer.toString(dateSelector.getYear() * 10000 + (dateSelector.getMonth() + 1) * 100 + dateSelector.getDay());
         default:
            return text.getText().trim();
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
