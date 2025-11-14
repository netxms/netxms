/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
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
package org.netxms.nxmc.resources;

import org.eclipse.swt.graphics.RGB;

/**
 * Default light theme
 */
public class DefaultLightTheme extends Theme
{
   /**
    * Create default light theme
    */
   public DefaultLightTheme()
   {
      super("Light [built-in]");
      elements.put(".", new ThemeElement(null, null, null, 0));
      elements.put("AiAssistant.AssistantMessage", new ThemeElement(new RGB(255, 255, 255), new RGB(192, 192, 192)));
      elements.put("AiAssistant.ChatView", new ThemeElement(new RGB(240, 240, 240), null));
      elements.put("AiAssistant.UserMessage", new ThemeElement(new RGB(223, 235, 247), new RGB(136, 192, 247)));
      elements.put("Card.Title", new ThemeElement(new RGB(153, 180, 209), new RGB(0, 0, 0)));
      elements.put("Chart.Base", new ThemeElement(new RGB(255, 255, 255), new RGB(64, 66, 68)));
      elements.put("Chart.Data.1", new ThemeElement(null, new RGB(51, 160, 44)));
      elements.put("Chart.Data.2", new ThemeElement(null, new RGB(31, 120, 180)));
      elements.put("Chart.Data.3", new ThemeElement(null, new RGB(255, 127, 0)));
      elements.put("Chart.Data.4", new ThemeElement(null, new RGB(227, 26, 28)));
      elements.put("Chart.Data.5", new ThemeElement(null, new RGB(106, 61, 154)));
      elements.put("Chart.Data.6", new ThemeElement(null, new RGB(166, 206, 227)));
      elements.put("Chart.Data.7", new ThemeElement(null, new RGB(178, 223, 138)));
      elements.put("Chart.Data.8", new ThemeElement(null, new RGB(251, 154, 153)));
      elements.put("Chart.Data.9", new ThemeElement(null, new RGB(253, 191, 111)));
      elements.put("Chart.Data.10", new ThemeElement(null, new RGB(202, 178, 214)));
      elements.put("Chart.Data.11", new ThemeElement(null, new RGB(177, 89, 40)));
      elements.put("Chart.Data.12", new ThemeElement(null, new RGB(27, 158, 119)));
      elements.put("Chart.Data.13", new ThemeElement(null, new RGB(217, 95, 2)));
      elements.put("Chart.Data.14", new ThemeElement(null, new RGB(117, 112, 179)));
      elements.put("Chart.Data.15", new ThemeElement(null, new RGB(230, 171, 2)));
      elements.put("Chart.Data.16", new ThemeElement(null, new RGB(102, 166, 30)));
      elements.put("Chart.DialNeedlePin", new ThemeElement(new RGB(27, 30, 33), null));
      elements.put("Chart.DialScale", new ThemeElement(null, new RGB(118, 120, 122)));
      elements.put("Chart.Gauge", new ThemeElement(new RGB(242, 242, 242), new RGB(51, 160, 44)));
      elements.put("Chart.Grid", new ThemeElement(null, new RGB(232, 232, 232)));
      elements.put("Chart.PlotArea", new ThemeElement(new RGB(255, 255, 255), new RGB(22, 22, 22)));
      elements.put("Chart.Selection", new ThemeElement(new RGB(0, 0, 128), null));
      elements.put("Dashboard", new ThemeElement(new RGB(240, 240, 240), null));
      elements.put("DeviceView.Port", new ThemeElement(new RGB(224, 224, 224), null));
      elements.put("DeviceView.PortHighlight", new ThemeElement(new RGB(64, 156, 224), null));
      elements.put("JSON.Builtin", new ThemeElement(null, new RGB(127, 146, 255)));
      elements.put("JSON.Key", new ThemeElement(null, new RGB(4, 81, 165)));
      elements.put("JSON.Number", new ThemeElement(null, new RGB(9, 143, 153)));
      elements.put("JSON.String", new ThemeElement(null, new RGB(163, 21, 21)));
      elements.put("List.DisabledItem", new ThemeElement(null, new RGB(172, 172, 172)));
      elements.put("List.Error", new ThemeElement(null, new RGB(255, 0, 0)));
      elements.put("Map.Border", new ThemeElement(null, new RGB(128, 128, 128)));
      elements.put("Map.GroupBox", new ThemeElement(null, new RGB(255, 255, 255)));
      elements.put("Map.LastValues", new ThemeElement(null, new RGB(0, 64, 0)));
      elements.put("Map.ObjectTooltip", new ThemeElement(new RGB(250, 243, 187), null));
      elements.put("MessageArea.Error", new ThemeElement(new RGB(254, 221, 215), new RGB(219, 33, 0)));
      elements.put("MessageArea.Info", new ThemeElement(new RGB(227, 245, 252), new RGB(0, 114, 163)));
      elements.put("MessageArea.Success", new ThemeElement(new RGB(223, 240, 208), new RGB(60, 133, 0)));
      elements.put("MessageArea.Warning", new ThemeElement(new RGB(255, 244, 199), new RGB(254, 226, 114)));
      elements.put("MessageBar", new ThemeElement(new RGB(255, 252, 192), new RGB(0, 0, 0)));
      elements.put("MibExplorer.Header", new ThemeElement(new RGB(153, 180, 209), new RGB(255, 255, 255)));
      elements.put("ObjectTab.Header", new ThemeElement(new RGB(153, 180, 209), new RGB(255, 255, 255)));
      elements.put("ObjectTree.Maintenance", new ThemeElement(null, new RGB(96, 96, 144)));
      elements.put("Rack", new ThemeElement(new RGB(255, 255, 255), new RGB(0, 0, 0)));
      elements.put("Rack.Border", new ThemeElement(new RGB(92, 92, 92), new RGB(92, 92, 92)));
      elements.put("Rack.EmptySpace", new ThemeElement(new RGB(224, 224, 224), null));
      elements.put("RuleEditor", new ThemeElement(new RGB(255, 255, 255), new RGB(0, 0, 0)));
      elements.put("RuleEditor.Title.Disabled", new ThemeElement(new RGB(202, 227, 206), null));
      elements.put("RuleEditor.Title.Normal", new ThemeElement(new RGB(225, 233, 241), null));
      elements.put("RuleEditor.Title.Selected", new ThemeElement(new RGB(245, 249, 104), null));
      elements.put("RuleEditor.Border.Action", new ThemeElement(new RGB(186, 176, 201), null));
      elements.put("RuleEditor.Border.Condition", new ThemeElement(new RGB(198, 214, 172), null));
      elements.put("RuleEditor.Border.Rule", new ThemeElement(new RGB(153, 180, 209), null));
      elements.put("ServiceAvailability.Legend", new ThemeElement(null, new RGB(0, 0, 0)));
      elements.put("Status.Normal", new ThemeElement(new RGB(197, 255, 164), new RGB(0, 192, 0)));
      elements.put("Status.Warning", new ThemeElement(new RGB(191, 255, 255), new RGB(0, 255, 255)));
      elements.put("Status.Minor", new ThemeElement(new RGB(255, 255, 180), new RGB(231, 226, 0)));
      elements.put("Status.Major", new ThemeElement(new RGB(255, 204, 153), new RGB(255, 128, 0)));
      elements.put("Status.Critical", new ThemeElement(new RGB(255, 140, 140), new RGB(192, 0, 0)));
      elements.put("Status.Unknown", new ThemeElement(new RGB(204, 204, 255), new RGB(0, 0, 128)));
      elements.put("Status.Unmanaged", new ThemeElement(new RGB(224, 224, 224), new RGB(192, 192, 192)));
      elements.put("Status.Disabled", new ThemeElement(new RGB(200, 158, 118), new RGB(128, 64, 0)));
      elements.put("Status.Testing", new ThemeElement(new RGB(255, 230, 255), new RGB(255, 128, 255)));
      elements.put("StatusMap.Text", new ThemeElement(null, new RGB(0, 0, 0)));
      elements.put("TextInput.Error", new ThemeElement(new RGB(255, 0, 0), null));
      elements.put("Window.Header", new ThemeElement(new RGB(17, 60, 81), new RGB(192, 192, 192), "Metropolis Medium,Segoe UI,Liberation Sans,Verdana,Helvetica", 13));
      elements.put("Window.Header.Highlight", new ThemeElement(new RGB(71, 113, 134), new RGB(192, 192, 192)));
      elements.put("Window.PerspectiveSwitcher", new ThemeElement(new RGB(0, 54, 77), new RGB(240, 240, 240), "Metropolis Medium,Segoe UI,Liberation Sans,Verdana,Helvetica", 14));
   }
}
