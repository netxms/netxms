/**
 * NetXMS - open source network management system
 * Copyright (C) 2019 Raden Solutions
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
package org.netxms.ui.eclipse.datacollection.widgets;

import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.util.ArrayList;
import org.eclipse.jface.preference.ColorSelector;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.ImageData;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.FileDialog;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Label;
import org.netxms.client.objects.AgentPolicy;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.datacollection.Activator;
import org.netxms.ui.eclipse.datacollection.widgets.helpers.FolderMenuItem;
import org.netxms.ui.eclipse.datacollection.widgets.helpers.GenericMenuItem;
import org.netxms.ui.eclipse.datacollection.widgets.helpers.MenuItem;
import org.netxms.ui.eclipse.datacollection.widgets.helpers.SupportAppMenuItemLabelProvider;
import org.netxms.ui.eclipse.datacollection.widgets.helpers.SupportAppMenuItemProvider;
import org.netxms.ui.eclipse.datacollection.widgets.helpers.SupportAppPolicy;
import org.netxms.ui.eclipse.tools.ColorConverter;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;
import org.netxms.ui.eclipse.widgets.SortableTreeViewer;

/**
 * Generic policy editor widget
 */
public class SupportAppPolicyEditor extends AbstractPolicyEditor
{
   public static final int NAME = 0;
   public static final int DICPLAY_NAME = 1;
   public static final int COMMAND = 2;
   public static final int ICON = 3;
   
   private ColorSelector backgroundColor;
   private ColorSelector normalForegroundColor;
   private ColorSelector highForegroundColor;
   private ColorSelector menuBackgroundColor;
   private ColorSelector menuForegroundColor;
   private LabeledText welcomeMessageText;
   private SortableTreeViewer viewer;
   private SupportAppPolicy sPolicy;
   private Label iconLabel;
   private Image icon;
   
   

   /**
    * Constructor
    * 
    * @param parent
    * @param style
    */
   public SupportAppPolicyEditor(Composite parent, int style, AgentPolicy policy)
   {
      super(parent, style);      
      this.policy = policy;     
      
      try
      {
         sPolicy = SupportAppPolicy.createFromXml(policy.getContent());
      }
      catch(Exception e)
      {
         sPolicy = new SupportAppPolicy();
         //print error of parsing in the log
      }

      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.numColumns = 2;
      setLayout(layout);
      
      // Image
      createIconSelector(this);
      
      Composite colorSelectors = new Composite(this, SWT.NONE);
      layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.numColumns = 3;
      layout.makeColumnsEqualWidth = true;
      colorSelectors.setLayout(layout);
      colorSelectors.setLayoutData(new GridData(SWT.FILL, SWT.TOP, false, false));
      
      GridData gd = new GridData();
      backgroundColor = WidgetHelper.createLabeledColorSelector(colorSelectors, "Background", WidgetHelper.DEFAULT_LAYOUT_DATA);
      
      gd = new GridData();
      normalForegroundColor = WidgetHelper.createLabeledColorSelector(colorSelectors, "Normal foreground", WidgetHelper.DEFAULT_LAYOUT_DATA);
      
      gd = new GridData();
      highForegroundColor = WidgetHelper.createLabeledColorSelector(colorSelectors, "High foreground", WidgetHelper.DEFAULT_LAYOUT_DATA);
      
      gd = new GridData();
      menuBackgroundColor = WidgetHelper.createLabeledColorSelector(colorSelectors, "Menu background", WidgetHelper.DEFAULT_LAYOUT_DATA);
      
      gd = new GridData();
      menuForegroundColor = WidgetHelper.createLabeledColorSelector(colorSelectors, "Menu foreground", WidgetHelper.DEFAULT_LAYOUT_DATA);
      
      //ListOfActions
      final String[] columnNames = { "Name", "Display name", "Command", "Icon" };
      final int[] columnWidths = { 300, 300, 300, 300 };         
      viewer = new SortableTreeViewer(this, columnNames, columnWidths, 0, SWT.UP, SortableTableViewer.DEFAULT_STYLE);
      viewer.setContentProvider(new SupportAppMenuItemProvider());
      viewer.setLabelProvider(new SupportAppMenuItemLabelProvider());

      gd = new GridData(SWT.FILL, SWT.FILL, true, true);
      gd.horizontalSpan = 5;
      viewer.getControl().setLayoutData(gd);
      
      ArrayList<GenericMenuItem> input = new ArrayList<GenericMenuItem>();
      FolderMenuItem folder = new FolderMenuItem("Email actions");
      folder.setIcon(new byte[10]);
      input.add(folder);
      MenuItem item = new MenuItem("TechMail", "Send mail to support", "mailto:tech@company.com", folder);
      item.setIcon(new byte[10]);
      folder.addChild(item);
      item = new MenuItem("ManagerMail", "Send mail to manager", "mailto:manager@company.com", folder);
      item.setIcon(new byte[10]);
      folder.addChild(item);
      input.add(new MenuItem("Test", "Test item", "http://netxms.org"));
      viewer.setInput(input);
      //comparator
      
      //add actions 
      sPolicy.menuItems = input.toArray(new GenericMenuItem[input.size()]);
      sPolicy.backgroundColor = ColorConverter.rgbToInt(new RGB(10, 0, 50));
      sPolicy.highForegroundColor = ColorConverter.rgbToInt(new RGB(10, 0, 50));
      sPolicy.menuBackgroundColor = ColorConverter.rgbToInt(new RGB(10, 0, 50));
      sPolicy.menuForegroundColor = ColorConverter.rgbToInt(new RGB(10, 0, 50));
      sPolicy.normalForegroundColor = ColorConverter.rgbToInt(new RGB(10, 0, 50));
      sPolicy.welcomeMessage = "Test welcome message";
      sPolicy.setLogo(new byte[10]);
      
      try
      {
         System.out.println(sPolicy.createXml());
      }
      catch(Exception e)
      {
         // TODO Auto-generated catch block
         e.printStackTrace();
      }
      
      refresh();
   }
   

   
   /**
    * Create icon
    */
   private void createIcon()
   {
      if (icon != null) 
      {
         icon.dispose();
         icon = null;
      }
      
      byte[] imageBytes = null; //objectTool.getImageData();
      if ((imageBytes == null) || (imageBytes.length == 0))
         return;
      
      ByteArrayInputStream input = new ByteArrayInputStream(imageBytes);
      try
      {
         ImageDescriptor d = ImageDescriptor.createFromImageData(new ImageData(input));
         icon = d.createImage();
      }
      catch(Exception e)
      {
         Activator.logError("Exception in General.createIcon()", e); //$NON-NLS-1$
      }
   }
   
