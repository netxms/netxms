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
package org.netxms.nxmc.modules.ai.views;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.dialogs.InputDialog;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.NXCSession;
import org.netxms.client.ai.AiAssistantFunction;
import org.netxms.client.ai.AiAssistantSkill;
import org.netxms.client.ai.AiDisabledItem;
import org.netxms.client.constants.UserAccessRights;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.helpers.TokenizedFilter;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.AbstractViewerFilter;
import org.netxms.nxmc.base.views.ConfigurationView;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.ai.views.helpers.AiItemComparator;
import org.netxms.nxmc.modules.ai.views.helpers.AiItemLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.xnap.commons.i18n.I18n;

/**
 * AI skills and functions management view
 */
public class AiSkillsManager extends ConfigurationView
{
   private final I18n i18n = LocalizationHelper.getI18n(AiSkillsManager.class);

   public static final int COLUMN_TYPE = 0;
   public static final int COLUMN_NAME = 1;
   public static final int COLUMN_DESCRIPTION = 2;
   public static final int COLUMN_STATUS = 3;

   private SortableTableViewer viewer;
   private NXCSession session;
   private List<AiItemEntry> items = new ArrayList<>();
   private Action actionEnable;
   private Action actionDisable;
   private Action actionAddCustom;
   private Action actionRemoveCustom;
   private boolean canManage;

