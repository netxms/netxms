package org.netxms.nxmc.modules.businessservice.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.businessservices.BusinessServiceCheck;
import org.netxms.client.constants.BusinessServiceCheckType;
import org.netxms.client.objects.GenericObject;
import org.netxms.nxmc.base.widgets.AbstractSelector;
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
   private static final String[] TYPES = { i18n.tr("Script"), i18n.tr("DCI threshold"), i18n.tr("Object status") };
   
   private BusinessServiceCheck check;
   private boolean createNew;
   
   private LabeledText descriptionText;
   private Combo typeCombo;
   private Combo thresholdCombo;
   private AbstractSelector objectOrDciSelector;
   private ScriptEditor scriptEditor;
   private Composite selectorGroup;
   private Composite dialogArea;

   public EditBusinessServiceCheckDlg(Shell parentShell, BusinessServiceCheck check, boolean createNew)
   {
      super(parentShell);
      this.check = check;
      this.createNew = createNew;
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(createNew ? i18n.tr("Create Business Service Check") : i18n.tr("Modify Business Service Check"));
   }  

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      dialogArea = (Composite)super.createDialogArea(parent);
      
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
      
      selectorGroup = new Composite(dialogArea, SWT.NONE);
      selectorGroup.setLayout(new FillLayout());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      selectorGroup.setLayoutData(gd);
      
      if (check.getCheckType() == BusinessServiceCheckType.DCI)
      {
         objectOrDciSelector = new DciSelector(selectorGroup, SWT.NONE, false);
         objectOrDciSelector.setLabel(i18n.tr("Data collection item"));
         ((DciSelector)objectOrDciSelector).setDciId(check.getObjectId(), check.getDciId());
      }
      else
      {        
         objectOrDciSelector = new ObjectSelector(selectorGroup, SWT.NONE, true);
         ((ObjectSelector)objectOrDciSelector).setObjectClass(GenericObject.class);
         ((ObjectSelector)objectOrDciSelector).setObjectId(check.getObjectId()); 
      }

      Label label = new Label(dialogArea, SWT.NONE);
      label.setText(i18n.tr("Check script"));
      gd = new GridData();
      gd.horizontalSpan = 2;
      label.setLayoutData(gd);

      scriptEditor = new ScriptEditor(dialogArea, SWT.BORDER, SWT.H_SCROLL | SWT.V_SCROLL, true, true);
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.widthHint = 700;
      gd.heightHint = 300;
      gd.horizontalSpan = 2;
      scriptEditor.setLayoutData(gd);
      
      //Set all values 
      descriptionText.setText(check.getDescription());
      typeCombo.select(check.getCheckType().getValue() - 1);
      thresholdCombo.select(check.getThreshold());
      scriptEditor.setText(check.getScript());  
      updateEditableElements();
      
      return dialogArea;
   }
   
   /**
    * Update dialog elements for selected check type
    */
   private void updateEditableElements()
   {
      BusinessServiceCheckType type = BusinessServiceCheckType.getByValue(typeCombo.getSelectionIndex() + 1);
      if (type == BusinessServiceCheckType.DCI && !(objectOrDciSelector instanceof DciSelector))
      {
         objectOrDciSelector.dispose();
         objectOrDciSelector = new DciSelector(selectorGroup, SWT.NONE, false);
         objectOrDciSelector.setLabel(i18n.tr("Data collection item"));
         ((DciSelector)objectOrDciSelector).setDciId(check.getObjectId(), check.getDciId());
      }
      if ((type == BusinessServiceCheckType.OBJECT || type == BusinessServiceCheckType.SCRIPT) && !(objectOrDciSelector instanceof ObjectSelector))
      {       
         objectOrDciSelector.dispose(); 
         objectOrDciSelector = new ObjectSelector(selectorGroup, SWT.NONE, true);
         ((ObjectSelector)objectOrDciSelector).setObjectClass(GenericObject.class);
         ((ObjectSelector)objectOrDciSelector).setObjectId(check.getObjectId()); 
      }
      
      scriptEditor.setEnabled(type == BusinessServiceCheckType.SCRIPT);
      thresholdCombo.setEnabled(type == BusinessServiceCheckType.OBJECT || type == BusinessServiceCheckType.DCI);
      if (type == BusinessServiceCheckType.OBJECT)
         ((ObjectSelector)objectOrDciSelector).setLabel(i18n.tr("Check object"));
      else if (type == BusinessServiceCheckType.SCRIPT)
         ((ObjectSelector)objectOrDciSelector).setLabel(i18n.tr("Related object"));

      dialogArea.layout(true, true);
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      check.setDescription(descriptionText.getText());

      BusinessServiceCheckType type = BusinessServiceCheckType.getByValue(typeCombo.getSelectionIndex() + 1);
      check.setCheckType(type);

      if (type == BusinessServiceCheckType.DCI)
      {
         check.setThreshold(thresholdCombo.getSelectionIndex());
         check.setObjectId(((DciSelector)objectOrDciSelector).getNodeId());
         check.setDciId(((DciSelector)objectOrDciSelector).getDciId());      
         check.setScript("");
      }
      else if (type == BusinessServiceCheckType.OBJECT)
      {
         check.setThreshold(thresholdCombo.getSelectionIndex());
         check.setObjectId(((ObjectSelector)objectOrDciSelector).getObjectId());
         check.setDciId(0);         
         check.setScript("");
      }
      else if (type == BusinessServiceCheckType.SCRIPT)
      {
         check.setObjectId(((ObjectSelector)objectOrDciSelector).getObjectId());
         check.setDciId(0);         
         check.setScript(scriptEditor.getText());
      }
      super.okPressed();
   }
}
