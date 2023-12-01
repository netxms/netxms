package org.netxms.nxmc.modules.objects.dialogs;

import java.util.HashSet;
import java.util.Set;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Spinner;
import org.netxms.client.NXCSession;
import org.netxms.client.PhysicalLink;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Interface;
import org.netxms.client.objects.Rack;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.modules.objects.widgets.ObjectSelector;
import org.netxms.nxmc.modules.objects.widgets.PatchPanelSelector;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;

/**
 * Dialog to create/edit physical link object
 */
public class PhysicalLinkEditDialog extends Dialog
{
   private final static String[] SIDE = { "FRONT", "BACK" };

   private PhysicalLink link;
   private String title;
   private NXCSession session;
   private LabeledText description;
   private ObjectSelector objectSelectorRight;
   private PatchPanelSelector patchPanelSelectorRight;
   private Spinner portRight;
   private Combo sideRight;
   private ObjectSelector objectSelectorLeft;
   private PatchPanelSelector patchPanelSelectorLeft;
   private Spinner portLeft;
   private Combo sideLeft;
   private long currLeftObjId;
   private long currRightObjId;

   /**
    * Physical link add/edit dialog constructor
    * 
    * @param parentShell
    * @param link link to edit or null if new should be created
    */
   public PhysicalLinkEditDialog(Shell parentShell, PhysicalLink link)
   {
      super(parentShell);
      this.link = (link != null) ? new PhysicalLink(link) : new PhysicalLink();
      title = (link != null) ? "Edit Physical Link" : "Create Physical Link";
      session = Registry.getSession();
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(title);
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      currLeftObjId = link.getLeftObjectId();
      currRightObjId = link.getRightObjectId();
      Composite dialogArea = new Composite(parent, SWT.NONE);
      
      GridLayout layout = new GridLayout();
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.numColumns = 2;
      dialogArea.setLayout(layout);
      
      GridData gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.horizontalSpan = 2;      
      description = new LabeledText(dialogArea, SWT.NONE);
      description.setLabel("Description");
      description.setText(link.getDescription());
      description.setLayoutData(gd);

      Group leftGroup = new Group(dialogArea, SWT.NONE);
      layout = new GridLayout();
      layout.horizontalSpacing = WidgetHelper.DIALOG_SPACING;
      layout.numColumns = 2; 
      layout.makeColumnsEqualWidth = true;
      leftGroup.setLayout(layout);
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      leftGroup.setLayoutData(gd);

      Set<Class<? extends AbstractObject>> filterSet = new HashSet<Class<? extends AbstractObject>>();
      objectSelectorLeft = new ObjectSelector(leftGroup, SWT.NONE, false);
      objectSelectorLeft.setObjectId(link.getLeftObjectId());
      objectSelectorLeft.setLabel("Interface or rack");
      objectSelectorLeft.setObjectClass(filterSet);
      gd = new GridData();
      gd.horizontalSpan = 2;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      objectSelectorLeft.setLayoutData(gd);
      objectSelectorLeft.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            if (currLeftObjId != objectSelectorLeft.getObjectId())
               patchPanelSelectorLeft.setPatchPanelId(0);
            currLeftObjId = objectSelectorLeft.getObjectId();
            if(objectSelectorLeft.getObject() instanceof Rack)
            {
               patchPanelSelectorLeft.setEnabled(true);
               portLeft.setEnabled(true);
               sideLeft.setEnabled(true);
               patchPanelSelectorLeft.setRack((Rack)objectSelectorLeft.getObject());
            }
            else
            {
               patchPanelSelectorLeft.setEnabled(false);
               portLeft.setEnabled(false);
               sideLeft.setEnabled(false);               
            }
         }
      });

      AbstractObject obj = session.findObjectById(link.getLeftObjectId());
      patchPanelSelectorLeft = new PatchPanelSelector(leftGroup, SWT.NONE, obj instanceof Rack ? (Rack)obj : null);
      patchPanelSelectorLeft.setPatchPanelId(link.getLeftPatchPanelId());
      patchPanelSelectorLeft.setLabel("Patch panel");
      gd = new GridData();
      gd.horizontalSpan = 2;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      patchPanelSelectorLeft.setLayoutData(gd);
      
      portLeft = WidgetHelper.createLabeledSpinner(leftGroup, SWT.BORDER, "Port", 1, 64, WidgetHelper.DEFAULT_LAYOUT_DATA);
      portLeft.setSelection(link.getLeftPortNumber());
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      portLeft.setLayoutData(gd);
      
      sideLeft = WidgetHelper.createLabeledCombo(leftGroup, SWT.READ_ONLY, "Side", gd);
      sideLeft.setItems(SIDE);
      sideLeft.select(link.getLeftFront());
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      sideLeft.setLayoutData(gd);
      
      if (!(obj instanceof Rack))
      {
         patchPanelSelectorLeft.setEnabled(false);
         portLeft.setEnabled(false);
         sideLeft.setEnabled(false);
      }
      else
      {
         patchPanelSelectorLeft.setPatchPanelId(link.getLeftPatchPanelId());
      }

      Group rightGroup = new Group(dialogArea, SWT.NONE);
      layout = new GridLayout();
      layout.horizontalSpacing = WidgetHelper.DIALOG_SPACING;
      layout.numColumns = 2; 
      layout.makeColumnsEqualWidth = true;
      rightGroup.setLayout(layout);
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      rightGroup.setLayoutData(gd);

      filterSet.add(Interface.class);
      filterSet.add(Rack.class);
      objectSelectorRight = new ObjectSelector(rightGroup, SWT.NONE, false);
      objectSelectorRight.setObjectId(link.getRightObjectId());
      objectSelectorRight.setLabel("Interface or rack");
      objectSelectorRight.setObjectClass(filterSet);
      gd = new GridData();
      gd.horizontalSpan = 2;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      objectSelectorRight.setLayoutData(gd);
      objectSelectorRight.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            if (currRightObjId != objectSelectorRight.getObjectId())
               patchPanelSelectorRight.setPatchPanelId(0);
            currRightObjId = objectSelectorRight.getObjectId();
            
            if(objectSelectorRight.getObject() instanceof Rack)
            {
               patchPanelSelectorRight.setEnabled(true);
               portRight.setEnabled(true);
               sideRight.setEnabled(true);
               patchPanelSelectorRight.setRack((Rack)objectSelectorRight.getObject());
            }
            else
            {
               patchPanelSelectorRight.setEnabled(false);
               portRight.setEnabled(false);
               sideRight.setEnabled(false);               
            }
         }
      });

      obj = session.findObjectById(link.getRightObjectId());
      patchPanelSelectorRight = new PatchPanelSelector(rightGroup, SWT.NONE, obj instanceof Rack ? (Rack)obj : null);
      patchPanelSelectorRight.setLabel("Patch panel");
      gd = new GridData();
      gd.horizontalSpan = 2;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      patchPanelSelectorRight.setLayoutData(gd);

      portRight = WidgetHelper.createLabeledSpinner(rightGroup, SWT.BORDER, "Port", 1, 64, WidgetHelper.DEFAULT_LAYOUT_DATA);
      portRight.setSelection(link.getRightPortNumber());
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      portRight.setLayoutData(gd);
      
      sideRight = WidgetHelper.createLabeledCombo(rightGroup, SWT.READ_ONLY, "Side", gd);
      sideRight.setItems(SIDE);
      sideRight.select(link.getRightFront());
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      sideRight.setLayoutData(gd);
      
      if (!(obj instanceof Rack))
      {
         patchPanelSelectorRight.setEnabled(false);
         portRight.setEnabled(false);
         sideRight.setEnabled(false);
      }
      else
         patchPanelSelectorRight.setPatchPanelId(link.getRightPatchPanelId());
      
      return dialogArea;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      if (objectSelectorLeft.getObject() == null || objectSelectorRight.getObject() == null)
      {
         MessageDialogHelper.openWarning(getShell(), "Warning", "Please select objects for both sides of the link");
         return;
      }

      if ((objectSelectorLeft.getObject() instanceof Rack && patchPanelSelectorLeft.getPatchPanelId() == 0) ||
            (objectSelectorRight.getObject() instanceof Rack && patchPanelSelectorRight.getPatchPanelId() == 0))
      {
         MessageDialogHelper.openWarning(getShell(), "Warning", "Please select patch panel(s) for rack(s)");
         return;
      }

      link.setDescription(description.getText());
      link.setLeftObjectId(objectSelectorLeft.getObjectId());
      if (objectSelectorLeft.getObject() instanceof Rack)
      {
         link.setLeftPatchPanelId(patchPanelSelectorLeft.getPatchPanelId());
         link.setLeftPortNumber(portLeft.getSelection());
         link.setLeftFront(sideLeft.getSelectionIndex() == 0);
      }
      link.setRightObjectId(objectSelectorRight.getObjectId());
      if (objectSelectorRight.getObject() instanceof Rack)
      {
         link.setRightPatchPanelId(patchPanelSelectorRight.getPatchPanelId());
         link.setRightPortNumber(portRight.getSelection());
         link.setRightFront(sideRight.getSelectionIndex() == 0);
      }
      super.okPressed();
   }

   /**
    * Get created/updated link
    * @return link object
    */
   public PhysicalLink getLink()
   {
      return link;
   }
}
