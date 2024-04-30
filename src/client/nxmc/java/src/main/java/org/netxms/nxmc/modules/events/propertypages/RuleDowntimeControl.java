/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.events.propertypages;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.nxmc.base.widgets.LabeledCombo;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.events.widgets.RuleEditor;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Downtime Control" property page for EPP rule
 */
public class RuleDowntimeControl extends RuleBasePropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(RuleDowntimeControl.class);

   private LabeledCombo mode;
   private LabeledText tag;

   /**
    * Create property page.
    *
    * @param editor rule editor
    */
   public RuleDowntimeControl(RuleEditor editor)
   {
      super(editor, LocalizationHelper.getI18n(RuleDowntimeControl.class).tr("Downtime Control"));
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
		Composite dialogArea = new Composite(parent, SWT.NONE);
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
      dialogArea.setLayout(layout);

      mode = new LabeledCombo(dialogArea, SWT.NONE);
      mode.setLabel(i18n.tr("Control mode"));
      mode.add(i18n.tr("Do nothing"));
      mode.add(i18n.tr("Start downtime"));
      mode.add(i18n.tr("End downtime"));
      if ((rule.getFlags() & EventProcessingPolicyRule.START_DOWNTIME) != 0)
         mode.select(1);
      else if ((rule.getFlags() & EventProcessingPolicyRule.END_DOWNTIME) != 0)
         mode.select(2);
      else
         mode.select(0);
      mode.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      tag = new LabeledText(dialogArea, SWT.NONE);
      tag.setLabel("Downtime tag");
      tag.setTextLimit(15);
      tag.setText(rule.getDowntimeTag());
      tag.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

		return dialogArea;
	}

   /**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(final boolean isApply)
	{
      int flags = rule.getFlags();
      switch(mode.getSelectionIndex())
      {
         case 1:
            flags |= EventProcessingPolicyRule.START_DOWNTIME;
            flags &= ~EventProcessingPolicyRule.END_DOWNTIME;
            break;
         case 2:
            flags |= EventProcessingPolicyRule.END_DOWNTIME;
            flags &= ~EventProcessingPolicyRule.START_DOWNTIME;
            break;
         default:
            flags &= ~(EventProcessingPolicyRule.START_DOWNTIME | EventProcessingPolicyRule.END_DOWNTIME);
            break;
      }
      rule.setFlags(flags);
      rule.setDowntimeTag(tag.getText().trim());
		editor.setModified(true);
      return true;
	}
}
