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
package org.netxms.ui.eclipse.serverconfig.views;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
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
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.filemanager.widgets.LocalFileSelector;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.serverconfig.Activator;
import org.netxms.ui.eclipse.serverconfig.Messages;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.StyledText;

/**
 * "Import Configuration" view
 */
public class ImportConfiguration extends ViewPart
{   
   public static final String ID = "org.netxms.ui.eclipse.serverconfig.views.ImportConfiguration"; //$NON-NLS-1$

   private org.netxms.ui.eclipse.filemanager.widgets.LocalFileSelector fileSelector;

   private Button buttonImport;
   private Group options;
   private List<Button> buttonFlags = new ArrayList<Button>();
   private Button buttonSelectAll;
   private Button buttonDeselectAll;
   private StyledText text;

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createPartControl(Composite parent)
   { 
      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      parent.setLayout(layout);

      fileSelector = new LocalFileSelector(parent, SWT.NONE, false, SWT.OPEN | SWT.SINGLE);
      fileSelector.setLabel(Messages.get().ConfigurationImportDialog_FileName);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      fileSelector.setLayoutData(gd);
      fileSelector.addModifyListener(new ModifyListener() {         
         @Override
         public void modifyText(ModifyEvent e)
         {
            buttonImport.setEnabled(fileSelector.getFile() != null);
         }
      });

      buttonImport = new Button(parent, SWT.PUSH);
      buttonImport.setText("Import");
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
      options.setText("Options");
      GridLayout optionsLayout = new GridLayout();
      optionsLayout.numColumns = 3;
      options.setLayout(optionsLayout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      options.setLayoutData(gd);

      addOptionCheckBox("Replace &actions", NXCSession.CFG_IMPORT_REPLACE_ACTIONS);
      addOptionCheckBox("Replace asset &management attributes", NXCSession.CFG_IMPORT_REPLACE_AM_DEFINITIONS);
      addOptionCheckBox("Replace &DCI summary tables", NXCSession.CFG_IMPORT_REPLACE_SUMMARY_TABLES);
      addOptionCheckBox("Replace EPP &rules", NXCSession.CFG_IMPORT_REPLACE_EPP_RULES);
      addOptionCheckBox("Replace &events", NXCSession.CFG_IMPORT_REPLACE_EVENTS);
      addOptionCheckBox("Replace library &scripts", NXCSession.CFG_IMPORT_REPLACE_SCRIPTS);
      addOptionCheckBox("Replace &object tools", NXCSession.CFG_IMPORT_REPLACE_OBJECT_TOOLS);
      addOptionCheckBox("Replace S&NMP traps", NXCSession.CFG_IMPORT_REPLACE_TRAPS);
      addOptionCheckBox("Replace &templates", NXCSession.CFG_IMPORT_REPLACE_TEMPLATES);
      addOptionCheckBox("Replace template names and &locations", NXCSession.CFG_IMPORT_REPLACE_TEMPLATE_NAMES_LOCATIONS);
      addOptionCheckBox("Replace &web service definitions", NXCSession.CFG_IMPORT_REPLACE_WEB_SVCERVICE_DEFINITIONS);
      addOptionCheckBox("Remove empty template &groups after import", NXCSession.CFG_IMPORT_DELETE_EMPTY_TEMPLATE_GROUPS);

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
      buttonSelectAll.setText("&Select all");
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
      buttonDeselectAll.setText("&Clear all");
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

      new Label(parent, SWT.NONE).setText("Import log");

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
      final NXCSession session = ConsoleSharedData.getSession();;
      ConsoleJob job = new ConsoleJob(Messages.get().ImportConfiguration_JobName, null, Activator.PLUGIN_ID) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
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
            return Messages.get().ImportConfiguration_JobError;
         }
      };
      job.start();
   }

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
    */
   @Override
   public void setFocus()
   {
      fileSelector.setFocus();
   }
}
