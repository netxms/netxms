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
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.events.EventProcessingPolicyRule;
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

   private Button selectionDoNothing;
   private Button selectionStartDowntime;
   private Button selectionEndDowntime;
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
      
      Composite radioGroup = new Composite(dialogArea, SWT.NONE);
      RowLayout rowLayout = new RowLayout(SWT.VERTICAL);
      rowLayout.marginBottom = 0;
      rowLayout.marginTop = 0;
      rowLayout.marginLeft = 0;
      rowLayout.marginRight = 0;
      radioGroup.setLayout(rowLayout);
      radioGroup.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      selectionDoNothing = new Button(radioGroup, SWT.RADIO);
      selectionDoNothing.setText(i18n.tr("Do nothing"));
      
      selectionStartDowntime = new Button(radioGroup, SWT.RADIO);
      selectionStartDowntime.setText(i18n.tr("Start downtime"));
      
      selectionEndDowntime = new Button(radioGroup, SWT.RADIO);
      selectionEndDowntime.setText(i18n.tr("End downtime"));
      
      if ((rule.getFlags() & EventProcessingPolicyRule.START_DOWNTIME) != 0)
         selectionStartDowntime.setSelection(true);
      else if ((rule.getFlags() & EventProcessingPolicyRule.END_DOWNTIME) != 0)
         selectionEndDowntime.setSelection(true);
      else
         selectionDoNothing.setSelection(true);
      
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
      if (selectionEndDowntime.getSelection())
      {
         flags |= EventProcessingPolicyRule.END_DOWNTIME;
         flags &= ~EventProcessingPolicyRule.START_DOWNTIME; 
      }
      else if (selectionStartDowntime.getSelection())
      {
         flags |= EventProcessingPolicyRule.START_DOWNTIME;
         flags &= ~EventProcessingPolicyRule.END_DOWNTIME;         
      }
      else 
      {  
         flags &= ~(EventProcessingPolicyRule.START_DOWNTIME | EventProcessingPolicyRule.END_DOWNTIME);      
      }
      rule.setFlags(flags);
      rule.setDowntimeTag(tag.getText().trim());
		editor.setModified(true);
      return true;
	}
}