   /**
    * @param parent
    */
   private void createIconSelector(Composite parent)
   {
      Group group = new Group(parent, SWT.NONE);
      group.setText("Logo");
      GridData gd = new GridData(SWT.FILL, SWT.FILL, false, false);
      group.setLayoutData(gd);
      
      GridLayout layout = new GridLayout();
      layout.numColumns = 3;
      group.setLayout(layout);
      
      iconLabel = new Label(group, SWT.CENTER);
      iconLabel.setImage(icon);
      gd = new GridData(SWT.FILL, SWT.FILL, true, true);
      gd.widthHint = 256;
      gd.heightHint = 256;
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
    * 
    */
   private void selectIcon()
   {
      FileDialog dlg = new FileDialog(getShell(), SWT.OPEN);
      dlg.setFilterExtensions(new String[] { "*.gif;*.jpg;*.png", "*.*" }); //$NON-NLS-1$ //$NON-NLS-2$
      dlg.setFilterNames(new String[] { "Image files", "All files" });
      String fileName = dlg.open();
      if (fileName == null)
         return;
      
      try
      {
         Image image = new Image(getShell().getDisplay(), new FileInputStream(new File(fileName)));
         //if ((image.getImageData().width <= 16) && (image.getImageData().height <= 16))
         //{ check on how large immage should be
            if (icon != null)
               icon.dispose();
            icon = image;
            iconLabel.setImage(icon);
         /*}
         else
         {
            image.dispose();
            MessageDialogHelper.openError(getShell(), "Error", "Image is too large");
         }*/
      }
      catch(Exception e)
      {
         MessageDialogHelper.openError(getShell(),"Error", String.format("Can not load image %s", e.getLocalizedMessage()));
      }
   }
   
   protected void refresh()
   {
   }

   public AgentPolicy getUpdatedPolicy()
   { 
      return policy;
   }
}
