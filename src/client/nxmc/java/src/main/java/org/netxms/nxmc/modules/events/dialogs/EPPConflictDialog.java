/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Victor Kirhenshtein
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

import java.util.Date;
import java.util.List;
import java.util.UUID;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ColumnLabelProvider;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.TableViewerColumn;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Table;
import org.netxms.client.events.EPPConflict;
import org.netxms.client.events.EventProcessingPolicy;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog for displaying EPP save conflicts and resolution options.
 */
public class EPPConflictDialog extends Dialog
{
   public static final int RELOAD = 0;
   public static final int CANCEL = 1;

   private final I18n i18n = LocalizationHelper.getI18n(EPPConflictDialog.class);
   private List<EPPConflict> conflicts;
   private EventProcessingPolicy policy;
   private TableViewer viewer;

   /**
    * Create conflict dialog.
    *
    * @param parentShell parent shell
    * @param conflicts list of conflicts
    * @param policy the event processing policy (for looking up rule info)
    */
   public EPPConflictDialog(Shell parentShell, List<EPPConflict> conflicts, EventProcessingPolicy policy)
   {
      super(parentShell);
      setShellStyle(getShellStyle() | SWT.RESIZE);
      this.conflicts = conflicts;
      this.policy = policy;
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(i18n.tr("Save Conflicts Detected"));
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite area = (Composite)super.createDialogArea(parent);

      Composite container = new Composite(area, SWT.NONE);
      container.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
      GridLayout layout = new GridLayout(1, false);
      layout.marginWidth = 10;
      layout.marginHeight = 10;
      container.setLayout(layout);

      // Warning message
      Label message = new Label(container, SWT.WRAP);
      message.setText(i18n.tr("Some rules have been modified by other users while you were editing. Your changes cannot be saved until you reload the policy from the server. Your local changes will be lost."));
      GridData gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
      gd.widthHint = 500;
      message.setLayoutData(gd);

      // Conflicts table
      viewer = new TableViewer(container, SWT.BORDER | SWT.FULL_SELECTION | SWT.V_SCROLL);
      Table table = viewer.getTable();
      table.setHeaderVisible(true);
      table.setLinesVisible(true);
      gd = new GridData(SWT.FILL, SWT.FILL, true, true);
      gd.heightHint = 200;
      table.setLayoutData(gd);

      // Rule number column
      TableViewerColumn colRuleNum = new TableViewerColumn(viewer, SWT.NONE);
      colRuleNum.getColumn().setText(i18n.tr("Rule #"));
      colRuleNum.getColumn().setWidth(60);
      colRuleNum.setLabelProvider(new ColumnLabelProvider() {
         @Override
         public String getText(Object element)
         {
            EPPConflict conflict = (EPPConflict)element;
            int ruleNum = findRuleNumber(conflict.getRuleGuid());
            return (ruleNum > 0) ? String.valueOf(ruleNum) : "?";
         }
      });

      // Description column
      TableViewerColumn colDesc = new TableViewerColumn(viewer, SWT.NONE);
      colDesc.getColumn().setText(i18n.tr("Description"));
      colDesc.getColumn().setWidth(200);
      colDesc.setLabelProvider(new ColumnLabelProvider() {
         @Override
         public String getText(Object element)
         {
            EPPConflict conflict = (EPPConflict)element;
            EventProcessingPolicyRule rule = findRule(conflict.getRuleGuid());
            if (rule != null)
            {
               String comments = rule.getComments();
               return (comments != null && !comments.isEmpty()) ? comments : i18n.tr("(no description)");
            }
            return i18n.tr("(deleted locally)");
         }
      });

      // Conflict type column
      TableViewerColumn colType = new TableViewerColumn(viewer, SWT.NONE);
      colType.getColumn().setText(i18n.tr("Conflict"));
      colType.getColumn().setWidth(120);
      colType.setLabelProvider(new ColumnLabelProvider() {
         @Override
         public String getText(Object element)
         {
            EPPConflict conflict = (EPPConflict)element;
            if (conflict.getType() == EPPConflict.Type.MODIFY)
               return i18n.tr("Both modified");
            else if (!conflict.hasServerRule())
               return i18n.tr("Server deleted");
            else
               return i18n.tr("You deleted");
         }
      });

      // Modified by column
      TableViewerColumn colModifiedBy = new TableViewerColumn(viewer, SWT.NONE);
      colModifiedBy.getColumn().setText(i18n.tr("Modified By"));
      colModifiedBy.getColumn().setWidth(180);
      colModifiedBy.setLabelProvider(new ColumnLabelProvider() {
         @Override
         public String getText(Object element)
         {
            EPPConflict conflict = (EPPConflict)element;
            String name = conflict.getServerModifiedByName();
            long time = conflict.getServerModificationTime();
            if (name == null || name.isEmpty())
               return i18n.tr("Unknown");
            if (time > 0)
            {
               String formattedTime = DateFormatFactory.getDateTimeFormat().format(new Date(time * 1000));
               return name + " (" + formattedTime + ")";
            }
            return name;
         }
      });

      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setInput(conflicts);

      return area;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createButtonsForButtonBar(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createButtonsForButtonBar(Composite parent)
   {
      createButton(parent, RELOAD, i18n.tr("Reload from Server"), true);
      createButton(parent, CANCEL, i18n.tr("Cancel"), false);
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#buttonPressed(int)
    */
   @Override
   protected void buttonPressed(int buttonId)
   {
      setReturnCode(buttonId);
      close();
   }

   /**
    * Find rule by GUID.
    *
    * @param guid rule GUID
    * @return rule or null if not found
    */
   private EventProcessingPolicyRule findRule(UUID guid)
   {
      for(EventProcessingPolicyRule rule : policy.getRules())
      {
         if (rule.getGuid().equals(guid))
            return rule;
      }
      return null;
   }

   /**
    * Find rule number by GUID.
    *
    * @param guid rule GUID
    * @return rule number (1-based) or -1 if not found
    */
   private int findRuleNumber(UUID guid)
   {
      int num = 1;
      for(EventProcessingPolicyRule rule : policy.getRules())
      {
         if (rule.getGuid().equals(guid))
            return num;
         num++;
      }
      return -1;
   }
}
