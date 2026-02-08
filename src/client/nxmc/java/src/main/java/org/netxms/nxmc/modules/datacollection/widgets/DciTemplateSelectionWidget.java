/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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
package org.netxms.nxmc.modules.datacollection.widgets;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Group;
import org.netxms.client.datacollection.DciTemplateConfig;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Composite widget for DCI template selection with matching options.
 */
public class DciTemplateSelectionWidget extends Composite
{
   private final I18n i18n = LocalizationHelper.getI18n(DciTemplateSelectionWidget.class);

   private TemplateDciSelector dciNameSelector;
   private TemplateDciSelector dciDescriptionSelector;
   private TemplateDciSelector dciTagSelector;
   private Button checkRegexMatch;
   private Button checkMultiMatch;
   private List<ModifyListener> modifyListeners = new ArrayList<>();

   /**
    * Create DCI template selection widget.
    *
    * @param parent parent composite
    * @param style SWT style
    */
   public DciTemplateSelectionWidget(Composite parent, int style)
   {
      super(parent, style);

      GridLayout layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      setLayout(layout);

      Group optionsGroup = new Group(this, SWT.NONE);
      optionsGroup.setText(i18n.tr("Matching options"));
      optionsGroup.setLayout(new GridLayout());
      optionsGroup.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      dciNameSelector = new TemplateDciSelector(optionsGroup, SWT.NONE);
      dciNameSelector.setLabel(i18n.tr("Metric"));
      dciNameSelector.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      dciNameSelector.addModifyListener(this::fireModifyListeners);

      dciDescriptionSelector = new TemplateDciSelector(optionsGroup, SWT.NONE);
      dciDescriptionSelector.setLabel(i18n.tr("DCI display name"));
      dciDescriptionSelector.setField(TemplateDciSelector.Field.DESCRIPTION);
      dciDescriptionSelector.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      dciDescriptionSelector.addModifyListener(this::fireModifyListeners);

      dciTagSelector = new TemplateDciSelector(optionsGroup, SWT.NONE);
      dciTagSelector.setLabel(i18n.tr("DCI tag"));
      dciTagSelector.setField(TemplateDciSelector.Field.TAG);
      dciTagSelector.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      dciTagSelector.addModifyListener(this::fireModifyListeners);

      checkRegexMatch = new Button(optionsGroup, SWT.CHECK);
      checkRegexMatch.setText(i18n.tr("Use regular expressions for DCI matching"));

      checkMultiMatch = new Button(optionsGroup, SWT.CHECK);
      checkMultiMatch.setText(i18n.tr("Multiple match"));
      checkMultiMatch.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
   }

   /**
    * Load configuration from DciTemplateConfig.
    *
    * @param config configuration to load
    */
   public void setConfig(DciTemplateConfig config)
   {
      dciNameSelector.setText(config.getDciName());
      dciDescriptionSelector.setText(config.getDciDescription());
      dciTagSelector.setText(config.getDciTag());
      checkRegexMatch.setSelection(config.isRegexMatch());
      checkMultiMatch.setSelection(config.isMultiMatch());
   }

   /**
    * Get DciTemplateConfig populated with current widget values.
    *
    * @return DciTemplateConfig with current values
    */
   public DciTemplateConfig getConfig()
   {
      DciTemplateConfig config = new DciTemplateConfig();
      config.setDciName(dciNameSelector.getText());
      config.setDciDescription(dciDescriptionSelector.getText());
      config.setDciTag(dciTagSelector.getText());
      config.setRegexMatch(checkRegexMatch.getSelection());
      config.setMultiMatch(checkMultiMatch.getSelection());
      return config;
   }

   /**
    * Set visibility of the multi-match option.
    *
    * @param visible true to show multi-match option, false to hide
    */
   public void setMultiMatchVisible(boolean visible)
   {
      checkMultiMatch.setVisible(visible);
      ((GridData)checkMultiMatch.getLayoutData()).exclude = !visible;
   }

   /**
    * Add modify listener.
    *
    * @param listener listener to add
    */
   public void addModifyListener(ModifyListener listener)
   {
      modifyListeners.add(listener);
   }

   /**
    * Remove modify listener.
    *
    * @param listener listener to remove
    */
   public void removeModifyListener(ModifyListener listener)
   {
      modifyListeners.remove(listener);
   }

   /**
    * Fire modify listeners.
    *
    * @param e modify event from child widget
    */
   private void fireModifyListeners(ModifyEvent e)
   {
      Event event = new Event();
      event.widget = this;
      ModifyEvent me = new ModifyEvent(event);
      for(ModifyListener listener : modifyListeners)
      {
         listener.modifyText(me);
      }
   }
}
