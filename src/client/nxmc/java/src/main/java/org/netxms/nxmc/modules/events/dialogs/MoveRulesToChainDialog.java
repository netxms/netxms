/**
 * NetXMS - open source network management system
 * Copyright (C) 2026 Raden Solutions
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
package org.netxms.nxmc.modules.events.dialogs;

import java.util.List;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Table;
import org.eclipse.swt.widgets.TableColumn;
import org.eclipse.swt.widgets.TableItem;
import org.netxms.client.events.EventProcessingPolicyChain;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog for selecting the chain to move event processing policy rules to
 */
public class MoveRulesToChainDialog extends Dialog
{
   private static final int MAX_PREVIEW_RULES = 6;

   private final I18n i18n = LocalizationHelper.getI18n(MoveRulesToChainDialog.class);

   private List<EventProcessingPolicyRule> rules;
   private String sourceChainName;
   private List<EventProcessingPolicyChain> targetChains;
   private Table chainList;
   private Composite warningArea;
   private Label warningText;
   private Label infoText;
   private EventProcessingPolicyChain targetChain;

   /**
    * Create dialog.
    *
    * @param parentShell parent shell
    * @param rules rules being moved, in chain order
    * @param sourceChainName display name of the chain rules are moved out of
    * @param targetChains chains rules can be moved to, in display order
    */
   public MoveRulesToChainDialog(Shell parentShell, List<EventProcessingPolicyRule> rules, String sourceChainName, List<EventProcessingPolicyChain> targetChains)
   {
      super(parentShell);
      this.rules = rules;
      this.sourceChainName = sourceChainName;
      this.targetChains = targetChains;
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(i18n.tr("Move Rules to Another Chain"));
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      GridLayout layout = new GridLayout();
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
      dialogArea.setLayout(layout);

      Label lead = new Label(dialogArea, SWT.WRAP);
      lead.setText(String.format(i18n.tr("Move %d rule(s) out of \"%s\". They keep their order and land at the end of the target chain."), rules.size(), sourceChainName));
      GridData gd = new GridData(SWT.FILL, SWT.TOP, true, false);
      gd.widthHint = 500;
      lead.setLayoutData(gd);

      StringBuilder preview = new StringBuilder();
      for(int i = 0; (i < rules.size()) && (i < MAX_PREVIEW_RULES); i++)
      {
         EventProcessingPolicyRule rule = rules.get(i);
         if (i > 0)
            preview.append('\n');
         preview.append(rule.getRuleNumber()).append(". ").append(rule.getComments().isEmpty() ? i18n.tr("(no comments)") : rule.getComments().lines().findFirst().orElse(""));
      }
      if (rules.size() > MAX_PREVIEW_RULES)
         preview.append('\n').append(String.format(i18n.tr("and %d more"), rules.size() - MAX_PREVIEW_RULES));
      Label previewText = new Label(dialogArea, SWT.NONE);
      previewText.setText(preview.toString());
      gd = new GridData(SWT.FILL, SWT.TOP, true, false);
      gd.horizontalIndent = WidgetHelper.DIALOG_WIDTH_MARGIN;
      previewText.setLayoutData(gd);

      Label chainListLabel = new Label(dialogArea, SWT.NONE);
      chainListLabel.setText(i18n.tr("Target chain"));

      chainList = new Table(dialogArea, SWT.BORDER | SWT.SINGLE | SWT.FULL_SELECTION);
      chainList.setHeaderVisible(true);
      TableColumn nameColumn = new TableColumn(chainList, SWT.LEFT);
      nameColumn.setText(i18n.tr("Name"));
      nameColumn.setWidth(200);
      TableColumn descriptionColumn = new TableColumn(chainList, SWT.LEFT);
      descriptionColumn.setText(i18n.tr("Description"));
      descriptionColumn.setWidth(300);
      for(EventProcessingPolicyChain chain : targetChains)
      {
         TableItem item = new TableItem(chainList, SWT.NONE);
         item.setText(0, getChainName(chain));
         item.setText(1, (chain.getDescription() != null) ? chain.getDescription() : "");
         item.setData(chain);
      }
      gd = new GridData(SWT.FILL, SWT.FILL, true, true);
      gd.heightHint = 200;
      chainList.setLayoutData(gd);
      chainList.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            onTargetSelection();
         }

         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            onTargetSelection();
            okPressed();
         }
      });

      warningText = createNote(dialogArea, parent.getDisplay().getSystemImage(SWT.ICON_WARNING));
      warningArea = warningText.getParent();
      ((GridData)warningArea.getLayoutData()).exclude = true;
      warningArea.setVisible(false);

      infoText = createNote(dialogArea, SharedIcons.IMG_INFORMATION);
      updateInfoText();

      return dialogArea;
   }

   /**
    * Create note area with icon and wrapping text
    *
    * @param parent parent composite
    * @param image icon
    * @return text label (its parent is the note area)
    */
   private static Label createNote(Composite parent, Image image)
   {
      Composite area = new Composite(parent, SWT.NONE);
      GridLayout layout = new GridLayout(2, false);
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      area.setLayout(layout);
      area.setLayoutData(new GridData(SWT.FILL, SWT.TOP, true, false));

      Label icon = new Label(area, SWT.NONE);
      icon.setImage(image);
      icon.setLayoutData(new GridData(SWT.LEFT, SWT.TOP, false, false));

      Label text = new Label(area, SWT.WRAP);
      GridData gd = new GridData(SWT.FILL, SWT.TOP, true, false);
      gd.widthHint = 450;
      text.setLayoutData(gd);
      return text;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createButtonsForButtonBar(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createButtonsForButtonBar(Composite parent)
   {
      super.createButtonsForButtonBar(parent);
      getButton(IDialogConstants.OK_ID).setEnabled(false);
   }

   /**
    * Handle selection of target chain
    */
   private void onTargetSelection()
   {
      TableItem[] selection = chainList.getSelection();
      targetChain = (selection.length > 0) ? (EventProcessingPolicyChain)selection[0].getData() : null;
      getButton(IDialogConstants.OK_ID).setEnabled(targetChain != null);

      int selfCalls = 0;
      if (targetChain != null)
      {
         for(EventProcessingPolicyRule rule : rules)
            if (rule.getChainCalls().contains(targetChain.getId()))
               selfCalls++;
      }
      if (selfCalls > 0)
         warningText.setText(String.format(i18n.tr("%d rule(s) already call \"%s\". Once moved, that becomes a call to their own chain, which the server skips at run time - remove the call afterwards."),
               selfCalls, getChainName(targetChain)));
      ((GridData)warningArea.getLayoutData()).exclude = (selfCalls == 0);
      warningArea.setVisible(selfCalls > 0);

      updateInfoText();
      getShell().layout(true, true);
      getShell().setSize(getShell().computeSize(getShell().getSize().x, SWT.DEFAULT));
   }

   /**
    * Update note about unsaved changes
    */
   private void updateInfoText()
   {
      infoText.setText((targetChain != null) ?
            String.format(i18n.tr("Nothing is saved yet: \"%s\" and \"%s\" will both have unsaved changes. Use \"Save all\" to apply the move."), sourceChainName, getChainName(targetChain)) :
            String.format(i18n.tr("Nothing is saved yet: \"%s\" and the target chain will both have unsaved changes. Use \"Save all\" to apply the move."), sourceChainName));
   }

   /**
    * Get chain name for display
    *
    * @param chain chain
    * @return localized name of the main chain or chain name
    */
   private String getChainName(EventProcessingPolicyChain chain)
   {
      return chain.isMain() ? i18n.tr("Main") : chain.getName();
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      if (targetChain == null)
         return;
      super.okPressed();
   }

   /**
    * @return selected target chain
    */
   public EventProcessingPolicyChain getTargetChain()
   {
      return targetChain;
   }
}
