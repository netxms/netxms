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
package org.netxms.nxmc.modules.objects.preferencepages;

import org.eclipse.jface.preference.BooleanFieldEditor;
import org.eclipse.jface.preference.FieldEditorPreferencePage;
import org.eclipse.jface.preference.StringFieldEditor;
import org.eclipse.jface.util.PropertyChangeEvent;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Group;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.base.editors.TimePeriodEditor;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Preferences page for object display
 */
public class ObjectsPreferences extends FieldEditorPreferencePage
{
   private final I18n i18n = LocalizationHelper.getI18n(ObjectsPreferences.class);

   private BooleanFieldEditor useServerFilter;
   private Composite autoApplyParent;
   private Composite delayParent;
   private Composite minLengthParent;
   private BooleanFieldEditor autoApply;
   private StringFieldEditor delay;
   private StringFieldEditor minLength;

   public ObjectsPreferences()
   {
      super(LocalizationHelper.getI18n(ObjectsPreferences.class).tr("Objects"), FieldEditorPreferencePage.FLAT);
      setPreferenceStore(PreferenceStore.getInstance());
   }

   /**
    * @see org.eclipse.jface.preference.FieldEditorPreferencePage#createFieldEditors()
    */
   @Override
   protected void createFieldEditors()
   {
      addField(new BooleanFieldEditor("ObjectsFullSync", i18n.tr("&Full object synchronization on startup"), getFieldEditorParent()));
      addField(new BooleanFieldEditor("CustomAttributes.ShowHidden", "Show &hidden custom attributes", getFieldEditorParent()));

      Group filterGroup = new Group(getFieldEditorParent(), SWT.NONE);
      filterGroup.setText(i18n.tr("Filter"));
      filterGroup.getParent().setLayout(new FillLayout());
      filterGroup.setLayout(new GridLayout());

      useServerFilter = new BooleanFieldEditor("ObjectBrowser.useServerFilterSettings", i18n.tr("&Use server settings for object filter"), getSubFieldEditorParent(filterGroup));
      addField(useServerFilter);

      autoApplyParent = getSubFieldEditorParent(filterGroup);
      autoApply = new BooleanFieldEditor("ObjectBrowser.filterAutoApply", i18n.tr("&Apply filter automatically"), autoApplyParent);
      addField(autoApply);

      delayParent = getSubFieldEditorParent(filterGroup);
      delay = new StringFieldEditor("ObjectBrowser.filterDelay", i18n.tr("Filter &delay"), delayParent);
      addField(delay);
      delay.setEmptyStringAllowed(false);
      delay.setTextLimit(5);

      minLengthParent = getSubFieldEditorParent(filterGroup);
      minLength = new StringFieldEditor("ObjectBrowser.filterMinLength", i18n.tr("Filter &minimal length"), minLengthParent);
      addField(minLength);
      minLength.setEmptyStringAllowed(false);
      minLength.setTextLimit(3);

      addField(new TimePeriodEditor("Maintenance.TimeEditor", i18n.tr("Predefined maintenance periods"), getFieldEditorParent(), "Maintenance.TimeMenuSize", "Maintenance.TimeMenuEntry."));
   }

   private Composite getSubFieldEditorParent(Group group)
   {
      Composite c = new Composite(group, SWT.NONE);
      c.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false));
      return c;
   }

   /**
    * @see org.eclipse.jface.preference.FieldEditorPreferencePage#initialize()
    */
   @Override
   protected void initialize()
   {
      super.initialize();
      boolean enabled = !useServerFilter.getBooleanValue();
      autoApply.setEnabled(enabled, autoApplyParent);
      delay.setEnabled(enabled, delayParent);
      minLength.setEnabled(enabled, minLengthParent);
   }

   /**
    * @see org.eclipse.jface.preference.FieldEditorPreferencePage#propertyChange(org.eclipse.jface.util.PropertyChangeEvent)
    */
   @Override
   public void propertyChange(PropertyChangeEvent event)
   {
      super.propertyChange(event);
      boolean enabled = !useServerFilter.getBooleanValue();
      autoApply.setEnabled(enabled, autoApplyParent);
      delay.setEnabled(enabled, delayParent);
      minLength.setEnabled(enabled, minLengthParent);
   }
}
