package org.netxms.nxmc.modules.businessservice.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.businessservices.ServiceCheck;
import org.netxms.client.constants.BusinessChecksType;
import org.netxms.client.objects.GenericObject;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.widgets.DciSelector;
import org.netxms.nxmc.modules.nxsl.widgets.ScriptEditor;
import org.netxms.nxmc.modules.objects.widgets.ObjectSelector;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Create or edit Business Service Check
 */
public class EditBusinessServiceCheckDlg extends Dialog
{
   private static final I18n i18n = LocalizationHelper.getI18n(EditBusinessServiceCheckDlg.class);
   private static final String[] TYPES = {i18n.tr("Object"), i18n.tr("Script"), i18n.tr("DCI")};
   
   private ServiceCheck check;
   private boolean createNew;
   
   private LabeledText descriptionText;
   private Combo typeCombo;
   private Combo thresholdCombo;
   private ObjectSelector objectSelector;
   private DciSelector dciSelector;
   private ScriptEditor scriptEditor;

   public EditBusinessServiceCheckDlg(Shell parentShell, ServiceCheck check, boolean createNew)
   {
      super(parentShell);
      this.check = check;
      this.createNew = createNew;
   }

   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(createNew ? i18n.tr("Create check") : i18n.tr("Modify check"));
   }  

   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);
      
      GridLayout layout = new GridLayout();
      layout.horizontalSpacing = WidgetHelper.OUTER_SPACING;
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      layout.numColumns = 2;
      dialogArea.setLayout(layout);
      
      descriptionText = new LabeledText(dialogArea, SWT.NONE);
      descriptionText.setLabel("Description");
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      descriptionText.setLayoutData(gd);
      
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      typeCombo = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, i18n.tr("Check type"), gd);
      for (String type : TYPES)
         typeCombo.add(type);   
      typeCombo.addSelectionListener(new SelectionListener() {
         
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            updateEditableElements();
         }
         
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }
      });
      
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      thresholdCombo = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, i18n.tr("Status Threshold"), gd);
      thresholdCombo.add(i18n.tr("Default"));   
      for (int i = 1; i <= 4; i++)
         thresholdCombo.add(StatusDisplayInfo.getStatusText(i));   
      
      objectSelector = new ObjectSelector(dialogArea, SWT.NONE, true);
      objectSelector.setObjectClass(GenericObject.class);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      objectSelector.setLayoutData(gd);
      
      dciSelector = new DciSelector(dialogArea, SWT.NONE, false);
      dciSelector.setLabel(i18n.tr("Data collection item"));
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      dciSelector.setLayoutData(gd);
      
      scriptEditor = new ScriptEditor(dialogArea, SWT.BORDER, SWT.H_SCROLL | SWT.V_SCROLL, true, true);
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.widthHint = 300;
      gd.heightHint = 300;
      gd.horizontalSpan = 2;
      scriptEditor.setLayoutData(gd);
      
      //Set all values 
      descriptionText.setText(check.getDescription());
      typeCombo.select(check.getCheckType().getValue());
      thresholdCombo.select(check.getThreshold());
      objectSelector.setObjectId(check.getObjectId());;
      dciSelector.setDciId(check.getObjectId(), check.getDciId());;
      scriptEditor.setText(check.getScript());  
      updateEditableElements();
      
      return dialogArea;
   }
   
   private void updateEditableElements()
   {
      BusinessChecksType type = BusinessChecksType.getByValue(typeCombo.getSelectionIndex());
      objectSelector.setEnabled(type == BusinessChecksType.OBJECT || type == BusinessChecksType.SCRIPT);
      dciSelector.setEnabled(type == BusinessChecksType.DCI);
      scriptEditor.setEnabled(type == BusinessChecksType.SCRIPT);
      thresholdCombo.setEnabled(type == BusinessChecksType.OBJECT || type == BusinessChecksType.DCI);
      if (type == BusinessChecksType.OBJECT)
         objectSelector.setLabel(i18n.tr("Check object"));
      else if (type == BusinessChecksType.SCRIPT)
         objectSelector.setLabel(i18n.tr("Related object"));
   }

   @Override
   protected void okPressed()
   {
      check.setDescription(descriptionText.getText());
      check.setCheckType(BusinessChecksType.getByValue(typeCombo.getSelectionIndex()));

      BusinessChecksType type = BusinessChecksType.getByValue(typeCombo.getSelectionIndex());
      if (type == BusinessChecksType.DCI)
      {
         check.setThreshold(thresholdCombo.getSelectionIndex());
         check.setObjectId(dciSelector.getNodeId());
         check.setDciId(dciSelector.getDciId());      
         check.setScript("");
      }
      else if (type == BusinessChecksType.OBJECT)
      {
         check.setThreshold(thresholdCombo.getSelectionIndex());
         check.setObjectId(objectSelector.getObjectId());
         check.setDciId(0);         
         check.setScript("");
      }
      else if (type == BusinessChecksType.SCRIPT)
      {
         check.setObjectId(objectSelector.getObjectId());
         check.setDciId(0);         
         check.setScript(scriptEditor.getText());
      }
      super.okPressed();
   }
   
}
