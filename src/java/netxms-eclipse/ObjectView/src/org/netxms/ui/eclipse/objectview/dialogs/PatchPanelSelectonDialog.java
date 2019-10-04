package org.netxms.ui.eclipse.objectview.dialogs;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.constants.RackElementType;
import org.netxms.client.objects.Rack;
import org.netxms.client.objects.configs.PassiveRackElement;
import org.netxms.ui.eclipse.objectview.Activator;
import org.netxms.ui.eclipse.objectview.dialogs.helpers.RackPassiveElementComparator;
import org.netxms.ui.eclipse.objectview.dialogs.helpers.RackPassiveElementLabelProvider;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

public class PatchPanelSelectonDialog extends Dialog
{
   public static final int COLUMN_NAME = 0;
   public static final int COLUMN_TYPE = 1;
   public static final int COLUMN_POSITION = 2;
   public static final int COLUMN_ORIENTATION = 3;
   
   private static final String CONFIG_PREFIX = "PatchPanel"; //$NON-NLS-1$

   private SortableTableViewer viewer;
   private List<PassiveRackElement> passiveElements;
   private long id; 
   
   public PatchPanelSelectonDialog(Shell parentShell, Rack rack)
   {
      super(parentShell);
      passiveElements = new ArrayList<PassiveRackElement>();
      for(PassiveRackElement el : rack.getPassiveElements())
         if (el.getType() == RackElementType.PATCH_PANEL)
            passiveElements.add(el);
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);
      
      dialogArea.setLayout(new FormLayout());

      
      final String[] columnNames = { "Name", "Type", "Position", "Orientation" };
      final int[] columnWidths = { 150, 100, 70, 30 };
      viewer = new SortableTableViewer(dialogArea, columnNames, columnWidths, 0, SWT.UP,
                                       SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new RackPassiveElementLabelProvider());
      viewer.setComparator(new RackPassiveElementComparator());
      //label provider
      //content comparator
      FormData fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(100, 0);
      fd.right = new FormAttachment(100, 0);
      fd.bottom = new FormAttachment(100, 0);
      //viewer.setLayoutData(fd);     
      viewer.setInput(passiveElements.toArray());
      
      return dialogArea;
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText("Patch panel selector");
      
      IDialogSettings settings = Activator.getDefault().getDialogSettings();
      try
      {
         newShell.setSize(settings.getInt(CONFIG_PREFIX + ".cx"), settings.getInt(CONFIG_PREFIX + ".cy")); //$NON-NLS-1$ //$NON-NLS-2$
      }
      catch(NumberFormatException e)
      {
      }
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {      
      final IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      final PassiveRackElement element = (PassiveRackElement)selection.getFirstElement();
      id = element.getId();
      super.okPressed();
   }
      
   /* (non-Javadoc)
    * @see org.eclipse.jface.dialogs.Dialog#buttonPressed(int)
    */
   @Override
   protected void buttonPressed(int buttonId)
   {
      Point size = getShell().getSize();
      IDialogSettings settings = Activator.getDefault().getDialogSettings();

      settings.put(CONFIG_PREFIX + ".cx", size.x); //$NON-NLS-1$
      settings.put(CONFIG_PREFIX + ".cy", size.y); //$NON-NLS-1$
      
      super.buttonPressed(buttonId);
   }

   /**
    * Get selected patch panel id
    * @return patch panel id
    */
   public long getPatchPanelId()
   {
      return id;
   }
}
