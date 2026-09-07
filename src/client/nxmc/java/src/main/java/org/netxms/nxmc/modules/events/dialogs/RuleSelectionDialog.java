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

import java.util.ArrayList;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.jface.viewers.ITreeContentProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.jface.viewers.ViewerFilter;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.TreeColumn;
import org.netxms.client.NXCSession;
import org.netxms.client.events.EventProcessingPolicy;
import org.netxms.client.events.EventProcessingPolicyChain;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.FilterText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog for selecting event processing policy rules. Rules are shown as a tree grouped by chain; selecting a chain selects all
 * its rules.
 */
public class RuleSelectionDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(RuleSelectionDialog.class);
   private static final String CONFIG_PREFIX = "SelectRule";

   private EventProcessingPolicy policy;
   private boolean multiSelection = true;
   private FilterText filterText;
   private TreeViewer viewer;
   private String filterString = null;
   private List<EventProcessingPolicyRule> selectedRules = new ArrayList<EventProcessingPolicyRule>();

   /**
    * @param parentShell parent shell
    * @param policy already loaded event processing policy (rules of all readable chains) or null to load it from server
    */
   public RuleSelectionDialog(Shell parentShell, EventProcessingPolicy policy)
   {
      super(parentShell);
      setShellStyle(getShellStyle() | SWT.RESIZE);
      this.policy = policy;
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(i18n.tr("Select Rule"));

      PreferenceStore settings = PreferenceStore.getInstance();
      int cx = settings.getAsInteger(CONFIG_PREFIX + ".cx", 600);
      int cy = settings.getAsInteger(CONFIG_PREFIX + ".cy", 460);
      newShell.setSize(cx, cy);
   }

   /**
    * Get display name of chain
    *
    * @param chain chain
    * @return chain name (localized for the main chain)
    */
   private String getChainName(EventProcessingPolicyChain chain)
   {
      return chain.isMain() ? i18n.tr("Main") : chain.getName();
   }

   /**
    * Check if rule matches current filter
    *
    * @param rule rule to check
    * @return true if rule matches or filter is empty
    */
   private boolean isRuleMatching(EventProcessingPolicyRule rule)
   {
      return (filterString == null) || rule.getComments().toLowerCase().contains(filterString);
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      PreferenceStore settings = PreferenceStore.getInstance();
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      GridLayout layout = new GridLayout();
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.horizontalSpacing = WidgetHelper.OUTER_SPACING;
      dialogArea.setLayout(layout);

      filterText = new FilterText(dialogArea, SWT.NONE, null, false);
      GridData gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      filterText.setLayoutData(gd);
      final String savedFilter = settings.getAsString("SelectRule.Filter");
      if (savedFilter != null)
         filterText.setText(savedFilter);
      filterString = ((savedFilter != null) && !savedFilter.isEmpty()) ? savedFilter.toLowerCase() : null;

      viewer = new TreeViewer(dialogArea, SWT.BORDER | SWT.FULL_SELECTION | (multiSelection ? SWT.MULTI : SWT.SINGLE) | SWT.H_SCROLL | SWT.V_SCROLL);
      viewer.getTree().setLinesVisible(true);
      viewer.getTree().setHeaderVisible(true);
      TreeColumn column = new TreeColumn(viewer.getTree(), SWT.LEFT);
      column.setText(i18n.tr("Chain / Rule #"));
      column.setWidth(160);
      column = new TreeColumn(viewer.getTree(), SWT.LEFT);
      column.setText(i18n.tr("Rule Name"));
      column.setWidth(350);

      viewer.setContentProvider(new ITreeContentProvider() {
         @Override
         public Object[] getElements(Object inputElement)
         {
            List<EventProcessingPolicyChain> chains = new ArrayList<>();
            for(EventProcessingPolicyChain chain : ((EventProcessingPolicy)inputElement).getChains())
               if (chain.isLoaded())
                  chains.add(chain);
            return chains.toArray();
         }

         @Override
         public Object[] getChildren(Object parentElement)
         {
            return (parentElement instanceof EventProcessingPolicyChain) ? ((EventProcessingPolicyChain)parentElement).getRules().toArray() : new Object[0];
         }

         @Override
         public Object getParent(Object element)
         {
            return (element instanceof EventProcessingPolicyRule) ? policy.findChain(((EventProcessingPolicyRule)element).getChainId()) : null;
         }

         @Override
         public boolean hasChildren(Object element)
         {
            return (element instanceof EventProcessingPolicyChain) && !((EventProcessingPolicyChain)element).getRules().isEmpty();
         }
      });
      viewer.setLabelProvider(new RuleTreeLabelProvider());
      viewer.setComparator(new ViewerComparator() {
         @Override
         public int compare(Viewer viewer, Object e1, Object e2)
         {
            if (e1 instanceof EventProcessingPolicyChain)
            {
               EventProcessingPolicyChain c1 = (EventProcessingPolicyChain)e1;
               EventProcessingPolicyChain c2 = (EventProcessingPolicyChain)e2;
               if (c1.isMain() != c2.isMain())
                  return c1.isMain() ? -1 : 1;
               return c1.getName().compareToIgnoreCase(c2.getName());
            }
            return ((EventProcessingPolicyRule)e1).getRuleNumber() - ((EventProcessingPolicyRule)e2).getRuleNumber();
         }
      });
      viewer.addFilter(new ViewerFilter() {
         @Override
         public boolean select(Viewer viewer, Object parentElement, Object element)
         {
            if (filterString == null)
               return true;
            if (element instanceof EventProcessingPolicyRule)
               return isRuleMatching((EventProcessingPolicyRule)element);
            EventProcessingPolicyChain chain = (EventProcessingPolicyChain)element;
            if (getChainName(chain).toLowerCase().contains(filterString))
               return true;
            for(EventProcessingPolicyRule rule : chain.getRules())
               if (isRuleMatching(rule))
                  return true;
            return false;
         }
      });
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.heightHint = 350;
      viewer.getTree().setLayoutData(gd);

      filterText.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            String text = filterText.getText();
            filterString = text.isEmpty() ? null : text.toLowerCase();
            viewer.refresh();
            viewer.expandAll();
         }
      });

      viewer.addDoubleClickListener((event) -> {
         Object element = viewer.getStructuredSelection().getFirstElement();
         if (element instanceof EventProcessingPolicyChain)
            viewer.setExpandedState(element, !viewer.getExpandedState(element));
         else
            okPressed();
      });

      if (policy == null)
      {
         viewer.getTree().setEnabled(false);
         getButton(IDialogConstants.OK_ID).setEnabled(false);
         final NXCSession session = Registry.getSession();
         Job job = new Job(i18n.tr("Get event processing rules"), null) {
            @Override
            protected void run(IProgressMonitor monitor) throws Exception
            {
               final EventProcessingPolicy loadedPolicy = session.getEventProcessingPolicy();
               runInUIThread(() -> {
                  policy = loadedPolicy;
                  viewer.getTree().setEnabled(true);
                  getButton(IDialogConstants.OK_ID).setEnabled(true);
                  viewer.setInput(policy);
                  viewer.expandAll();
               });
            }

            @Override
            protected String getErrorMessage()
            {
               return i18n.tr("Cannot get event processing rules from server");
            }
         };
         job.setUser(false);
         job.start();
      }
      else
      {
         viewer.setInput(policy);
         viewer.expandAll();
      }

      return dialogArea;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#cancelPressed()
    */
   @Override
   protected void cancelPressed()
   {
      saveSettings();
      super.cancelPressed();
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      // A selected chain stands for all its rules that pass the filter; keep policy order without duplicates
      Set<EventProcessingPolicyRule> rules = new LinkedHashSet<>();
      for(Object o : viewer.getStructuredSelection().toList())
      {
         if (o instanceof EventProcessingPolicyChain)
         {
            for(EventProcessingPolicyRule rule : ((EventProcessingPolicyChain)o).getRules())
               if (isRuleMatching(rule))
                  rules.add(rule);
         }
         else
         {
            rules.add((EventProcessingPolicyRule)o);
         }
      }
      selectedRules = new ArrayList<>(rules);
      saveSettings();
      super.okPressed();
   }

   /**
    * Save dialog settings
    */
   private void saveSettings()
   {
      Point size = getShell().getSize();
      PreferenceStore settings = PreferenceStore.getInstance();
      settings.set(CONFIG_PREFIX + ".cx", size.x);
      settings.set(CONFIG_PREFIX + ".cy", size.y);
      settings.set(CONFIG_PREFIX + ".Filter", filterText.getText());
   }

   /**
    * @return true if multiple rule selection is enabled
    */
   public boolean isMultiSelectionEnabled()
   {
      return multiSelection;
   }

   /**
    * Enable or disable selection of multiple rules.
    *
    * @param enable true to enable multiselection, false to disable
    */
   public void enableMultiSelection(boolean enable)
   {
      this.multiSelection = enable;
   }

   /**
    * @return the selectedRules
    */
   public List<EventProcessingPolicyRule> getSelectedRules()
   {
      return selectedRules;
   }

   /**
    * Label provider for the chain/rule tree
    */
   private class RuleTreeLabelProvider extends LabelProvider implements ITableLabelProvider
   {
      @Override
      public Image getColumnImage(Object element, int columnIndex)
      {
         return null;
      }

      @Override
      public String getColumnText(Object element, int columnIndex)
      {
         if (element instanceof EventProcessingPolicyChain)
         {
            EventProcessingPolicyChain chain = (EventProcessingPolicyChain)element;
            return (columnIndex == 0) ? getChainName(chain) : String.format(i18n.tr("%d rule(s)"), chain.getRules().size());
         }
         EventProcessingPolicyRule rule = (EventProcessingPolicyRule)element;
         return (columnIndex == 0) ? Integer.toString(rule.getRuleNumber()) : rule.getComments();
      }
   }
}
