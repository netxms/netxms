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
package org.netxms.nxmc.modules.events.propertypages;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.nxmc.base.widgets.LabeledCombo;
import org.netxms.nxmc.base.widgets.LabeledSpinner;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.events.widgets.RuleEditor;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Incident" property page for EPP rule
 */
public class RuleIncident extends RuleBasePropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(RuleIncident.class);

   private Button checkCreateIncident;
   private Button checkAIAnalysis;
   private Composite incidentOptionsGroup;
   private LabeledSpinner incidentDelay;
   private LabeledText incidentTitle;
   private LabeledText incidentDescription;
   private Composite incidentAIAnalysisGroup;
   private LabeledCombo incidentAIAnalysysDepth;
   private Button incidentAIAutoAssign;
   private LabeledText insidentAIPrompt;

   /**
    * Create property page.
    *
    * @param editor rule editor
    */
   public RuleIncident(RuleEditor editor)
   {
      super(editor, LocalizationHelper.getI18n(RuleIncident.class).tr("Incident"));
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

      checkCreateIncident = new Button(dialogArea, SWT.CHECK);
      checkCreateIncident.setText(i18n.tr("Create incident on alarm creation"));
      checkCreateIncident.setSelection((rule.getFlags() & EventProcessingPolicyRule.CREATE_INCIDENT) != 0);
      checkCreateIncident.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            boolean enabled = checkCreateIncident.getSelection();
            checkAIAnalysis.setEnabled(enabled);
            incidentOptionsGroup.setVisible(enabled);
            ((GridData)incidentOptionsGroup.getLayoutData()).exclude = !enabled;
            dialogArea.layout(true, true);
         }
      });

      checkAIAnalysis = new Button(dialogArea, SWT.CHECK);
      checkAIAnalysis.setText(i18n.tr("Perform AI-based incident analysis"));
      checkAIAnalysis.setSelection((rule.getFlags() & EventProcessingPolicyRule.AI_ANALYZE_INCIDENT) != 0);
      checkAIAnalysis.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            boolean enabled = checkAIAnalysis.getSelection() && checkCreateIncident.getSelection();
            incidentAIAnalysisGroup.setVisible(enabled);
            ((GridData)incidentAIAnalysisGroup.getLayoutData()).exclude = !enabled;
            dialogArea.layout(true, true);
         }
      });

      incidentOptionsGroup = new Composite(dialogArea, SWT.NONE);
      GridData gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.TOP;
      gd.exclude = (rule.getFlags() & EventProcessingPolicyRule.CREATE_INCIDENT) == 0;
      incidentOptionsGroup.setLayoutData(gd);
      incidentOptionsGroup.setVisible((rule.getFlags() & EventProcessingPolicyRule.CREATE_INCIDENT) != 0);
      layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      incidentOptionsGroup.setLayout(layout);

      incidentDelay = new LabeledSpinner(incidentOptionsGroup, SWT.NONE);
      incidentDelay.setLabel(i18n.tr("Creation delay (seconds, 0 for immediate)"));
      incidentDelay.setRange(0, 99999);
      incidentDelay.setSelection(rule.getIncidentDelay());

      incidentTitle = new LabeledText(incidentOptionsGroup, SWT.NONE);
      incidentTitle.setLabel(i18n.tr("Title (leave empty to use alarm message)"));
      incidentTitle.setText(rule.getIncidentTitle());
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      incidentTitle.setLayoutData(gd);

      incidentDescription = new LabeledText(incidentOptionsGroup, SWT.NONE, SWT.MULTI | SWT.BORDER | SWT.V_SCROLL);
      incidentDescription.setLabel(i18n.tr("Description"));
      incidentDescription.setText(rule.getIncidentDescription());
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.heightHint = 100;
      incidentDescription.setLayoutData(gd);

      incidentAIAnalysisGroup = new Composite(dialogArea, SWT.NONE);
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.TOP;
      gd.exclude = (rule.getFlags() & EventProcessingPolicyRule.CREATE_INCIDENT) == 0;
      incidentAIAnalysisGroup.setLayoutData(gd);
      incidentAIAnalysisGroup.setVisible(
            (rule.getFlags() & (EventProcessingPolicyRule.CREATE_INCIDENT | EventProcessingPolicyRule.AI_ANALYZE_INCIDENT)) == (EventProcessingPolicyRule.CREATE_INCIDENT | EventProcessingPolicyRule.AI_ANALYZE_INCIDENT));
      layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      incidentAIAnalysisGroup.setLayout(layout);

      incidentAIAnalysysDepth = new LabeledCombo(incidentAIAnalysisGroup, SWT.NONE);
      incidentAIAnalysysDepth.setLabel(i18n.tr("AI Analysis Depth"));
      incidentAIAnalysysDepth.add(i18n.tr("Quick"));
      incidentAIAnalysysDepth.add(i18n.tr("Standard"));
      incidentAIAnalysysDepth.add(i18n.tr("Deep"));
      incidentAIAnalysysDepth.select(rule.getIncidentAIAnalysisDepth());
      
      incidentAIAutoAssign = new Button(incidentAIAnalysisGroup, SWT.CHECK);
      incidentAIAutoAssign.setText(i18n.tr("Automatically assign incident to suggested owner"));
      incidentAIAutoAssign.setSelection((rule.getFlags() & EventProcessingPolicyRule.AI_AUTO_ASSIGN) != 0);

      insidentAIPrompt = new LabeledText(incidentAIAnalysisGroup, SWT.NONE, SWT.MULTI | SWT.BORDER | SWT.V_SCROLL);
      insidentAIPrompt.setLabel(i18n.tr("AI Prompt for Analysis (optional)"));
      insidentAIPrompt.setText(rule.getIncidentAIPrompt());
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.heightHint = 100;
      insidentAIPrompt.setLayoutData(gd);

      return dialogArea;
   }

   /**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(final boolean isApply)
   {
      if (checkCreateIncident.getSelection())
      {
         rule.setIncidentDelay(incidentDelay.getSelection());
         rule.setIncidentTitle(incidentTitle.getText().trim());
         rule.setIncidentDescription(incidentDescription.getText());
         rule.setFlags(rule.getFlags() | EventProcessingPolicyRule.CREATE_INCIDENT);
         if (checkAIAnalysis.getSelection())
         {
            rule.setIncidentAIAnalysisDepth(incidentAIAnalysysDepth.getSelectionIndex());
            rule.setIncidentAIPrompt(insidentAIPrompt.getText());
            if (incidentAIAutoAssign.getSelection())
               rule.setFlags(rule.getFlags() | EventProcessingPolicyRule.AI_ANALYZE_INCIDENT | EventProcessingPolicyRule.AI_AUTO_ASSIGN);
            else
               rule.setFlags((rule.getFlags() & ~EventProcessingPolicyRule.AI_AUTO_ASSIGN) | EventProcessingPolicyRule.AI_ANALYZE_INCIDENT);
         }
         else
         {
            rule.setFlags(rule.getFlags() & ~(EventProcessingPolicyRule.AI_ANALYZE_INCIDENT | EventProcessingPolicyRule.AI_AUTO_ASSIGN));
         }
      }
      else
      {
         rule.setFlags(rule.getFlags() & ~EventProcessingPolicyRule.CREATE_INCIDENT);
      }
      editor.setModified(true);
      return true;
   }
}
