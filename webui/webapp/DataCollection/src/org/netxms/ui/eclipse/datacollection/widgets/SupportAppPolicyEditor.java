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
import java.io.IOException;
import java.io.InputStream;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.preference.ColorSelector;
import org.eclipse.jface.util.IPropertyChangeListener;
import org.eclipse.jface.util.PropertyChangeEvent;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.window.Window;
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
import org.netxms.ui.eclipse.datacollection.widgets.helpers.AppMenuItem;
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
   
   private static final int[] IMAGE_SIZE = { 128, 64, 32, 16 };
   
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
   private Action addSubMenuAction;
   private Action addItemAction;
   private Action deleteAction;
   private Action editAction;
   private SortableTreeViewer viewer;
   private SupportAppPolicy sPolicy;
   private Label[] iconBox = new Label[4];
   private Image[] iconPreview = new Image[4];
   private byte[] iconFile = null;

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
      createIconSelector();
      
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
      setColorSchemaCheckbox.setSelection(sPolicy.menuBackgroundColor != null);
      
      backgroundColor = WidgetHelper.createLabeledColorSelector(colorSelectors, "Background", WidgetHelper.DEFAULT_LAYOUT_DATA);    
      textColor = WidgetHelper.createLabeledColorSelector(colorSelectors, "Text", WidgetHelper.DEFAULT_LAYOUT_DATA);      
      borderColor = WidgetHelper.createLabeledColorSelector(colorSelectors, "Border", WidgetHelper.DEFAULT_LAYOUT_DATA);     
      headerColor = WidgetHelper.createLabeledColorSelector(colorSelectors, "Header text", WidgetHelper.DEFAULT_LAYOUT_DATA);     
      menuBackgroundColor = WidgetHelper.createLabeledColorSelector(colorSelectors, "Menu background", WidgetHelper.DEFAULT_LAYOUT_DATA);   
      menuTextColor = WidgetHelper.createLabeledColorSelector(colorSelectors, "Menu text", WidgetHelper.DEFAULT_LAYOUT_DATA);
      menuHighligtColor = WidgetHelper.createLabeledColorSelector(colorSelectors, "Menu highlight", WidgetHelper.DEFAULT_LAYOUT_DATA);
      menuSelectionColor = WidgetHelper.createLabeledColorSelector(colorSelectors, "Menu selection", WidgetHelper.DEFAULT_LAYOUT_DATA);
      
      welcomeMessageText = WidgetHelper.createLabeledText(this, SWT.MULTI | SWT.BORDER, SWT.DEFAULT,
            "Welcome message", "", WidgetHelper.DEFAULT_LAYOUT_DATA);
      GridData data = new GridData(SWT.FILL, SWT.TOP, true, false);
      data.heightHint = 100;
      welcomeMessageText.setLayoutData(data);
      
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
      
      updateControlsFromPolicy();      

      IPropertyChangeListener listener = new IPropertyChangeListener() {
         @Override
         public void propertyChange(PropertyChangeEvent event)
         {
            fireModifyListeners();
         }
      };
      backgroundColor.addListener(listener); 
      textColor.addListener(listener);
      borderColor.addListener(listener); 
      headerColor.addListener(listener); 
      menuBackgroundColor.addListener(listener); 
      menuTextColor.addListener(listener); 
      menuHighligtColor.addListener(listener); 
      menuSelectionColor.addListener(listener);

      welcomeMessageText.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            fireModifyListeners();
         }
      });  

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
            widgetSelected(e);
         }
      });
      
      createActions();
      createPopupMenu();
   }
   
   /**
    * @param parent
    */
   private void createIconSelector()
   {
      Group group = new Group(this, SWT.NONE);
      group.setText("Application Icon");
      GridData gd = new GridData(SWT.FILL, SWT.FILL, false, false);
      group.setLayoutData(gd);
      
      GridLayout layout = new GridLayout();
      layout.numColumns = 4;
      layout.makeColumnsEqualWidth = true;
      group.setLayout(layout);
   
      for(int i = 0; i < IMAGE_SIZE.length; i++)
         createIconPreviewControl(group, i);

      Button link = new Button(group, SWT.PUSH);
      link.setImage(SharedIcons.IMG_FIND);
      link.setText("Select");
      link.setToolTipText("Select");
      link.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            selectIcon();
         }
      });
      link.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      link = new Button(group, SWT.PUSH);
      link.setImage(SharedIcons.IMG_CLEAR);
      link.setText("Clear");
      link.setToolTipText("Clear");
      link.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            iconFile = null;
            for(int i = 0; i < IMAGE_SIZE.length; i++)
            {
               if (iconPreview[i] != null)
               {
                  iconPreview[i].dispose();
                  iconPreview[i] = null;
               }
               iconBox[i].setImage(null);
            }
            fireModifyListeners();
         }
      });
      link.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
   }
   
   /**
    * Create icon selector
    * 
    * @param parent
    * @param sizeIndex
    */
   private void createIconPreviewControl(Composite parent, final int sizeIndex)
   {
      Composite selector = new Composite(parent, SWT.NONE);
      GridLayout layout = new GridLayout();
      selector.setLayout(layout);
      layout.numColumns = 2;
      layout.makeColumnsEqualWidth = true;
      selector.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
      
      Label title = new Label(selector, SWT.CENTER);
      title.setText(String.format("%dx%d", IMAGE_SIZE[sizeIndex], IMAGE_SIZE[sizeIndex]));
      GridData gd = new GridData(SWT.CENTER, SWT.FILL, true, false);
      gd.horizontalSpan = 2;
      title.setLayoutData(gd);
      
      iconBox[sizeIndex] = new Label(selector, SWT.CENTER | SWT.BORDER);
      gd = new GridData(SWT.FILL, SWT.FILL, true, true);
      gd.widthHint = 128;
      gd.heightHint = 128;
      gd.horizontalSpan = 2;
      iconBox[sizeIndex].setLayoutData(gd);
   }
   
   /**
    * Create actions
    */
   protected void createActions()
   {
      addSubMenuAction = new Action("Add sub menu", SharedIcons.ADD_OBJECT) {
         @Override
         public void run()
         {
            createMenuItem(true);
         }
      };
      
      addItemAction = new Action("Add item", SharedIcons.ADD_OBJECT) {
         @Override
         public void run()
         {
            createMenuItem(false);
         }
      };
      
      editAction = new Action("Edit", SharedIcons.EDIT) {
         @Override
         public void run()
         {
            editMenuItem();
         }
      };

      deleteAction = new Action("Delete", SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            deleteMenuItems();
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
            mgr.add(addSubMenuAction);
            mgr.add(addItemAction);
            mgr.add(deleteAction);
            mgr.add(editAction);
         }
      });

      // Create menu
      Menu menu = menuMgr.createContextMenu(viewer.getControl());
      viewer.getControl().setMenu(menu);
   }

   /**
    * Delete selected menu items
    */
   private void deleteMenuItems()
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if (selection.isEmpty())
         return;
      for(Object o : selection.toList())
      {
         ((AppMenuItem)o).delete();
      }
      fireModifyListeners();
   }

   /**
    * Edit menu item
    */
   private void editMenuItem()
   { 
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if (selection.size() != 1)
         return;
      
      AppMenuItem menuItem = (AppMenuItem)selection.getFirstElement();
      MenuItemDialog dlg = new MenuItemDialog(getShell(), menuItem);
      if (dlg.open() != Window.OK)
         return;      

      viewer.refresh(menuItem);
      fireModifyListeners();
   }
   
   /**
    * Create new menu item
    * 
    * @param isSubMenu true if new item is a sub-menu
    */
   private void createMenuItem(boolean isSubMenu)
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      
      MenuItemDialog dlg = new MenuItemDialog(getShell(), isSubMenu);
      if (dlg.open() != Window.OK)
         return;
      
      if (selection.isEmpty())
        sPolicy.menu.addSubItem(dlg.getItem());
      else
         ((AppMenuItem)selection.getFirstElement()).addSubItem(dlg.getItem());

      viewer.refresh();
      fireModifyListeners();
   }
   
   /**
    * Icon selection action
    */
   private void selectIcon()
   {
      FileDialog dlg = new FileDialog(getShell(), SWT.OPEN);
      dlg.setFilterExtensions(new String[] { "*.ico", "*.*" }); //$NON-NLS-1$ //$NON-NLS-2$
      //dlg.setFilterNames(new String[] { "Icon files", "All files" });
      String fileName = dlg.open();
      if (fileName == null)
         return;
      
      InputStream in = null;
      try
      {
         in = new FileInputStream(new File(fileName));
         ByteArrayOutputStream out = new ByteArrayOutputStream();
         byte[] buffer = new byte[8192];
         int bytes;
         while((bytes = in.read(buffer)) > 0)
            out.write(buffer, 0, bytes);
         iconFile = out.toByteArray();

         updateIconPreview();
         fireModifyListeners();
      }
      catch(Exception e)
      {
         MessageDialogHelper.openError(getShell(), "Error", String.format("Cannot load image: %s", e.getLocalizedMessage()));
      }
      finally
      {
         if (in != null)
         {
            try
            {
               in.close();
            }
            catch(IOException e)
            {
            }
         }
      }
   }
   
   /**
    * Update icon preview
    */
   private void updateIconPreview()
   {
      for(int i = 0; i < IMAGE_SIZE.length; i++)
      {
         if (iconPreview[i] != null)
         {
            iconPreview[i].dispose();
            iconPreview[i] = null;
         }
      }
      if ((iconFile != null) && (iconFile.length != 0))
      {
         ByteArrayInputStream input = new ByteArrayInputStream(iconFile);
         try
         {
            ImageLoader loader = new ImageLoader();
            ImageData[] data = loader.load(input);
            for(int i = 0; i < data.length; i++)
            {
               for(int j = 0; j < IMAGE_SIZE.length; j++)
               {
                  int size = IMAGE_SIZE[j];
                  if ((data[i].width == size) && (data[i].height == size))
                  {
                     if (iconPreview[j] != null)
                        iconPreview[j].dispose();
                     iconPreview[j] = new Image(getDisplay(), data[i]);
                     break;
                  }
               }
            }
         }
         catch(Exception e)
         {
            Activator.logError("Exception while updating ", e); //$NON-NLS-1$
         }
      }
      
      for(int i = 0; i < IMAGE_SIZE.length; i++)
         iconBox[i].setImage(iconPreview[i]);
   }
   
   /**
    * Create preview images
    */
   private void updateIconFromPolicy()
   {
      iconFile = sPolicy.getIcon();
      updateIconPreview();
   }
   
   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.datacollection.widgets.AbstractPolicyEditor#updateControlsFromPolicy()
    */
   @Override
   protected void updateControlsFromPolicy()
   {             
      updateIconFromPolicy();

      if (sPolicy.backgroundColor != null)
         backgroundColor.setColorValue(ColorConverter.rgbFromInt(sPolicy.backgroundColor));
      if (sPolicy.borderColor != null)
         borderColor.setColorValue(ColorConverter.rgbFromInt(sPolicy.borderColor));
      if (sPolicy.headerColor != null)
         headerColor.setColorValue(ColorConverter.rgbFromInt(sPolicy.headerColor));
      if (sPolicy.textColor != null)
         textColor.setColorValue(ColorConverter.rgbFromInt(sPolicy.textColor));
      if (sPolicy.menuBackgroundColor != null)
         menuBackgroundColor.setColorValue(ColorConverter.rgbFromInt(sPolicy.menuBackgroundColor));
      if (sPolicy.menuHighligtColor != null)
         menuHighligtColor.setColorValue(ColorConverter.rgbFromInt(sPolicy.menuHighligtColor));
      if (sPolicy.menuSelectionColor != null)
         menuSelectionColor.setColorValue(ColorConverter.rgbFromInt(sPolicy.menuSelectionColor));
      if (sPolicy.menuTextColor != null)
         menuTextColor.setColorValue(ColorConverter.rgbFromInt(sPolicy.menuTextColor));

      welcomeMessageText.setText(sPolicy.welcomeMessage != null ? sPolicy.welcomeMessage : "");

      viewer.setInput(new Object[] { sPolicy.menu });
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.datacollection.widgets.AbstractPolicyEditor#getUpdatedPolicy()
    */
   @Override
   public AgentPolicy getUpdatedPolicy()
   {
      sPolicy.setIcon((iconFile != null) && (iconFile.length > 0) ? iconFile : null);
      sPolicy.welcomeMessage = welcomeMessageText.getText();

      if (setColorSchemaCheckbox.getSelection())
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
      }
      catch(Exception e)
      {
         policy.setContent("");
         e.printStackTrace();
      }
      return policy;
   }
}
