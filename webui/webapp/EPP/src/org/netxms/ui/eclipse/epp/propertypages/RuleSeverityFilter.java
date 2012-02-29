/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.epp.propertypages;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.epp.widgets.RuleEditor;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * "Severity Filter" property page
 *
 */
public class RuleSeverityFilter extends PropertyPage
{
	private static final long serialVersionUID = 1L;
	private static final int[] severityFlag = { EventProcessingPolicyRule.SEVERITY_NORMAL, EventProcessingPolicyRule.SEVERITY_WARNING,
		EventProcessingPolicyRule.SEVERITY_MINOR, EventProcessingPolicyRule.SEVERITY_MAJOR, EventProcessingPolicyRule.SEVERITY_CRITICAL
	};
		
	private RuleEditor editor;
	private EventProcessingPolicyRule rule;
	private Button[] checkButton = new Button[5];
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		editor = (RuleEditor)getElement().getAdapter(RuleEditor.class);
		rule = editor.getRule();
		
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
	 * Perform apply operation
	 */
	private void doApply()
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
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performApply()
	 */
	@Override
	protected void performApply()
	{
		doApply();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performDefaults()
	 */
	@Override
	protected void performDefaults()
	{
		for(int i = 0; i < checkButton.length; i++)
			checkButton[i].setSelection(true);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performOk()
	 */
	@Override
	public boolean performOk()
	{
		doApply();
		return super.performOk();
	}
}
