/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.traffic.widgets;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.TrafficConnector;
import org.netxms.client.TrafficCredentialField;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import com.google.gson.JsonPrimitive;
import com.google.gson.JsonSyntaxException;

/**
 * Editor for traffic observer credentials. Renders one control per credential field declared
 * by the selected connector and assembles the credentials JSON from them. Password fields are
 * never pre-filled; in edit mode an empty password field means "keep current value" (the server
 * merges stored secrets back in), so their JSON keys are omitted when left empty. A connector
 * that declares no fields gets a raw JSON text area instead.
 */
public class TrafficCredentialsEditor extends Composite
{
   private final I18n i18n = LocalizationHelper.getI18n(TrafficCredentialsEditor.class);

   private boolean editMode;
   private List<TrafficCredentialField> fields = new ArrayList<TrafficCredentialField>();
   private List<Control> controls = new ArrayList<Control>();
   private JsonObject extraValues = new JsonObject();
   private LabeledText rawEditor = null;

   /**
    * Create editor.
    *
    * @param parent parent composite
    * @param style widget style
    */
   public TrafficCredentialsEditor(Composite parent, int style)
   {
      super(parent, style);

      GridLayout layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      setLayout(layout);
   }

   /**
    * Rebuild controls for given connector. Current values (sanitized credentials JSON from the
    * server) pre-fill non-password fields and switch the editor into edit mode, where an empty
    * password field means "keep stored value"; keys not declared by the connector are preserved
    * and written back unchanged. Pass null when no credentials are stored for this connector,
    * so required password fields must be entered.
    *
    * @param connector connector to build form for (null clears the editor)
    * @param currentValues current credentials JSON or null
    */
   public void setConnector(TrafficConnector connector, String currentValues)
   {
      for(Control c : getChildren())
         c.dispose();
      fields.clear();
      controls.clear();
      extraValues = new JsonObject();
      rawEditor = null;
      editMode = (currentValues != null);

      JsonObject values = parseObject(currentValues);
      if (connector != null)
      {
         if (connector.getCredentialFields().isEmpty())
         {
            rawEditor = new LabeledText(this, SWT.NONE, SWT.BORDER | SWT.MULTI);
            rawEditor.setLabel(i18n.tr("Credentials (JSON)"));
            if (values != null)
               rawEditor.setText(values.toString());
            GridData gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
            gd.heightHint = 120;
            rawEditor.setLayoutData(gd);
            layout(true, true);
            return;
         }
         for(TrafficCredentialField f : connector.getCredentialFields())
         {
            fields.add(f);
            controls.add(createControl(f, values));
         }
      }

      // Keep keys the connector did not declare so hand-edited settings survive a form save
      if (values != null)
      {
         for(String key : values.keySet())
         {
            if (findField(key) == null)
               extraValues.add(key, values.get(key));
         }
      }

      layout(true, true);
   }

   /**
    * Create control for given field.
    *
    * @param f field descriptor
    * @param values current values (may be null)
    * @return created control
    */
   private Control createControl(TrafficCredentialField f, JsonObject values)
   {
      JsonElement current = (values != null) ? values.get(f.getName()) : null;
      String currentText = ((current != null) && current.isJsonPrimitive()) ? current.getAsString() : null;
      String initialText = (currentText != null) ? currentText : f.getDefaultValue();

      if (f.getType() == TrafficCredentialField.Type.BOOLEAN)
      {
         Button button = new Button(this, SWT.CHECK);
         button.setText(f.getDisplayName());
         button.setSelection(initialText != null ? Boolean.parseBoolean(initialText) : false);
         button.setToolTipText(f.getDescription());
         button.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
         return button;
      }

      boolean password = (f.getType() == TrafficCredentialField.Type.PASSWORD);
      LabeledText text = new LabeledText(this, SWT.NONE, SWT.BORDER | (password ? SWT.PASSWORD : 0));
      String label = f.getDisplayName();
      if (password && editMode)
         label += i18n.tr(" (leave empty to keep current)");
      else if (f.isRequired())
         label += " *";
      text.setLabel(label);
      text.getTextControl().setToolTipText(f.getDescription());
      if (!password && (initialText != null))
         text.setText(initialText);
      text.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      return text;
   }

   /**
    * Find declared field by name.
    *
    * @param name field name
    * @return field descriptor or null
    */
   private TrafficCredentialField findField(String name)
   {
      for(TrafficCredentialField f : fields)
         if (f.getName().equals(name))
            return f;
      return null;
   }

   /**
    * Parse JSON object text.
    *
    * @param text JSON text or null
    * @return parsed object or null if text is null or not a JSON object
    */
   private static JsonObject parseObject(String text)
   {
      if ((text == null) || text.trim().isEmpty())
         return null;
      try
      {
         JsonElement e = JsonParser.parseString(text);
         return e.isJsonObject() ? e.getAsJsonObject() : null;
      }
      catch(JsonSyntaxException e)
      {
         return null;
      }
   }

   /**
    * Validate entered values. Required fields must be non-empty (password fields only when
    * creating), integer fields must parse. Error messages are shown inline on offending controls.
    *
    * @return true if all values are valid
    */
   public boolean validate()
   {
      if (rawEditor != null)
      {
         String text = rawEditor.getText().trim();
         boolean valid = text.isEmpty() || (parseObject(text) != null);
         rawEditor.setErrorMessage(valid ? null : i18n.tr("Credentials must be a valid JSON object"));
         return valid;
      }

      boolean valid = true;
      for(int i = 0; i < fields.size(); i++)
      {
         TrafficCredentialField f = fields.get(i);
         if (f.getType() == TrafficCredentialField.Type.BOOLEAN)
            continue;

         LabeledText text = (LabeledText)controls.get(i);
         String value = text.getText().trim();
         String error = null;
         if (value.isEmpty())
         {
            if (f.isRequired() && !((f.getType() == TrafficCredentialField.Type.PASSWORD) && editMode))
               error = i18n.tr("Value is required");
         }
         else if (f.getType() == TrafficCredentialField.Type.INTEGER)
         {
            try
            {
               Long.parseLong(value);
            }
            catch(NumberFormatException e)
            {
               error = i18n.tr("Value must be an integer");
            }
         }
         text.setErrorMessage(error);
         if (error != null)
            valid = false;
      }
      return valid;
   }

   /**
    * Build credentials JSON from entered values. Empty optional fields and empty password
    * fields are omitted.
    *
    * @return credentials JSON text
    */
   public String getCredentials()
   {
      if (rawEditor != null)
         return rawEditor.getText().trim();

      JsonObject json = new JsonObject();
      for(int i = 0; i < fields.size(); i++)
      {
         TrafficCredentialField f = fields.get(i);
         Control c = controls.get(i);
         switch(f.getType())
         {
            case BOOLEAN:
               json.add(f.getName(), new JsonPrimitive(((Button)c).getSelection()));
               break;
            case INTEGER:
            {
               String value = ((LabeledText)c).getText().trim();
               if (!value.isEmpty())
                  json.add(f.getName(), new JsonPrimitive(Long.parseLong(value)));
               break;
            }
            default:
            {
               String value = ((LabeledText)c).getText();
               if (f.getType() == TrafficCredentialField.Type.STRING)
                  value = value.trim();
               if (!value.isEmpty())
                  json.add(f.getName(), new JsonPrimitive(value));
               break;
            }
         }
      }
      for(String key : extraValues.keySet())
         json.add(key, extraValues.get(key));
      return json.toString();
   }

}
