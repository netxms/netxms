/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
package org.netxms.nxmc.base.widgets;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.netxms.nxmc.base.widgets.helpers.LineStyler;
import org.netxms.nxmc.base.widgets.helpers.StyleRange;
import org.netxms.nxmc.resources.ThemeEngine;
import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.gson.JsonElement;

/**
 * JSON viewer
 */
public class JsonViewer extends Composite
{
   private StyledText textControl;
   private Color colorBuiltin = ThemeEngine.getForegroundColor("JSON.Builtin");
   private Color colorKey = ThemeEngine.getForegroundColor("JSON.Key");
   private Color colorNumber = ThemeEngine.getForegroundColor("JSON.Number");
   private Color colorString = ThemeEngine.getForegroundColor("JSON.String");

   /**
    * @param parent
    * @param style
    */
   public JsonViewer(Composite parent, int style)
   {
      super(parent, style);
      setLayout(new FillLayout());
      textControl = new StyledText(this, SWT.MULTI | SWT.H_SCROLL | SWT.V_SCROLL | SWT.READ_ONLY);
      textControl.setFont(JFaceResources.getTextFont());
      textControl.setLineStyler(new LineStyler() {
         @Override
         public StyleRange[] styleLine(String line)
         {
            List<StyleRange> ranges = new ArrayList<>(8);
            int state = 0;
            int start = 0;
            char[] chars = line.toCharArray();
            for(int pos = 0; pos < chars.length; pos++)
            {
               switch(state)
               {
                  case 0:
                     if (chars[pos] == '"')
                     {
                        state = 1;
                        start = pos;
                     }
                     else if ((chars[pos] >= '0') && (chars[pos] <= '9'))
                     {
                        state = 3;
                        start = pos;
                     }
                     else if (((chars[pos] == 't') && line.substring(pos, pos + 4).equals("true")) ||
                              ((chars[pos] == 'f') && line.substring(pos, pos + 5).equals("false")) ||
                              ((chars[pos] == 'n') && line.substring(pos, pos + 4).equals("null")))
                     {
                        state = 4;
                        start = pos;
                        pos += (chars[pos] == 'f') ? 4 : 3;
                     }
                     else if ((chars[pos] == '{') || (chars[pos] == '}') || (chars[pos] == '[') || (chars[pos] == ']'))
                     {
                        ranges.add(new StyleRange(pos, 1, null, null, SWT.BOLD));
                     }
                     break;
                  case 1:
                     if (chars[pos] == '\\')
                     {
                        state = 2;
                     }
                     else if (chars[pos] == '"')
                     {
                        state = 0;
                        Color color = ((pos < chars.length - 1) && (chars[pos + 1] == ':')) ? colorKey : colorString;
                        ranges.add(new StyleRange(start, pos - start + 1, color, null));
                     }
                     break;
                  case 2:
                     state = 1;
                     break;
                  case 3:
                     if (Character.isWhitespace(chars[pos]) || (chars[pos] == ',') || (chars[pos] == ']') || (chars[pos] == '}'))
                     {
                        ranges.add(new StyleRange(start, pos - start, colorNumber, null));
                        state = 0;
                        pos--; // Process this character again in state 0
                     }
                     else if ((chars[pos] < '0') || (chars[pos] > '9'))
                     {
                        state = 0;
                     }
                     break;
                  case 4:
                     if (Character.isWhitespace(chars[pos]) || (chars[pos] == ',') || (chars[pos] == ']') || (chars[pos] == '}'))
                     {
                        ranges.add(new StyleRange(start, pos - start, colorBuiltin, null, SWT.BOLD));
                        pos--; // Process this character again in state 0
                     }
                     state = 0;
                     break;
                  default:
                     break;
               }
            }

            if (state == 3)
               ranges.add(new StyleRange(start, chars.length - start, colorNumber, null));
            else if (state == 4)
               ranges.add(new StyleRange(start, chars.length - start, colorBuiltin, null, SWT.BOLD));

            return !ranges.isEmpty() ? ranges.toArray(StyleRange[]::new) : null;
         }

         @Override
         public void dispose()
         {
         }
      });
   }

   /**
    * Set content for this viewer
    * 
    * @param jsonText JSON document as text
    * @param prettify if true, do reparsing and formatting on JSON document before display
    */
   public void setContent(String jsonText, boolean prettify)
   {
      if (prettify)
      {
         jsonText = jsonText.trim();
         if (!jsonText.startsWith("{") && !jsonText.startsWith("["))
            jsonText = "{" + jsonText + "}";
         Gson gson = new GsonBuilder().serializeNulls().setPrettyPrinting().create();
         JsonElement json = gson.fromJson(jsonText, JsonElement.class);
         jsonText = gson.toJson(json);
      }
      textControl.setText(jsonText);
   }
}
