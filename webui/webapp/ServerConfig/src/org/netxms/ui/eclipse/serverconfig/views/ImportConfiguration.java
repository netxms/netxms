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
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.filemanager.widgets.LocalFileSelector;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.serverconfig.Activator;
import org.netxms.ui.eclipse.serverconfig.Messages;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.widgets.StyledText;

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
            buttonImport.setEnabled(true);            
         }
      });
      
      buttonImport = new Button(parent, SWT.PUSH);
      buttonImport.setText("Import");
      buttonImport.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            run();
         }
      });
      gd = new GridData();
      gd.verticalAlignment = SWT.BOTTOM;
      buttonImport.setLayoutData(gd);
      buttonImport.setEnabled(false);
      
      options = new Group(parent, SWT.NONE);
      GridLayout optionsLayout = new GridLayout();
      optionsLayout.numColumns = 3;
      options.setLayout(optionsLayout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      options.setLayoutData(gd);
      
      addFlag("Replace existing &actions", NXCSession.CFG_IMPORT_REPLACE_ACTIONS);
      addFlag("Replace existing &DCI summary tables", NXCSession.CFG_IMPORT_REPLACE_SUMMARY_TABLES);
      addFlag("Replace existing EPP &rules", NXCSession.CFG_IMPORT_REPLACE_EPP_RULES);
      addFlag("Replace existing &events", NXCSession.CFG_IMPORT_REPLACE_EVENTS);
      addFlag("Replace existing library &scripts", NXCSession.CFG_IMPORT_REPLACE_SCRIPTS);
      addFlag("Replace existing &object tools", NXCSession.CFG_IMPORT_REPLACE_OBJECT_TOOLS);
      addFlag("Replace existing S&NMP traps", NXCSession.CFG_IMPORT_REPLACE_TRAPS);
      addFlag("Replace existing &templates", NXCSession.CFG_IMPORT_REPLACE_TEMPLATES);
      addFlag("Replace existing &template &names and &locations", NXCSession.CFG_IMPORT_REPLACE_TEMPLATE_NAMES_LOCATIONS);
      addFlag("Remove empty template groups after import", NXCSession.CFG_IMPORT_DELETE_EMPTY_TEMPLATE_GROUPS);
      addFlag("Replace &web service definitions", NXCSession.CFG_IMPORT_REPLACE_WEB_SVCERVICE_DEFINITIONS);
      addFlag("Replace existing &asset management defenitions", NXCSession.CFG_IMPORT_REPLACE_AM_DEFINITIONS);
      

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
      buttonSelectAll.setText("Select all");
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
      buttonSelectAll.setLayoutData(gd);
      
      buttonDeselectAll = new Button(buttons, SWT.PUSH);
      buttonDeselectAll.setText("Clear selection");
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
      buttonDeselectAll.setLayoutData(gd);
      
      text = new StyledText(parent, SWT.H_SCROLL | SWT.V_SCROLL);
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
   
   private void addFlag(String text, int flag)
   {
      Button button = new Button(options, SWT.CHECK);
      button.setText(text);
      button.setData(flag);
      buttonFlags.add(button);
   }

   private void run()
   {
      int flagBuilder = 0;
      for (Button button : buttonFlags)
      {
         if (button.getSelection())
         {
            flagBuilder |= (Integer)button.getData();            
         }
      }
      
      final int flags = flagBuilder;
      final NXCSession session = ConsoleSharedData.getSession();;
      ConsoleJob job = new ConsoleJob(Messages.get().ImportConfiguration_JobName, null, Activator.PLUGIN_ID, null) {
         @Override
         protected String getErrorMessage()
         {
            return Messages.get().ImportConfiguration_JobError;
         }
         
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
                     text.setText(result);
                  }
               });
            }
            catch (NXCException e)
            {
               runInUIThread(new Runnable() {                  
                  @Override
                  public void run()
                  {
                     text.setText(e.getAdditionalInfo());
                     text.append(e.getLocalizedMessage());
                  }
               });         
               throw e;
            }
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
