package org.netxms.nxmc.modules.serverconfig.views;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Group;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.ConfigurationView;
import org.netxms.nxmc.base.widgets.StyledText;
import org.netxms.nxmc.base.widgets.helpers.LineStyler;
import org.netxms.nxmc.base.widgets.helpers.StyleRange;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.filemanager.widgets.LocalFileSelector;
import org.netxms.nxmc.tools.ColorCache;
import org.xnap.commons.i18n.I18n;

public class ImportConfiguration extends ConfigurationView
{
   private static final I18n i18n = LocalizationHelper.getI18n(ServerVariables.class);
   
   private LocalFileSelector fileSelector;

   private Button buttonImport;
   private Group options;
   private List<Button> buttonFlags = new ArrayList<Button>();
   private Button buttonSelectAll;
   private Button buttonDeselectAll;
   private StyledText text;

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
      
      addFlag(i18n.tr("Replace existing &actions"), NXCSession.CFG_IMPORT_REPLACE_ACTIONS);
      addFlag(i18n.tr("Replace existing &DCI summary tables"), NXCSession.CFG_IMPORT_REPLACE_SUMMARY_TABLES);
      addFlag(i18n.tr("Replace existing EPP &rules"), NXCSession.CFG_IMPORT_REPLACE_EPP_RULES);
      addFlag(i18n.tr("Replace existing &events"), NXCSession.CFG_IMPORT_REPLACE_EVENTS);
      addFlag(i18n.tr("Replace existing library &scripts"), NXCSession.CFG_IMPORT_REPLACE_SCRIPTS);
      addFlag(i18n.tr("Replace existing &object tools"), NXCSession.CFG_IMPORT_REPLACE_OBJECT_TOOLS);
      addFlag(i18n.tr("Replace existing S&NMP traps"), NXCSession.CFG_IMPORT_REPLACE_TRAPS);
      addFlag(i18n.tr("Replace existing &templates"), NXCSession.CFG_IMPORT_REPLACE_TEMPLATES);
      addFlag(i18n.tr("Replace existing &template &names and &locations"), NXCSession.CFG_IMPORT_REPLACE_TEMPLATE_NAMES_LOCATIONS);
      addFlag(i18n.tr("Remove empty template groups after import"), NXCSession.CFG_IMPORT_DELETE_EMPTY_TEMPLATE_GROUPS);
      addFlag(i18n.tr("Replace &web service definitions"), NXCSession.CFG_IMPORT_REPLACE_WEB_SVCERVICE_DEFINITIONS);
      addFlag(i18n.tr("Replace existing &asset management defenitions"), NXCSession.CFG_IMPORT_REPLACE_AM_DEFINITIONS);
      

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
      
      ColorCache colorCache = new ColorCache(parent);      
      LineStyler lineStyler = new LineStyler() {

         private final RGB COLOR_ERROR = new RGB(192, 0, 0);
         private final RGB COLOR_SUCCESS = new RGB(155, 187, 89);
         
         @Override
         public StyleRange[] styleLine(String line)
         {
            List<StyleRange> styles = new ArrayList<StyleRange>(16);
            
            if (line.startsWith("ERROR")) 
            {
               StyleRange r = new StyleRange(0, line.length(), colorCache.create(COLOR_ERROR), null);
               r.fontStyle = SWT.BOLD;
               styles.add(r);
            }
            
            if (line.startsWith("SUCCESS")) 
            {
               StyleRange r = new StyleRange(0, line.length(), colorCache.create(COLOR_SUCCESS), null);
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
      final NXCSession session = Registry.getSession();
      Job job = new Job(i18n.tr("Import configuration"), this) {
         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Failed to import configuration");
         }

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
                     text.setText(result);
                     text.append("SUCCESS");
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
                     text.append("ERROR");
                  }
               });         
               throw e;
            }
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
