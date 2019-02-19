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
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.preference.ColorSelector;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.jface.util.IPropertyChangeListener;
import org.eclipse.jface.util.PropertyChangeEvent;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.ImageData;
import org.eclipse.swt.graphics.ImageLoader;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.FileDialog;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.objects.AgentPolicy;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.datacollection.Activator;
import org.netxms.ui.eclipse.datacollection.dialogs.MenuItemDialog;
import org.netxms.ui.eclipse.datacollection.widgets.helpers.FolderMenuItem;
import org.netxms.ui.eclipse.datacollection.widgets.helpers.GenericMenuItem;
import org.netxms.ui.eclipse.datacollection.widgets.helpers.SupportAppMenuItemLabelProvider;
import org.netxms.ui.eclipse.datacollection.widgets.helpers.SupportAppMenuItemProvider;
import org.netxms.ui.eclipse.datacollection.widgets.helpers.SupportAppPolicy;
import org.netxms.ui.eclipse.tools.ColorConverter;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;
import org.netxms.ui.eclipse.widgets.SortableTreeViewer;

/**
 * Support application editor widget
 */
public class SupportAppPolicyEditor extends AbstractPolicyEditor
{
   public static final int NAME = 0;
   public static final int DISCRIPTION = 1;
   public static final int COMMAND = 2;
   public static final int ICON = 3;
   
