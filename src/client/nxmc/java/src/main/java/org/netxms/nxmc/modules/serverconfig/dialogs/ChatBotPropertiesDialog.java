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
package org.netxms.nxmc.modules.serverconfig.dialogs;

import java.util.ArrayList;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.ChatBot;
import org.netxms.client.NXCSession;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.LabeledSpinner;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.ViewerElementUpdater;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Chat bot properties dialog
 */
public class ChatBotPropertiesDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(ChatBotPropertiesDialog.class);

   private ChatBot bot;
   private LabeledText textName;
   private LabeledText textDescription;
   private LabeledText textConfiguration;
   private LabeledText textProviderSlot;
   private LabeledSpinner spinnerIdleTimeout;
   private Combo comboDriverName;
   private SortableTableViewer mappingViewer;
   private Map<String, Integer> userMappings = new LinkedHashMap<String, Integer>();
   private boolean customName;

   /**
    * Create new chat bot properties dialog.
    *
    * @param parentShell parent shell
    * @param bot chat bot to edit or null to create new one
    */
   public ChatBotPropertiesDialog(Shell parentShell, ChatBot bot)
   {
      super(parentShell);
      this.bot = bot;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      GridLayout layout = new GridLayout();
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
      layout.numColumns = 2;
      dialogArea.setLayout(layout);

      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      comboDriverName = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, i18n.tr("Driver"), gd);
      comboDriverName.addModifyListener(listener -> {
         final int selectionIndex = comboDriverName.getSelectionIndex();
         if (selectionIndex != -1 && !customName)
         {
            textName.setText(comboDriverName.getItem(selectionIndex));
            textName.getTextControl().selectAll();
         }
      });

      textName = new LabeledText(dialogArea, SWT.NONE);
      textName.setLabel(i18n.tr("Name"));
      textName.getTextControl().setTextLimit(63);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.widthHint = 300;
      gd.horizontalSpan = 2;
      textName.setLayoutData(gd);
      textName.getTextControl().addModifyListener(listener -> {
         final String name = textName.getText();

         final int selectionIndex = comboDriverName.getSelectionIndex();
         if (selectionIndex != -1)
         {
            final String driver = comboDriverName.getItem(selectionIndex);
            customName = !name.equals(driver);
         }

         Control button = getButton(IDialogConstants.OK_ID);
         if (button != null)
         {
            button.setEnabled(!name.trim().isBlank());
         }
      });

      textDescription = new LabeledText(dialogArea, SWT.NONE);
      textDescription.setLabel(i18n.tr("Description"));
      textDescription.getTextControl().setTextLimit(255);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      textDescription.setLayoutData(gd);

      spinnerIdleTimeout = new LabeledSpinner(dialogArea, SWT.NONE);
      spinnerIdleTimeout.setLabel(i18n.tr("Session idle timeout (seconds)"));
      spinnerIdleTimeout.setRange(60, 86400);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      spinnerIdleTimeout.setLayoutData(gd);

      textProviderSlot = new LabeledText(dialogArea, SWT.NONE);
      textProviderSlot.setLabel(i18n.tr("AI provider slot (empty for default)"));
      textProviderSlot.getTextControl().setTextLimit(31);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      textProviderSlot.setLayoutData(gd);

      textConfiguration = new LabeledText(dialogArea, SWT.NONE, SWT.MULTI | SWT.BORDER);
      textConfiguration.setLabel(i18n.tr("Driver configuration"));
      textConfiguration.getTextControl().setFont(JFaceResources.getTextFont());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.heightHint = 200;
      gd.widthHint = 900;
      gd.horizontalSpan = 2;
      textConfiguration.setLayoutData(gd);

      createMappingSection(dialogArea);

      if (bot != null)
      {
         textName.setText(bot.getName());
         textDescription.setText(bot.getDescription());
         textConfiguration.setText(bot.getConfiguration());
         textProviderSlot.setText(bot.getProviderSlot());
         spinnerIdleTimeout.setSelection((bot.getIdleTimeout() > 0) ? bot.getIdleTimeout() : 1800);
         userMappings.putAll(bot.getUserMappings());
      }
      else
      {
         spinnerIdleTimeout.setSelection(1800);
      }
      mappingViewer.setInput(userMappings.entrySet().toArray());

      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Get driver names"), null) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final List<String> drivers = session.getChatBotDrivers();
            Collections.sort(drivers, String.CASE_INSENSITIVE_ORDER);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  updateUI(drivers);
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot get driver names");
         }
      }.start();

      return dialogArea;
   }

   /**
    * Create user mapping section of the dialog
    *
    * @param dialogArea parent composite
    */
   private void createMappingSection(Composite dialogArea)
   {
      final NXCSession session = Registry.getSession();

      Composite mappingArea = new Composite(dialogArea, SWT.NONE);
      GridLayout layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.numColumns = 2;
      mappingArea.setLayout(layout);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      mappingArea.setLayoutData(gd);

      final String[] names = { i18n.tr("Peer ID"), i18n.tr("User") };
      final int[] widths = { 300, 300 };
      mappingViewer = new SortableTableViewer(mappingArea, names, widths, 0, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI | SWT.BORDER);
      mappingViewer.setContentProvider(new ArrayContentProvider());
      mappingViewer.setLabelProvider(new MappingLabelProvider(session));
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      gd.heightHint = 150;
      mappingViewer.getControl().setLayoutData(gd);

      Composite buttons = new Composite(mappingArea, SWT.NONE);
      layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      buttons.setLayout(layout);
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      buttons.setLayoutData(gd);

      final Button addButton = new Button(buttons, SWT.PUSH);
      addButton.setText(i18n.tr("&Add..."));
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
      addButton.setLayoutData(gd);
      addButton.addListener(SWT.Selection, (e) -> addMapping());

      final Button editButton = new Button(buttons, SWT.PUSH);
      editButton.setText(i18n.tr("&Edit..."));
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
      editButton.setLayoutData(gd);
      editButton.addListener(SWT.Selection, (e) -> editMapping());

      final Button deleteButton = new Button(buttons, SWT.PUSH);
      deleteButton.setText(i18n.tr("&Delete"));
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
      deleteButton.setLayoutData(gd);
      deleteButton.addListener(SWT.Selection, (e) -> deleteMappings());

      mappingViewer.addDoubleClickListener((e) -> editMapping());
   }

   /**
    * Label provider for user mapping table
    */
   private class MappingLabelProvider extends LabelProvider implements ITableLabelProvider
   {
      private NXCSession session;

      MappingLabelProvider(NXCSession session)
      {
         this.session = session;
      }

      @Override
      public Image getColumnImage(Object element, int columnIndex)
      {
         return null;
      }

      @SuppressWarnings("unchecked")
      @Override
      public String getColumnText(Object element, int columnIndex)
      {
         Map.Entry<String, Integer> mapping = (Map.Entry<String, Integer>)element;
         if (columnIndex == 0)
            return mapping.getKey();
         AbstractUserObject user = session.findUserDBObjectById(mapping.getValue(), new ViewerElementUpdater(mappingViewer, element));
         return (user != null) ? user.getName() : ("[" + Integer.toString(mapping.getValue()) + "]");
      }
   }

   /**
    * Add new user mapping
    */
   private void addMapping()
   {
      ChatBotUserMappingDialog dlg = new ChatBotUserMappingDialog(getShell(), null, 0);
      if (dlg.open() != Window.OK)
         return;

      userMappings.put(dlg.getPeerId(), dlg.getUserId());
      mappingViewer.setInput(userMappings.entrySet().toArray());
   }

   /**
    * Change user in selected mapping
    */
   @SuppressWarnings("unchecked")
   private void editMapping()
   {
      IStructuredSelection selection = mappingViewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      Map.Entry<String, Integer> mapping = (Map.Entry<String, Integer>)selection.getFirstElement();
      ChatBotUserMappingDialog dlg = new ChatBotUserMappingDialog(getShell(), mapping.getKey(), mapping.getValue());
      if (dlg.open() != Window.OK)
         return;

      userMappings.put(dlg.getPeerId(), dlg.getUserId());
      mappingViewer.setInput(userMappings.entrySet().toArray());
   }

   /**
    * Delete selected user mappings
    */
   @SuppressWarnings("unchecked")
   private void deleteMappings()
   {
      for(Object o : mappingViewer.getStructuredSelection().toList())
         userMappings.remove(((Map.Entry<String, Integer>)o).getKey());
      mappingViewer.setInput(userMappings.entrySet().toArray());
   }

   /**
    * Update driver selection combo
    *
    * @param drivers list of available drivers
    */
   private void updateUI(List<String> drivers)
   {
      int index = 0;
      if (!drivers.isEmpty())
      {
         for(int i = 0; i < drivers.size(); i++)
         {
            String item = drivers.get(i);
            comboDriverName.add(item);

            if (bot != null && item.equals(bot.getDriverName()))
            {
               customName = !textName.getText().equals(bot.getDriverName());
               index = i;
            }
         }
         comboDriverName.select(index);
      }
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(bot != null ? i18n.tr("Update Chat Bot") : i18n.tr("Create Chat Bot"));
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      if (textName.getText().isEmpty())
      {
         MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Chat bot name should not be empty"));
         return;
      }

      if (comboDriverName.getSelectionIndex() == -1)
      {
         MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Chat bot driver should be selected"));
         return;
      }

      if (bot == null)
      {
         bot = new ChatBot();
      }

      bot.setName(textName.getText());
      bot.setDescription(textDescription.getText());
      bot.setDriverName(comboDriverName.getItem(comboDriverName.getSelectionIndex()));
      bot.setConfiguration(textConfiguration.getText());
      bot.setIdleTimeout(spinnerIdleTimeout.getSelection());
      bot.setProviderSlot(textProviderSlot.getText().trim());
      bot.setUserMappings(new LinkedHashMap<String, Integer>(userMappings));
      super.okPressed();
   }

   /**
    * Get updated chat bot object
    *
    * @return updated chat bot object
    */
   public ChatBot getChatBot()
   {
      return bot;
   }
}