   /**
    * Create AI skills manager view
    */
   public AiSkillsManager()
   {
      super(LocalizationHelper.getI18n(AiSkillsManager.class).tr("AI Skills & Functions"), ResourceManager.getImageDescriptor("icons/config-views/ai-skills.png"), "configuration.ai-skills", true);
      session = Registry.getSession();
      canManage = (session.getUserSystemRights() & UserAccessRights.SYSTEM_ACCESS_MANAGE_AI_SKILLS) != 0;
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      final String[] columnNames = { i18n.tr("Type"), i18n.tr("Name"), i18n.tr("Description"), i18n.tr("Status") };
      final int[] columnWidths = { 90, 200, 400, 100 };
      viewer = new SortableTableViewer(parent, columnNames, columnWidths, COLUMN_NAME, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI, "AISkillsManager");
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new AiItemLabelProvider());
      viewer.setComparator(new AiItemComparator());
      AiItemFilter filter = new AiItemFilter();
      viewer.addFilter(filter);
      setFilterClient(viewer, filter);
      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            updateActionState();
         }
      });

      createActions();
      createContextMenu();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#postContentCreate()
    */
   @Override
   protected void postContentCreate()
   {
      super.postContentCreate();
      refresh();
   }

   /**
    * @see org.netxms.nxmc.base.views.ConfigurationView#isModified()
    */
   @Override
   public boolean isModified()
   {
      return false;
   }

   /**
    * @see org.netxms.nxmc.base.views.ConfigurationView#save()
    */
   @Override
   public void save()
   {
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionEnable = new Action(i18n.tr("&Enable")) {
         @Override
         public void run()
         {
            setItemState(false);
         }
      };
      actionEnable.setEnabled(false);

      actionDisable = new Action(i18n.tr("&Disable")) {
         @Override
         public void run()
         {
            setItemState(true);
         }
      };
      actionDisable.setEnabled(false);

      actionAddCustom = new Action(i18n.tr("&Add custom entry..."), SharedIcons.ADD_OBJECT) {
         @Override
         public void run()
         {
            addCustomEntry();
         }
      };
      actionAddCustom.setEnabled(canManage);

      actionRemoveCustom = new Action(i18n.tr("&Remove custom entry"), SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            removeCustomEntry();
         }
      };
      actionRemoveCustom.setEnabled(false);
   }

   /**
    * Create context menu
    */
   private void createContextMenu()
   {
      MenuManager menuMgr = new MenuManager();
      menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener((m) -> fillContextMenu(m));
      Menu menu = menuMgr.createContextMenu(viewer.getControl());
      viewer.getControl().setMenu(menu);
   }

   /**
    * Fill context menu
    */
   private void fillContextMenu(IMenuManager manager)
   {
      if (canManage)
      {
         manager.add(actionEnable);
         manager.add(actionDisable);
         manager.add(actionAddCustom);
         manager.add(actionRemoveCustom);
      }
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(org.eclipse.jface.action.IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      if (canManage)
      {
         manager.add(actionEnable);
         manager.add(actionDisable);
         manager.add(actionAddCustom);
      }
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      Action resetAction = viewer.getResetColumnOrderAction();
      if (resetAction != null)
         manager.add(resetAction);
      Action showAllAction = viewer.getShowAllColumnsAction();
      if (showAllAction != null)
         manager.add(showAllAction);
      Action autoSizeAction = viewer.getAutoSizeColumnsAction();
      if (autoSizeAction != null)
      {
         manager.add(new Separator());
         manager.add(autoSizeAction);
      }
   }

   /**
    * Update action state based on selection
    */
   private void updateActionState()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      boolean hasSelection = !selection.isEmpty();
      boolean hasCustom = false;

      if (hasSelection)
      {
         for(Object o : selection.toList())
         {
            AiItemEntry entry = (AiItemEntry)o;
            if (!entry.isRegistered())
            {
               hasCustom = true;
               break;
            }
         }
      }

      actionEnable.setEnabled(canManage && hasSelection);
      actionDisable.setEnabled(canManage && hasSelection);
      actionRemoveCustom.setEnabled(canManage && hasCustom);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      new Job(i18n.tr("Loading AI skills and functions"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final Object[] result = session.getAiSkillsAndFunctions();
            runInUIThread(new Runnable() {
               @SuppressWarnings("unchecked")
               @Override
               public void run()
               {
                  items.clear();
                  for(AiAssistantSkill skill : (List<AiAssistantSkill>)result[0])
                     items.add(new AiItemEntry(skill));
                  for(AiAssistantFunction function : (List<AiAssistantFunction>)result[1])
                     items.add(new AiItemEntry(function));
                  for(AiDisabledItem extra : (List<AiDisabledItem>)result[2])
                     items.add(new AiItemEntry(extra));
                  viewer.setInput(items.toArray());
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot load AI skills and functions");
         }
      }.start();
   }

   /**
    * Enable or disable selected items
    */
   private void setItemState(final boolean disabled)
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;

      final List<AiItemEntry> selectedItems = new ArrayList<>();
      for(Object o : selection.toList())
         selectedItems.add((AiItemEntry)o);

      new Job(disabled ? i18n.tr("Disabling AI items") : i18n.tr("Enabling AI items"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(AiItemEntry entry : selectedItems)
            {
               if (disabled)
                  session.addAiDisabledItem(entry.getTypeChar(), entry.getName());
               else
                  session.removeAiDisabledItem(entry.getTypeChar(), entry.getName());
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot modify AI disabled list");
         }

         @Override
         protected void jobFinalize()
         {
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  refresh();
               }
            });
         }
      }.start();
   }

   /**
    * Add custom entry to the disabled list
    */
   private void addCustomEntry()
   {
      final String[] types = { i18n.tr("Skill"), i18n.tr("Function") };
      InputDialog dlg = new InputDialog(getWindow().getShell(), i18n.tr("Add Custom Entry"),
         i18n.tr("Enter the name of the skill or function to disable:"), "", null);
      if (dlg.open() != Window.OK)
         return;

      final String name = dlg.getValue().trim();
      if (name.isEmpty())
         return;

      // Ask for type
      org.eclipse.jface.dialogs.MessageDialog typeDlg = new org.eclipse.jface.dialogs.MessageDialog(
         getWindow().getShell(), i18n.tr("Item Type"), null,
         i18n.tr("Is \"{0}\" a skill or a function?", name),
         org.eclipse.jface.dialogs.MessageDialog.QUESTION, types, 1);
      int typeIndex = typeDlg.open();
      if (typeIndex < 0)
         return;

      final char type = (typeIndex == 0) ? 'S' : 'F';

      new Job(i18n.tr("Adding custom disabled entry"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.addAiDisabledItem(type, name);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot add custom disabled entry");
         }

         @Override
         protected void jobFinalize()
         {
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  refresh();
               }
            });
         }
      }.start();
   }

   /**
    * Remove custom (unregistered) entry from the disabled list
    */
   private void removeCustomEntry()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;

      if (!MessageDialogHelper.openQuestion(getWindow().getShell(), i18n.tr("Remove Custom Entries"),
            i18n.tr("Are you sure you want to remove selected custom entries from the disabled list?")))
         return;

      final List<AiItemEntry> selectedItems = new ArrayList<>();
      for(Object o : selection.toList())
      {
         AiItemEntry entry = (AiItemEntry)o;
         if (!entry.isRegistered())
            selectedItems.add(entry);
      }

      new Job(i18n.tr("Removing custom disabled entries"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(AiItemEntry entry : selectedItems)
               session.removeAiDisabledItem(entry.getTypeChar(), entry.getName());
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot remove custom disabled entries");
         }

         @Override
         protected void jobFinalize()
         {
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  refresh();
               }
            });
         }
      }.start();
   }

   /**
    * List item filter
    */
   private static class AiItemFilter extends ViewerFilter implements AbstractViewerFilter
   {
      private TokenizedFilter filter = new TokenizedFilter(null);

      /**
       * @see org.netxms.nxmc.base.views.AbstractViewerFilter#setFilterString(java.lang.String)
       */
      @Override
      public void setFilterString(String string)
      {
         filter = new TokenizedFilter(string);
      }

      /**
       * @see org.eclipse.jface.viewers.ViewerFilter#select(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
       */
      @Override
      public boolean select(Viewer viewer, Object parentElement, Object element)
      {
         if (filter.isEmpty())
            return true;

         AiItemEntry e = (AiItemEntry)element;
         return filter.matches(e.getDescription(), e.getName());
      }
   }

   /**
    * Unified item entry for the table viewer
    */
   public static class AiItemEntry
   {
      private String name;
      private String description;
      private char typeChar; // 'S' or 'F'
      private boolean disabled;
      private boolean registered;

      public AiItemEntry(AiAssistantSkill skill)
      {
         this.name = skill.getName();
         this.description = skill.getDescription();
         this.typeChar = 'S';
         this.disabled = skill.isDisabled();
         this.registered = true;
      }

      public AiItemEntry(AiAssistantFunction function)
      {
         this.name = function.getName();
         this.description = function.getDescription();
         this.typeChar = 'F';
         this.disabled = function.isDisabled();
         this.registered = true;
      }

      public AiItemEntry(AiDisabledItem extra)
      {
         this.name = extra.getName();
         this.description = "";
         this.typeChar = extra.getType();
         this.disabled = true;
         this.registered = false;
      }

      public String getName()
      {
         return name;
      }

      public String getDescription()
      {
         return description;
      }

      public char getTypeChar()
      {
         return typeChar;
      }

      public String getTypeName()
      {
         return typeChar == 'S' ? "Skill" : "Function";
      }

      public boolean isDisabled()
      {
         return disabled;
      }

      public boolean isRegistered()
      {
         return registered;
      }

      public String getStatusText()
      {
         return disabled ? "Disabled" : "Enabled";
      }
   }
}
