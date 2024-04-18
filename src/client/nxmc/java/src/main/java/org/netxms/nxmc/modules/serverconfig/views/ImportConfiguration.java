/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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
package org.netxms.nxmc.modules.serverconfig.views;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Label;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.Severity;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.ConfigurationView;
import org.netxms.nxmc.base.widgets.StyledText;
import org.netxms.nxmc.base.widgets.helpers.LineStyler;
import org.netxms.nxmc.base.widgets.helpers.StyleRange;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.filemanager.widgets.LocalFileSelector;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Import Configuration" view
 */
public class ImportConfiguration extends ConfigurationView
{
   private final I18n i18n = LocalizationHelper.getI18n(ImportConfiguration.class);

   private LocalFileSelector fileSelector;
   private Button buttonImport;
   private Group options;
   private List<Button> buttonFlags = new ArrayList<Button>();
   private Button buttonSelectAll;
   private Button buttonDeselectAll;
   private StyledText text;
   private Action actionImport;

   /**
    * Create view
    */
   public ImportConfiguration()
   {
      super(LocalizationHelper.getI18n(ImportConfiguration.class).tr("Import Configuration"), SharedIcons.IMPORT, "config.import", false);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      parent.setLayout(layout);

      fileSelector = new LocalFileSelector(parent, SWT.NONE, false, SWT.OPEN | SWT.SINGLE);
      fileSelector.setLabel(i18n.tr("File name"));
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      fileSelector.setLayoutData(gd);
      fileSelector.addModifyListener(new ModifyListener() {         
         @Override
         public void modifyText(ModifyEvent e)
         {
            boolean fileSelected = (fileSelector.getFile() != null);
            buttonImport.setEnabled(fileSelected);
            actionImport.setEnabled(fileSelected);
         }
      });

      buttonImport = new Button(parent, SWT.PUSH);
      buttonImport.setText(i18n.tr("Import"));
      buttonImport.setToolTipText(i18n.tr("Start import from selected file (F9)"));
      buttonImport.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            doImport();
         }
      });
      gd = new GridData();
      gd.verticalAlignment = SWT.BOTTOM;
      gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
      buttonImport.setLayoutData(gd);
      buttonImport.setEnabled(false);

      options = new Group(parent, SWT.NONE);
      options.setText(i18n.tr("Options"));
      GridLayout optionsLayout = new GridLayout();
      optionsLayout.numColumns = 3;
      options.setLayout(optionsLayout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      options.setLayoutData(gd);

      addOptionCheckBox(i18n.tr("Replace &actions"), NXCSession.CFG_IMPORT_REPLACE_ACTIONS);
      addOptionCheckBox(i18n.tr("Replace asset &management attributes"), NXCSession.CFG_IMPORT_REPLACE_AM_DEFINITIONS);
      addOptionCheckBox(i18n.tr("Replace &DCI summary tables"), NXCSession.CFG_IMPORT_REPLACE_SUMMARY_TABLES);
      addOptionCheckBox(i18n.tr("Replace EPP &rules"), NXCSession.CFG_IMPORT_REPLACE_EPP_RULES);
      addOptionCheckBox(i18n.tr("Replace &events"), NXCSession.CFG_IMPORT_REPLACE_EVENTS);
      addOptionCheckBox(i18n.tr("Replace library &scripts"), NXCSession.CFG_IMPORT_REPLACE_SCRIPTS);
      addOptionCheckBox(i18n.tr("Replace &object tools"), NXCSession.CFG_IMPORT_REPLACE_OBJECT_TOOLS);
      addOptionCheckBox(i18n.tr("Replace S&NMP traps"), NXCSession.CFG_IMPORT_REPLACE_TRAPS);
      addOptionCheckBox(i18n.tr("Replace &templates"), NXCSession.CFG_IMPORT_REPLACE_TEMPLATES);
      addOptionCheckBox(i18n.tr("Replace template names and &locations"), NXCSession.CFG_IMPORT_REPLACE_TEMPLATE_NAMES_LOCATIONS);
      addOptionCheckBox(i18n.tr("Replace &web service definitions"), NXCSession.CFG_IMPORT_REPLACE_WEB_SVCERVICE_DEFINITIONS);
      addOptionCheckBox(i18n.tr("Remove empty template &groups after import"), NXCSession.CFG_IMPORT_DELETE_EMPTY_TEMPLATE_GROUPS);

      Composite buttons = new Composite(options, SWT.NONE);
      layout = new GridLayout();
      layout.numColumns = 2;
      layout.makeColumnsEqualWidth = true;
      buttons.setLayout(layout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 3;
      buttons.setLayoutData(gd);

      buttonSelectAll = new Button(buttons, SWT.PUSH);
      buttonSelectAll.setText(i18n.tr("&Select all"));
      buttonSelectAll.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            for (Button button : buttonFlags)
            {
               button.setSelection(true);
            }
         }
      });
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
      buttonSelectAll.setLayoutData(gd);

      buttonDeselectAll = new Button(buttons, SWT.PUSH);
      buttonDeselectAll.setText(i18n.tr("&Clear all"));
      buttonDeselectAll.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            for (Button button : buttonFlags)
            {
               button.setSelection(false);
            }
         }
      });
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
      buttonDeselectAll.setLayoutData(gd);

      new Label(parent, SWT.NONE).setText(i18n.tr("Import log"));

      text = new StyledText(parent, SWT.H_SCROLL | SWT.V_SCROLL | SWT.BORDER);
      text.setEditable(false);
      text.setFont(JFaceResources.getTextFont());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.horizontalSpan = 2;
      text.setLayoutData(gd);

      LineStyler lineStyler = new LineStyler() {
         @Override
         public StyleRange[] styleLine(String line)
         {
            List<StyleRange> styles = new ArrayList<StyleRange>(16);

            if (line.startsWith("[ERROR]") || line.startsWith("[FAILURE]"))
            {
               StyleRange r = new StyleRange(1, line.indexOf(']') - 1, StatusDisplayInfo.getStatusColor(Severity.CRITICAL), null);
               r.fontStyle = SWT.BOLD;
               styles.add(r);
            }
            else if (line.startsWith("[WARNING]"))
            {
               StyleRange r = new StyleRange(1, 7, StatusDisplayInfo.getStatusColor(Severity.MAJOR), null);
               r.fontStyle = SWT.BOLD;
               styles.add(r);
            }
            else if (line.startsWith("[SUCCESS]"))
            {
               StyleRange r = new StyleRange(1, 7, StatusDisplayInfo.getStatusColor(Severity.NORMAL), null);
               r.fontStyle = SWT.BOLD;
               styles.add(r);
            }
            else if (line.startsWith("[INFO]"))
            {
               StyleRange r = new StyleRange(1, 4, StatusDisplayInfo.getStatusColor(Severity.UNKNOWN), null);
               r.fontStyle = SWT.BOLD;
               styles.add(r);
            }
            return (styles.size() > 0) ? styles.toArray(new StyleRange[styles.size()]) : null;
         }

         @Override
         public void dispose()
         {
         }
      };
      text.setLineStyler(lineStyler);

      createActions();
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionImport = new Action(i18n.tr("Import"), SharedIcons.IMPORT) {
         @Override
         public void run()
         {
            doImport();
         }
      };
      actionImport.setEnabled(false);
      addKeyBinding("F9", actionImport);
   }

   /**
    * Add checkbox for import option
    *
    * @param text option text
    * @param flag bit flag to set
    */
   private void addOptionCheckBox(String text, int flag)
   {
      Button button = new Button(options, SWT.CHECK);
      button.setText(text);
      button.setData("BitFlag", flag);
      buttonFlags.add(button);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(org.eclipse.jface.action.IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionImport);
      super.fillLocalToolBar(manager);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionImport);
      super.fillLocalMenu(manager);
   }

   /**
    * Run import
    */
   private void doImport()
   {
      text.setText("");

      int flagBuilder = 0;
      for (Button button : buttonFlags)
      {
         if (button.getSelection())
            flagBuilder |= (Integer)button.getData("BitFlag");
      }

      final int flags = flagBuilder;
      final NXCSession session = Registry.getSession();
      Job job = new Job(i18n.tr("Importing configuration"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            try
            {
               String result = session.importConfiguration(fileSelector.getFile(), flags);
               runInUIThread(new Runnable() {                  
                  @Override
                  public void run()
                  {
                     text.setText(result + "[SUCCESS] Import completed successfully");
                  }
               });
            }
            catch (NXCException e)
            {
               runInUIThread(new Runnable() {                  
                  @Override
                  public void run()
                  {
                     String output = e.getAdditionalInfo();
                     text.setText(((output != null) ? output : "") + "[FAILURE] Import failed (" + e.getLocalizedMessage() + ")");
                  }
               });         
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Failed to import configuration");
         }
      };
      job.start();
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
}
