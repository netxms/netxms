/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.console.resources;

import java.util.HashMap;
import java.util.Map;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Listener;
import org.netxms.ui.eclipse.console.ColorManager;
import org.netxms.ui.eclipse.tools.ColorCache;

/**
 * Shared colors
 */
public class SharedColors
{
	public static final String BORDER = "Border";  //$NON-NLS-1$
	public static final String CGROUP_BORDER = "CGroup.Border";  //$NON-NLS-1$
	public static final String CGROUP_TITLE = "CGroup.Title";  //$NON-NLS-1$
	public static final String COMMAND_BOX_TEXT = "CommandBox.Text";  //$NON-NLS-1$
	public static final String COMMAND_BOX_BACKGROUND = "CommandBox.Background";  //$NON-NLS-1$
	public static final String DASHBOARD_BACKGROUND = "Dashboard.Background";  //$NON-NLS-1$
	public static final String DIAL_CHART_LEGEND = "DialChart.Legend";  //$NON-NLS-1$
	public static final String DIAL_CHART_NEEDLE = "DialChart.Needle"; //$NON-NLS-1$
	public static final String DIAL_CHART_NEEDLE_PIN = "DialChart.NeedlePin"; //$NON-NLS-1$
	public static final String DIAL_CHART_SCALE = "DialChart.Scale";  //$NON-NLS-1$
	public static final String DIAL_CHART_VALUE = "DialChart.Value";  //$NON-NLS-1$
	public static final String DIAL_CHART_VALUE_BACKGROUND = "DialChart.ValueBackground";  //$NON-NLS-1$
	public static final String ERROR_BACKGROUND = "ErrorBackground";  //$NON-NLS-1$
	public static final String GEOMAP_TITLE = "GeoMap.Title";  //$NON-NLS-1$
	public static final String MAP_GROUP_BOX_TITLE = "Map.GroupBox.Title";  //$NON-NLS-1$
	public static final String MAP_LAST_VALUES_TEXT = "Map.LastValues.Text";  //$NON-NLS-1$
	public static final String MAP_SMALL_LABEL_BACKGROUND = "Map.ObjectFigure.SmallLabel.Background";  //$NON-NLS-1$
	public static final String MIB_EXPLORER_HEADER_BACKGROUND = "MibExplorer.Header.Background";  //$NON-NLS-1$
	public static final String MIB_EXPLORER_HEADER_TEXT = "MibExplorer.Header.Text";  //$NON-NLS-1$
	public static final String OBJECT_TAB_BACKGROUND = "ObjectTab.Background";  //$NON-NLS-1$
	public static final String OBJECT_TAB_HEADER = "ObjectTab.Header";  //$NON-NLS-1$
	public static final String OBJECT_TAB_HEADER_BACKGROUND = "ObjectTab.Header.Background"; //$NON-NLS-1$
	public static final String RULE_EDITOR_BACKGROUND = "RuleEditor.Background"; //$NON-NLS-1$
	public static final String RULE_EDITOR_NORMAL_TITLE_BACKGROUND = "RuleEditor.Title.Normal.Background"; //$NON-NLS-1$
	public static final String RULE_EDITOR_DISABLED_TITLE_BACKGROUND = "RuleEditor.Title.Disabled.Background"; //$NON-NLS-1$
	public static final String RULE_EDITOR_SELECTED_TITLE_BACKGROUND = "RuleEditor.Title.Selected.Background"; //$NON-NLS-1$
	public static final String RULE_EDITOR_RULE_BORDER = "RuleEditor.Border.Rule"; //$NON-NLS-1$
	public static final String RULE_EDITOR_CONDITION_BORDER = "RuleEditor.Border.Condition"; //$NON-NLS-1$
	public static final String RULE_EDITOR_ACTION_BORDER = "RuleEditor.Border.Action"; //$NON-NLS-1$
	public static final String RULE_EDITOR_TITLE_TEXT = "RuleEditor.Title.Text"; //$NON-NLS-1$
	public static final String SERVICE_AVAILABILITY_LEGEND = "ServiceAvailability.Legend";  //$NON-NLS-1$
	public static final String STATUS_NORMAL = "Status.Normal";  //$NON-NLS-1$
	public static final String STATUS_WARNING = "Status.Warning";  //$NON-NLS-1$
	public static final String STATUS_MINOR = "Status.Minor";  //$NON-NLS-1$
	public static final String STATUS_MAJOR = "Status.Major";  //$NON-NLS-1$
	public static final String STATUS_CRITICAL = "Status.Critical";  //$NON-NLS-1$
	public static final String STATUS_UNKNOWN = "Status.Unknown";  //$NON-NLS-1$
	public static final String STATUS_UNMANAGED = "Status.Unmanaged";  //$NON-NLS-1$
	public static final String STATUS_DISABLED = "Status.Disabled";  //$NON-NLS-1$
	public static final String STATUS_TESTING = "Status.Testing";  //$NON-NLS-1$
	public static final String TEXT_ERROR = "Text.Error";  //$NON-NLS-1$
	public static final String TEXT_NORMAL = "Text.Normal";  //$NON-NLS-1$
	
	/**
	 * Default color to be used when no named default found
	 */
	private static final RGB DEFAULT_COLOR = new RGB(0,0,0);
	
	private static final Map<Display, SharedColors> instances = new HashMap<Display, SharedColors>();
	private static final Map<String, RGB> defaultColors = new HashMap<String, RGB>();
	
