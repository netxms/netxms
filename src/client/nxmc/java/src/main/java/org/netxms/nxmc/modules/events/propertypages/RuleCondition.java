/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.events.widgets.RuleEditor;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Condition" property page for EPP rule
 */
public class RuleCondition extends RuleBasePropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(RuleCondition.class);

	private Button checkDisabled;
   private Button checkAcceptCorrelatedEvents;
	
   /**
    * Create property page.
    *
    * @param editor rule editor
    */
   public RuleCondition(RuleEditor editor)
   {
      super(editor, LocalizationHelper.getI18n(RuleCondition.class).tr("Condition"));
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
		Composite dialogArea = new Composite(parent, SWT.NONE);
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING * 2;
      dialogArea.setLayout(layout);

      checkDisabled = new Button(dialogArea, SWT.CHECK);
      checkDisabled.setText(i18n.tr("Rule is &disabled"));
      checkDisabled.setSelection((rule.getFlags() & EventProcessingPolicyRule.DISABLED) != 0);

      checkAcceptCorrelatedEvents = new Button(dialogArea, SWT.CHECK);
      checkAcceptCorrelatedEvents.setText(i18n.tr("Accept &correlated events"));
      checkAcceptCorrelatedEvents.setSelection((rule.getFlags() & EventProcessingPolicyRule.ACCEPT_CORRELATED) != 0);

		return dialogArea;
	}

   /**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(final boolean isApply)
	{
		if (checkDisabled.getSelection())
			rule.setFlags(rule.getFlags() | EventProcessingPolicyRule.DISABLED);
		else
			rule.setFlags(rule.getFlags() & ~EventProcessingPolicyRule.DISABLED);

      if (checkAcceptCorrelatedEvents.getSelection())
         rule.setFlags(rule.getFlags() | EventProcessingPolicyRule.ACCEPT_CORRELATED);
      else
         rule.setFlags(rule.getFlags() & ~EventProcessingPolicyRule.ACCEPT_CORRELATED);

      editor.setModified(true);
		return true;
	}
}
