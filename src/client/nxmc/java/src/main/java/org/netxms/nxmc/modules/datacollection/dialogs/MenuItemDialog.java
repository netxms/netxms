package org.netxms.nxmc.modules.datacollection.dialogs;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.ImageData;
import org.eclipse.swt.graphics.ImageLoader;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.FileDialog;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.AppMenuItem;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;

public class MenuItemDialog extends Dialog
{
   boolean isSubMenu;
   AppMenuItem item;
   private Text textName;
   private Text textDescription;
   private Text textCommand;
   private Image icon;
   private Label iconLabel;
   
   public MenuItemDialog(Shell parentShell, AppMenuItem item)
   {
      super(parentShell);
      this.isSubMenu = item.isSubMenu();
      this.item = item;
   }
   
   public MenuItemDialog(Shell parentShell, boolean isSubMenu)
   {
      super(parentShell);
      this.isSubMenu = isSubMenu;
      this.item = null;
   }
   
   /* (non-Javadoc)
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText((item != null) ? "Edit Application Menu Item" : "Create Application Menu Item");
   }
   
   /* (non-Javadoc)
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      GridLayout layout = new GridLayout();
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      dialogArea.setLayout(layout);
      
      textName = WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER, SWT.DEFAULT,
            "Name", item != null ? item.getName() : "", WidgetHelper.DEFAULT_LAYOUT_DATA);
      textName.getShell().setMinimumSize(300, 0);
      textName.setTextLimit(63);
      
      textDescription = WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER, SWT.DEFAULT,
            "Description", item != null ? item.getDescription() : "", WidgetHelper.DEFAULT_LAYOUT_DATA);
      textDescription.getShell().setMinimumSize(300, 0);
      
      if (!isSubMenu)
      {
         textCommand = WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER, SWT.DEFAULT,
               "Command", item != null ? item.getCommand() : "", WidgetHelper.DEFAULT_LAYOUT_DATA);
         textCommand.getShell().setMinimumSize(300, 0);
      }

      createIconSelector(dialogArea);      
      textName.setFocus();
      
      return dialogArea;
   }
   
   /**
    * @param parent
    */
   private void createIconSelector(Composite parent)
   {
      Group group = new Group(parent, SWT.NONE);
      group.setText("Image");
      GridData gd = new GridData(SWT.FILL, SWT.FILL, false, false);
      group.setLayoutData(gd);
      
      GridLayout layout = new GridLayout();
      layout.numColumns = 3;
      group.setLayout(layout);
      
      iconLabel = new Label(group, SWT.CENTER);
      iconLabel.setImage(icon);
      gd = new GridData(SWT.FILL, SWT.FILL, true, true);
      gd.widthHint = 16;
      gd.heightHint = 16;
      iconLabel.setLayoutData(gd);
      
      Button link = new Button(group, SWT.PUSH);
      link.setImage(SharedIcons.IMG_FIND);
      link.setToolTipText("Select");
      link.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            selectIcon();
         }
      });

      link = new Button(group, SWT.PUSH);
      link.setImage(SharedIcons.IMG_CLEAR);
      link.setToolTipText("Clear");
      link.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            iconLabel.setImage(SharedIcons.IMG_EMPTY);
            if (icon != null)
            {
               icon.dispose();
               icon = null;
            }
         }
      });
   }
   
   /**
    * Icon selection action
    */
   private void selectIcon()
   {
      FileDialog dlg = new FileDialog(getShell(), SWT.OPEN);
      WidgetHelper.setFileDialogFilterExtensions(dlg, new String[] { "*.gif;*.jpg;*.png", "*.*" });
      WidgetHelper.setFileDialogFilterNames(dlg, new String[] { "Image files", "All files" });
      String fileName = dlg.open();
      if (fileName == null)
         return;
      
      try
      {
         Image image = new Image(getShell().getDisplay(), new FileInputStream(new File(fileName)));
         if ((image.getImageData().width <= 16) && (image.getImageData().height <= 16))
         { 
            if (icon != null)
               icon.dispose();
            icon = image;
            iconLabel.setImage(icon);
         }
         else
         {
            image.dispose();
            MessageDialogHelper.openError(getShell(), "Error", "Image is too large");
         }
      }
      catch(Exception e)
      {
         MessageDialogHelper.openError(getShell(),"Error", String.format("Can not load image %s", e.getLocalizedMessage()));
      }
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      if (item == null)
         item = new AppMenuItem(isSubMenu); 
      item.setName(textName.getText().trim());
      item.setDescription(textDescription.getText().trim());
      
      if ((icon != null) && (icon.getImageData() != null))
      {
         ImageLoader loader = new ImageLoader();
         loader.data = new ImageData[] { icon.getImageData() };
         ByteArrayOutputStream stream = new ByteArrayOutputStream(1024);
         loader.save(stream, SWT.IMAGE_PNG);
         item.setIcon(stream.toByteArray());
      }
      if (!isSubMenu)
      {
         item.setCommand(textCommand.getText().trim());
      }
      //Check that not empty
      super.okPressed();
   }

   public AppMenuItem getItem()
   {
      return item;
   }
}
