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
 * "Action" property page for EPP rule
 */
public class RuleAction extends RuleBasePropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(RuleAction.class);

	private Button checkStopProcessing;

   /**
    * Create property page.
    *
    * @param editor rule editor
    */
   public RuleAction(RuleEditor editor)
   {
      super(editor, LocalizationHelper.getI18n(RuleAction.class).tr("Action"));
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
      
      checkStopProcessing = new Button(dialogArea, SWT.CHECK);
      checkStopProcessing.setText(i18n.tr("&Stop event processing"));
      checkStopProcessing.setSelection((rule.getFlags() & EventProcessingPolicyRule.STOP_PROCESSING) != 0);

		return dialogArea;
	}

   /**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(final boolean isApply)
	{
		if (checkStopProcessing.getSelection())
			rule.setFlags(rule.getFlags() | EventProcessingPolicyRule.STOP_PROCESSING);
		else
			rule.setFlags(rule.getFlags() & ~EventProcessingPolicyRule.STOP_PROCESSING);
		editor.setModified(true);
		return true;
	}
}