	static
	{
		defaultColors.put(BORDER, new RGB(153, 180, 209));
		defaultColors.put(CGROUP_BORDER, new RGB(153, 180, 209));
		defaultColors.put(CGROUP_TITLE, new RGB(0, 0, 0));
		defaultColors.put(COMMAND_BOX_TEXT, new RGB(0, 0, 96));
		defaultColors.put(COMMAND_BOX_BACKGROUND, new RGB(255, 255, 255));
		defaultColors.put(DASHBOARD_BACKGROUND, new RGB(255, 255, 255));
		defaultColors.put(DIAL_CHART_LEGEND, new RGB(0, 0, 0));
		defaultColors.put(DIAL_CHART_NEEDLE, new RGB(51, 78, 113));
		defaultColors.put(DIAL_CHART_NEEDLE_PIN, new RGB(239, 228, 176));
		defaultColors.put(DIAL_CHART_SCALE, new RGB(0, 0, 0));
		defaultColors.put(DIAL_CHART_VALUE, new RGB(255, 255, 255));
		defaultColors.put(DIAL_CHART_VALUE_BACKGROUND, new RGB(51, 78, 113));
		defaultColors.put(ERROR_BACKGROUND, new RGB(255, 0, 0));
		defaultColors.put(GEOMAP_TITLE, new RGB(0, 0, 0));
		defaultColors.put(MAP_GROUP_BOX_TITLE, new RGB(255, 255, 255));
		defaultColors.put(MAP_LAST_VALUES_TEXT, new RGB(0, 64, 0));
		defaultColors.put(MAP_SMALL_LABEL_BACKGROUND, new RGB(255, 255, 255));
		defaultColors.put(MIB_EXPLORER_HEADER_BACKGROUND, new RGB(153, 180, 209));
		defaultColors.put(MIB_EXPLORER_HEADER_TEXT, new RGB(255, 255, 255));
		defaultColors.put(OBJECT_TAB_BACKGROUND, new RGB(255, 255, 255));
		defaultColors.put(OBJECT_TAB_HEADER, new RGB(255, 255, 255));
		defaultColors.put(OBJECT_TAB_HEADER_BACKGROUND, new RGB(153, 180, 209));
		defaultColors.put(RULE_EDITOR_BACKGROUND, new RGB(255, 255, 255));
		defaultColors.put(RULE_EDITOR_NORMAL_TITLE_BACKGROUND, new RGB(225, 233, 241));
		defaultColors.put(RULE_EDITOR_DISABLED_TITLE_BACKGROUND, new RGB(202, 227, 206));
		defaultColors.put(RULE_EDITOR_SELECTED_TITLE_BACKGROUND, new RGB(245, 249, 104));
		defaultColors.put(RULE_EDITOR_RULE_BORDER, new RGB(153, 180, 209));
		defaultColors.put(RULE_EDITOR_CONDITION_BORDER, new RGB(198,214,172));
		defaultColors.put(RULE_EDITOR_ACTION_BORDER, new RGB(186,176,201));
		defaultColors.put(RULE_EDITOR_TITLE_TEXT, new RGB(0, 0, 0));
		defaultColors.put(SERVICE_AVAILABILITY_LEGEND, new RGB(0, 0, 0));
		defaultColors.put(STATUS_NORMAL, new RGB(0, 192, 0));
		defaultColors.put(STATUS_WARNING, new RGB(0, 255, 255));
		defaultColors.put(STATUS_MINOR, new RGB(231, 226, 0));
		defaultColors.put(STATUS_MAJOR, new RGB(255, 128, 0));
		defaultColors.put(STATUS_CRITICAL, new RGB(192, 0, 0));
		defaultColors.put(STATUS_UNKNOWN, new RGB(0, 0, 128));
		defaultColors.put(STATUS_UNMANAGED, new RGB(192, 192, 192));
		defaultColors.put(STATUS_DISABLED, new RGB(128, 64, 0));
		defaultColors.put(STATUS_TESTING, new RGB(255, 128, 255));		
		defaultColors.put(TEXT_ERROR, new RGB(255, 0, 0));
		defaultColors.put(TEXT_NORMAL, new RGB(0, 0, 0));
	}
	
	/**
	 * Get shared colors instance for given display
	 * 
	 * @param display
	 * @return
	 */
	private static SharedColors getInstance(Display display)
	{
		SharedColors instance;
		synchronized(instances)
		{
			instance = instances.get(display);
			if (instance == null)
			{
				instance = new SharedColors(display);
				instances.put(display, instance);
			}
		}
		return instance;
	}
	
	private ColorCache colors;
	
	/**
	 * @param display
	 */
	public SharedColors(final Display display)
	{
		colors = new ColorCache();
		display.addListener(SWT.Dispose, new Listener() {
			@Override
			public void handleEvent(Event event)
			{
				synchronized(instances)
				{
					instances.remove(display);
				}
				colors.dispose();
			}
		});
	}
	
	/**
	 * @param name
	 * @param defaultValue
	 * @param display
	 * @return
	 */
	public static Color getColor(String name, Display display)
	{
		RGB rgb = ColorManager.getInstance().getColor(name);
		if (rgb == null)
		{
			rgb = defaultColors.get(name);
			if (rgb == null)
				rgb = DEFAULT_COLOR;
		}
		return getInstance(display).colors.create(rgb);
	}
}
