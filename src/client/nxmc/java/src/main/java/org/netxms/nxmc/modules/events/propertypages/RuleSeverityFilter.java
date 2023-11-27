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
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.events.widgets.RuleEditor;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.netxms.nxmc.tools.WidgetHelper;

/**
 * "Severity Filter" property page
 */
public class RuleSeverityFilter extends RuleBasePropertyPage
{
	private static final int[] severityFlag = { 
	   EventProcessingPolicyRule.SEVERITY_NORMAL,
	   EventProcessingPolicyRule.SEVERITY_WARNING,
		EventProcessingPolicyRule.SEVERITY_MINOR,
		EventProcessingPolicyRule.SEVERITY_MAJOR,
		EventProcessingPolicyRule.SEVERITY_CRITICAL
	};

	private Button[] checkButton = new Button[5];

   /**
    * Create property page.
    *
    * @param editor rule editor
    */
   public RuleSeverityFilter(RuleEditor editor)
   {
      super(editor, LocalizationHelper.getI18n(RuleSeverityFilter.class).tr("Severity Filter"));
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		RowLayout layout = new RowLayout();
		layout.type = SWT.VERTICAL;
		layout.spacing = WidgetHelper.OUTER_SPACING;
		dialogArea.setLayout(layout);

		for(int i = 0; i < checkButton.length; i++)
		{
			checkButton[i] = new Button(dialogArea, SWT.CHECK);
			checkButton[i].setText(StatusDisplayInfo.getStatusText(i));
			checkButton[i].setSelection((rule.getFlags() & severityFlag[i]) != 0);
		}
		
		return dialogArea;
	}

   /**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(final boolean isApply)
	{
		for(int i = 0; i < checkButton.length; i++)
		{
			if (checkButton[i].getSelection())
			{
				rule.setFlags(rule.getFlags() | severityFlag[i]);
			}
			else
			{
				rule.setFlags(rule.getFlags() & ~severityFlag[i]);
			}
		}
		editor.setModified(true);
      return true;
	}
}
