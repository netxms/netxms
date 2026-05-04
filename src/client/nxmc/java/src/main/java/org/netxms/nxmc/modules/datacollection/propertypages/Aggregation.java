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
package org.netxms.nxmc.modules.datacollection.propertypages;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.constants.DciAggregationMode;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.DataCollectionObjectEditor;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Aggregation" property page for DCI items — controls per-DCI participation in the
 * tiered hourly/daily rollup feature (see issue #419).
 */
public class Aggregation extends AbstractDCIPropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(Aggregation.class);

   private DataCollectionItem dci;

   private Button modeDefault;
   private Button modeEnabled;
   private Button modeDisabled;

   private Button hourlyDefault;
   private Button hourlyCustom;
   private Composite hourlyValueComposite;
   private Text hourlyRetention;

   private Button dailyDefault;
   private Button dailyCustom;
   private Composite dailyValueComposite;
   private Text dailyRetention;

   /**
    * Create page.
    *
    * @param editor data collection object editor
    */
   public Aggregation(DataCollectionObjectEditor editor)
   {
      super(LocalizationHelper.getI18n(Aggregation.class).tr("Aggregation"), editor);
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      Composite dialogArea = (Composite)super.createContents(parent);
      dci = (DataCollectionItem)editor.getObject();

      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      layout.marginWidth = 0;
      layout.marginRight = WidgetHelper.OUTER_SPACING;
      layout.marginHeight = 0;
      dialogArea.setLayout(layout);

      createModeGroup(dialogArea);
      createHourlyGroup(dialogArea);
      createDailyGroup(dialogArea);
      createTsdbNotice(dialogArea);

      return dialogArea;
   }

   /**
    * Three-radio "Aggregation mode" group: Server default (inherit), Enabled, Disabled.
    */
   private void createModeGroup(Composite dialogArea)
   {
      Group group = new Group(dialogArea, SWT.NONE);
      group.setText(i18n.tr("Aggregation mode"));
      GridLayout layout = new GridLayout();
      layout.marginHeight = WidgetHelper.OUTER_SPACING;
      layout.marginWidth = WidgetHelper.OUTER_SPACING;
      group.setLayout(layout);
      group.setLayoutData(fillHorizontal());

      DciAggregationMode current = dci.getAggregationMode();

      modeDefault = new Button(group, SWT.RADIO);
      modeDefault.setText(i18n.tr("Server default (use server-wide setting)"));
      modeDefault.setSelection(current == DciAggregationMode.INHERIT);

      modeEnabled = new Button(group, SWT.RADIO);
      modeEnabled.setText(i18n.tr("Enabled"));
      modeEnabled.setSelection(current == DciAggregationMode.ENABLED);

      modeDisabled = new Button(group, SWT.RADIO);
      modeDisabled.setText(i18n.tr("Disabled"));
      modeDisabled.setSelection(current == DciAggregationMode.DISABLED);
   }

   /**
    * Hourly retention group: Server default radio + Custom radio with days input.
    */
   private void createHourlyGroup(Composite dialogArea)
   {
      Group group = new Group(dialogArea, SWT.NONE);
      group.setText(i18n.tr("Hourly aggregate retention"));
      GridLayout layout = new GridLayout(2, false);
      layout.marginHeight = WidgetHelper.OUTER_SPACING;
      layout.marginWidth = WidgetHelper.OUTER_SPACING;
      group.setLayout(layout);
      group.setLayoutData(fillHorizontal());

      SelectionListener buttons = new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            boolean custom = hourlyCustom.getSelection();
            hourlyRetention.setEnabled(custom);
            hourlyValueComposite.setVisible(custom);
            if (custom)
               hourlyRetention.setFocus();
         }
      };

      hourlyDefault = new Button(group, SWT.RADIO);
      hourlyDefault.setText(i18n.tr("Server default"));
      hourlyDefault.setSelection(dci.getHourlyRetention() <= 0);
      hourlyDefault.addSelectionListener(buttons);
      GridData gd = new GridData();
      gd.horizontalSpan = 2;
      hourlyDefault.setLayoutData(gd);

      hourlyCustom = new Button(group, SWT.RADIO);
      hourlyCustom.setText(i18n.tr("Custom"));
      hourlyCustom.setSelection(dci.getHourlyRetention() > 0);
      hourlyCustom.addSelectionListener(buttons);

      hourlyValueComposite = createDaysInput(group, dci.getHourlyRetention());
      hourlyRetention = (Text)hourlyValueComposite.getData("input");
   }

   /**
    * Daily retention group: Server default radio + Custom radio with days input.
    */
   private void createDailyGroup(Composite dialogArea)
   {
      Group group = new Group(dialogArea, SWT.NONE);
      group.setText(i18n.tr("Daily aggregate retention"));
      GridLayout layout = new GridLayout(2, false);
      layout.marginHeight = WidgetHelper.OUTER_SPACING;
      layout.marginWidth = WidgetHelper.OUTER_SPACING;
      group.setLayout(layout);
      group.setLayoutData(fillHorizontal());

      SelectionListener buttons = new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            boolean custom = dailyCustom.getSelection();
            dailyRetention.setEnabled(custom);
            dailyValueComposite.setVisible(custom);
            if (custom)
               dailyRetention.setFocus();
         }
      };

      dailyDefault = new Button(group, SWT.RADIO);
      dailyDefault.setText(i18n.tr("Server default"));
      dailyDefault.setSelection(dci.getDailyRetention() <= 0);
      dailyDefault.addSelectionListener(buttons);
      GridData gd = new GridData();
      gd.horizontalSpan = 2;
      dailyDefault.setLayoutData(gd);

      dailyCustom = new Button(group, SWT.RADIO);
      dailyCustom.setText(i18n.tr("Custom"));
      dailyCustom.setSelection(dci.getDailyRetention() > 0);
      dailyCustom.addSelectionListener(buttons);

      dailyValueComposite = createDaysInput(group, dci.getDailyRetention());
      dailyRetention = (Text)dailyValueComposite.getData("input");
   }

   /**
    * Build the "[ days ]" sub-composite used by both retention groups. Stashes the Text widget
    * via setData("input", ...) so the caller can wire it up without juggling extra refs.
    */
   private Composite createDaysInput(Composite parent, int initialValue)
   {
      Composite composite = new Composite(parent, SWT.NONE);
      GridLayout layout = new GridLayout(2, false);
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.horizontalSpacing = WidgetHelper.INNER_SPACING;
      composite.setLayout(layout);

      Text input = new Text(composite, SWT.BORDER);
      input.setText(initialValue > 0 ? Integer.toString(initialValue) : "");
      input.setEnabled(initialValue > 0);
      input.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      Label label = new Label(composite, SWT.NONE);
      label.setText(i18n.tr("days"));
      GridData gd = new GridData(SWT.LEFT, SWT.CENTER, false, false);
      gd.horizontalIndent = WidgetHelper.OUTER_SPACING;
      label.setLayoutData(gd);

      composite.setData("input", input);
      composite.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      composite.setVisible(initialValue > 0);
      return composite;
   }

   /**
    * Footer notice describing TSDB caveats. Server-side database backend is not exposed to the
    * client, so the wording is conditional ("if you are on TimescaleDB") rather than gated on
    * a runtime check.
    */
   private void createTsdbNotice(Composite dialogArea)
   {
      Label notice = new Label(dialogArea, SWT.WRAP);
      notice.setText(i18n.tr(
            "Note: on TimescaleDB backend, per-DCI retention overrides are not honored "
            + "(retention is per storage class) and \"Disabled\" is enforced as a query-time filter only."));
      GridData gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
      gd.widthHint = 480;
      notice.setLayoutData(gd);
   }

   /**
    * Reusable GridData factory for the section groups.
    */
   private static GridData fillHorizontal()
   {
      GridData gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      return gd;
   }

   /**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(final boolean isApply)
   {
      DciAggregationMode mode = DciAggregationMode.INHERIT;
      if (modeEnabled.getSelection())
         mode = DciAggregationMode.ENABLED;
      else if (modeDisabled.getSelection())
         mode = DciAggregationMode.DISABLED;
      dci.setAggregationMode(mode);

      dci.setHourlyRetention(hourlyCustom.getSelection() ? parsePositiveInt(hourlyRetention.getText()) : 0);
      dci.setDailyRetention(dailyCustom.getSelection() ? parsePositiveInt(dailyRetention.getText()) : 0);

      editor.modify();
      return true;
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performDefaults()
    */
   @Override
   protected void performDefaults()
   {
      super.performDefaults();
      modeDefault.setSelection(true);
      modeEnabled.setSelection(false);
      modeDisabled.setSelection(false);

      hourlyDefault.setSelection(true);
      hourlyCustom.setSelection(false);
      hourlyRetention.setText("");
      hourlyRetention.setEnabled(false);
      hourlyValueComposite.setVisible(false);

      dailyDefault.setSelection(true);
      dailyCustom.setSelection(false);
      dailyRetention.setText("");
      dailyRetention.setEnabled(false);
      dailyValueComposite.setVisible(false);
   }

   /**
    * Parse a positive integer; treat blank/non-numeric as 0.
    */
   private static int parsePositiveInt(String value)
   {
      if ((value == null) || value.isBlank())
         return 0;
      try
      {
         int v = Integer.parseInt(value.trim());
         return (v > 0) ? v : 0;
      }
      catch(NumberFormatException e)
      {
         return 0;
      }
   }
}