   private ColorSelector backgroundColor;
   private ColorSelector borderColor;
   private ColorSelector headerColor;
   private ColorSelector textColor;
   private ColorSelector menuBackgroundColor;
   private ColorSelector menuHighligtColor;
   private ColorSelector menuSelectionColor;
   private ColorSelector menuTextColor;
   private Text welcomeMessageText;
   private Button setColorSchemaCheckbox;
   private Action addFolderAction;
   private Action addItemAction;
   private Action deleteAction;
   private Action editAction;
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
         System.out.println(policy.getContent());
         sPolicy = SupportAppPolicy.createFromXml(policy.getContent());
      }
      catch(Exception e)
      {
         e.printStackTrace();
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
      gd.horizontalSpan = 3;
      setColorSchemaCheckbox = new Button(colorSelectors, SWT.CHECK);
      setColorSchemaCheckbox.setText("Define color schema");
      setColorSchemaCheckbox.setLayoutData(gd);
      
      IPropertyChangeListener listener = new IPropertyChangeListener() {
         
         @Override
         public void propertyChange(PropertyChangeEvent event)
         {
            fireModifyListeners();
         }
      };
      
      backgroundColor = WidgetHelper.createLabeledColorSelector(colorSelectors, "Background", WidgetHelper.DEFAULT_LAYOUT_DATA);    
      backgroundColor.addListener(listener);  
      textColor = WidgetHelper.createLabeledColorSelector(colorSelectors, "Text", WidgetHelper.DEFAULT_LAYOUT_DATA);      
      textColor.addListener(listener);
      borderColor = WidgetHelper.createLabeledColorSelector(colorSelectors, "Border", WidgetHelper.DEFAULT_LAYOUT_DATA);     
      borderColor.addListener(listener); 
      headerColor = WidgetHelper.createLabeledColorSelector(colorSelectors, "Header text", WidgetHelper.DEFAULT_LAYOUT_DATA);     
      headerColor.addListener(listener); 
      menuBackgroundColor = WidgetHelper.createLabeledColorSelector(colorSelectors, "Menu background", WidgetHelper.DEFAULT_LAYOUT_DATA);   
      menuBackgroundColor.addListener(listener);   
      menuTextColor = WidgetHelper.createLabeledColorSelector(colorSelectors, "Menu text", WidgetHelper.DEFAULT_LAYOUT_DATA);
      menuTextColor.addListener(listener); 
      menuHighligtColor = WidgetHelper.createLabeledColorSelector(colorSelectors, "Menu highlight", WidgetHelper.DEFAULT_LAYOUT_DATA);
      menuHighligtColor.addListener(listener); 
      menuSelectionColor = WidgetHelper.createLabeledColorSelector(colorSelectors, "Menu selection", WidgetHelper.DEFAULT_LAYOUT_DATA);
      menuSelectionColor.addListener(listener);
      
      setColorSchemaCheckbox.addSelectionListener(new SelectionListener() {
         
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            boolean enableSelection = setColorSchemaCheckbox.getSelection();
            backgroundColor.setEnabled(enableSelection);
            textColor.setEnabled(enableSelection);
            borderColor.setEnabled(enableSelection);
            headerColor.setEnabled(enableSelection);
            menuBackgroundColor.setEnabled(enableSelection);
            menuTextColor.setEnabled(enableSelection);
            menuHighligtColor.setEnabled(enableSelection);
            menuSelectionColor.setEnabled(enableSelection);
            fireModifyListeners();
         }
         
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            // TODO Auto-generated method stub
            
         }
      });
      setColorSchemaCheckbox.setSelection(sPolicy.menuBackgroundColor != null);
      
      welcomeMessageText =  WidgetHelper.createLabeledText(this, SWT.SINGLE | SWT.BORDER, SWT.DEFAULT,
            "Welcome message", "", WidgetHelper.DEFAULT_LAYOUT_DATA);
      welcomeMessageText.setLayoutData(new GridData(SWT.FILL, SWT.TOP, true, false));
      welcomeMessageText.addModifyListener(new ModifyListener() {
         
         @Override
         public void modifyText(ModifyEvent e)
         {
            fireModifyListeners();
         }
      });
      
      //ListOfActions
      final String[] columnNames = { "Name", "Display name", "Command", "Icon" };
      final int[] columnWidths = { 300, 300, 300, 300 };         
      viewer = new SortableTreeViewer(this, columnNames, columnWidths, 0, SWT.UP, SortableTableViewer.DEFAULT_STYLE);
      viewer.setContentProvider(new SupportAppMenuItemProvider());
      viewer.setLabelProvider(new SupportAppMenuItemLabelProvider());

      gd = new GridData(SWT.FILL, SWT.FILL, true, true);
      gd.horizontalSpan = 5;
      viewer.getControl().setLayoutData(gd);
      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
         
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
            deleteAction.setEnabled(!selection.isEmpty());
            editAction.setEnabled(!selection.isEmpty());            
         }
      });
      
      createActions();
      createPopupMenu();
      refresh();
      fireModifyListeners();
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
      
      byte[] imageBytes = sPolicy.getLogo();
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
    * Create actions
    */
   protected void createActions()
   {
      addFolderAction = new Action("Add folder", SharedIcons.ADD_OBJECT) {
         @Override
         public void run()
         {
            createObjct(true);
         }
      };
      
      addItemAction = new Action("Add tiem", SharedIcons.ADD_OBJECT) {
         @Override
         public void run()
         {
            createObjct(false);
         }
      };
      
      deleteAction = new Action("Delete", SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            
            deleteObject();
         }
      };
      
      editAction = new Action("Edit", SharedIcons.EDIT) {
         @Override
         public void run()
         {
            editObject();
         }
      };
   }
   



   /**
    * Create pop-up menu for user list
    */
   private void createPopupMenu()
   {
      // Create menu manager
      MenuManager menuMgr = new MenuManager();
      menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener(new IMenuListener() {
         public void menuAboutToShow(IMenuManager mgr)
         {
            mgr.add(addFolderAction);
            mgr.add(addItemAction);
            mgr.add(deleteAction);
            mgr.add(editAction);
         }
      });

      // Create menu
      Menu menu = menuMgr.createContextMenu(viewer.getControl());
      viewer.getControl().setMenu(menu);
   }

   private void deleteObject()
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if (selection.isEmpty())
         return;
      for(Object o : selection.toList())
      {
         sPolicy.deleteMenuItem(((GenericMenuItem)o));
      }
      fireModifyListeners();
   }

   private void editObject()
   { 
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if (selection.isEmpty() || selection.size() > 1)
         return;
      GenericMenuItem obj = (GenericMenuItem)selection.toList().get(0);
      MenuItemDialog dlg = new MenuItemDialog(welcomeMessageText.getShell(), obj instanceof FolderMenuItem, obj);
      int rc = dlg.open();
      if(rc != dlg.OK)
         return;      

      viewer.refresh(obj);
      fireModifyListeners();
   }
   
   private void createObjct(boolean isFolder)
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      
      MenuItemDialog dlg = new MenuItemDialog(welcomeMessageText.getShell(), isFolder, null);
      int rc = dlg.open();
      if(rc != dlg.OK)
         return;
      
      if (selection.isEmpty())
        sPolicy.root.addChild(dlg.getItem());
      else
         ((GenericMenuItem)selection.toList().get(0)).addChild(dlg.getItem());

      viewer.refresh();
      fireModifyListeners();
   }
   
   
   /**
    * Icon selection action
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
         if ((image.getImageData().width <= 256) && (image.getImageData().height <= 256))
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
   
   protected void refresh()
   {             
      createIcon();
      iconLabel.setImage(icon);
      if(sPolicy.backgroundColor != null)
         backgroundColor.setColorValue(ColorConverter.rgbFromInt(sPolicy.backgroundColor));
      if(sPolicy.borderColor != null)
         borderColor.setColorValue(ColorConverter.rgbFromInt(sPolicy.borderColor));
      if(sPolicy.headerColor != null)
         headerColor.setColorValue(ColorConverter.rgbFromInt(sPolicy.headerColor));
      if(sPolicy.textColor != null)
         textColor.setColorValue(ColorConverter.rgbFromInt(sPolicy.textColor));
      if(sPolicy.menuBackgroundColor != null)
         menuBackgroundColor.setColorValue(ColorConverter.rgbFromInt(sPolicy.menuBackgroundColor));
      if(sPolicy.menuHighligtColor != null)
         menuHighligtColor.setColorValue(ColorConverter.rgbFromInt(sPolicy.menuHighligtColor));
      if(sPolicy.menuSelectionColor != null)
         menuSelectionColor.setColorValue(ColorConverter.rgbFromInt(sPolicy.menuSelectionColor));
      if(sPolicy.menuTextColor != null)
         menuTextColor.setColorValue(ColorConverter.rgbFromInt(sPolicy.menuTextColor));
      
      sPolicy.welcomeMessage = welcomeMessageText.getText();
      
      viewer.setInput(new Object[] { sPolicy.root });
   }

   public AgentPolicy getUpdatedPolicy()
   { 
      if (icon != null)
      {
         ImageLoader loader = new ImageLoader();
         loader.data = new ImageData[] { icon.getImageData() };
         ByteArrayOutputStream stream = new ByteArrayOutputStream(1024);
         loader.save(stream, SWT.IMAGE_PNG);
         sPolicy.setLogo(stream.toByteArray());
      }
      else
         sPolicy.setLogo(null);
      
      
      if(setColorSchemaCheckbox.getSelection())
      {
         sPolicy.backgroundColor = ColorConverter.rgbToInt(backgroundColor.getColorValue());
         sPolicy.borderColor = ColorConverter.rgbToInt(borderColor.getColorValue());
         sPolicy.headerColor = ColorConverter.rgbToInt(headerColor.getColorValue());
         sPolicy.textColor = ColorConverter.rgbToInt(textColor.getColorValue());
         sPolicy.menuBackgroundColor = ColorConverter.rgbToInt(menuBackgroundColor.getColorValue());
         sPolicy.menuHighligtColor = ColorConverter.rgbToInt(menuHighligtColor.getColorValue());
         sPolicy.menuSelectionColor = ColorConverter.rgbToInt(menuSelectionColor.getColorValue());
         sPolicy.menuTextColor = ColorConverter.rgbToInt(menuTextColor.getColorValue());
      }
      else
      {
         sPolicy.backgroundColor = null;
         sPolicy.borderColor = null;
         sPolicy.headerColor = null;
         sPolicy.textColor = null;
         sPolicy.menuBackgroundColor = null;
         sPolicy.menuHighligtColor = null;
         sPolicy.menuSelectionColor = null;
         sPolicy.menuTextColor = null;
      }
      
      try
      {
         policy.setContent(sPolicy.createXml());
         System.out.println(policy.getContent());
      }
      catch(Exception e)
      {
         //TODO: make some notifiaction?
         policy.setContent("");
         e.printStackTrace();
      }
      return policy;
   }
}
