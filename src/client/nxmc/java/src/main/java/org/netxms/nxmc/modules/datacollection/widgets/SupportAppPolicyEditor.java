/**
 * NetXMS - open source network management system
 * Copyright (C) 2019-2023 Raden Solutions
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
package org.netxms.nxmc.modules.datacollection.widgets;

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
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.FileDialog;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.AgentPolicy;
import org.netxms.nxmc.base.widgets.LabeledSpinner;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.base.widgets.SortableTreeViewer;
import org.netxms.nxmc.modules.datacollection.dialogs.MenuItemDialog;
import org.netxms.nxmc.modules.datacollection.views.PolicyEditorView;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.AppMenuItem;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.SupportAppMenuItemLabelProvider;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.SupportAppMenuItemProvider;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.SupportAppPolicy;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.ColorConverter;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Support application editor widget
 */
public class SupportAppPolicyEditor extends AbstractPolicyEditor
{
   private static final Logger logger = LoggerFactory.getLogger(SupportAppPolicy.class);
   
   public static final int NAME = 0;
   public static final int DISCRIPTION = 1;
   public static final int COMMAND = 2;
   public static final int ICON = 3;
   
   private static final int[] IMAGE_SIZE = { 128, 64, 32, 16 };
   private static final Integer[] WINDOW_POSITION_CODES = { null, 0, 0x11, 0x12, 0x13, 0x21, 0x22, 0x23, 0x31, 0x32, 0x33 }; 
   
   private ColorSelector backgroundColor;
   private ColorSelector borderColor;
   private ColorSelector highlightColor;
   private ColorSelector textColor;
   private ColorSelector menuBackgroundColor;
   private ColorSelector menuHighligtColor;
   private ColorSelector menuSelectionColor;
   private ColorSelector menuTextColor;
   private ColorSelector notificationBackgroundColor;
   private ColorSelector notificationHighligtColor;
   private ColorSelector notificationSelectionColor;
   private ColorSelector notificationTextColor;
   private LabeledText desktopWallpaperFile;
   private LabeledText welcomeMessageText;
   private LabeledText tooltipMessageText;
   private Button customColorSchemaCheckbox;
   private Button closeOnDeactivateCheckbox;
   private Combo windowPositioning;
   private LabeledSpinner notificationTimeout;
   private Action addSubMenuAction;
   private Action addItemAction;
   private Action deleteAction;
   private Action editAction;
   private SortableTreeViewer viewer;
   private SupportAppPolicy policyData;
   private Label[] iconBox = new Label[4];
   private Image[] iconPreview = new Image[4];
   private byte[] iconFile = null;

   /**
    * Get window position index from configuration value
    *
    * @param value window position code
    * @return window position index
    */
   private static int windowPositionIndexFromValue(Integer value)
   {
      for(int i = 0; i < WINDOW_POSITION_CODES.length; i++)
         if (value == WINDOW_POSITION_CODES[i])
            return i;
      return 0;   // Undefined for unknown values
   }
  
   /**
    * Constructor
    * 
    * @param parent
    * @param style
    */
   public SupportAppPolicyEditor(Composite parent, int style, AgentPolicy policy, PolicyEditorView view)
   {
      super(parent, style, policy, view);      
      
      try
      {
         policyData = SupportAppPolicy.createFromXml(policy.getContent());
      }
      catch(Exception e)
      {
         logger.error("Cannot parse support application policy XML", e);
         policyData = new SupportAppPolicy();
      }

      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      setLayout(layout);
      
      Composite topArea = new Composite(this,  SWT.NONE);
      layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      layout.numColumns = 2;
      topArea.setLayout(layout);
      topArea.setLayoutData(new GridData(SWT.FILL, SWT.TOP, true, false));
      
      // Image
      createIconSelector(topArea);
      
      Group colorSelectors = new Group(topArea, SWT.NONE);
      colorSelectors.setText("Color schema");
      layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      layout.numColumns = 3;
      layout.makeColumnsEqualWidth = true;
      colorSelectors.setLayout(layout);
      colorSelectors.setLayoutData(new GridData(SWT.FILL, SWT.FILL, false, false));

      GridData gd = new GridData();
      gd.horizontalSpan = layout.numColumns;
      customColorSchemaCheckbox = new Button(colorSelectors, SWT.CHECK);
      customColorSchemaCheckbox.setText("Use custom color schema");
      customColorSchemaCheckbox.setLayoutData(gd);
      customColorSchemaCheckbox.setSelection(policyData.menuBackgroundColor != null);

      backgroundColor = WidgetHelper.createLabeledColorSelector(colorSelectors, "Background", WidgetHelper.DEFAULT_LAYOUT_DATA);    
      textColor = WidgetHelper.createLabeledColorSelector(colorSelectors, "Text", WidgetHelper.DEFAULT_LAYOUT_DATA);      
      borderColor = WidgetHelper.createLabeledColorSelector(colorSelectors, "Border", WidgetHelper.DEFAULT_LAYOUT_DATA);     
      highlightColor = WidgetHelper.createLabeledColorSelector(colorSelectors, "Highlight", WidgetHelper.DEFAULT_LAYOUT_DATA);     
      menuBackgroundColor = WidgetHelper.createLabeledColorSelector(colorSelectors, "Menu background", WidgetHelper.DEFAULT_LAYOUT_DATA);   
      menuTextColor = WidgetHelper.createLabeledColorSelector(colorSelectors, "Menu text", WidgetHelper.DEFAULT_LAYOUT_DATA);
      menuSelectionColor = WidgetHelper.createLabeledColorSelector(colorSelectors, "Menu selection", WidgetHelper.DEFAULT_LAYOUT_DATA);
      menuHighligtColor = WidgetHelper.createLabeledColorSelector(colorSelectors, "Menu highlight", WidgetHelper.DEFAULT_LAYOUT_DATA);
      notificationBackgroundColor = WidgetHelper.createLabeledColorSelector(colorSelectors, "Notification background", WidgetHelper.DEFAULT_LAYOUT_DATA);   
      notificationTextColor = WidgetHelper.createLabeledColorSelector(colorSelectors, "Notification text", WidgetHelper.DEFAULT_LAYOUT_DATA);
      notificationSelectionColor = WidgetHelper.createLabeledColorSelector(colorSelectors, "Notification selection", WidgetHelper.DEFAULT_LAYOUT_DATA);
      notificationHighligtColor = WidgetHelper.createLabeledColorSelector(colorSelectors, "Notification highlight", WidgetHelper.DEFAULT_LAYOUT_DATA);

      Composite messageArea = new Composite(topArea, SWT.NONE);
      messageArea.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false));
      layout = new GridLayout();
      layout.numColumns = 2;
      layout.makeColumnsEqualWidth = true;
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      messageArea.setLayout(layout);

      welcomeMessageText = new LabeledText(messageArea, SWT.NONE, SWT.MULTI | SWT.BORDER);
      welcomeMessageText.setLabel("Welcome message");
      welcomeMessageText.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));

      tooltipMessageText = new LabeledText(messageArea, SWT.NONE, SWT.MULTI | SWT.BORDER);
      tooltipMessageText.setLabel("Tooltip message");
      tooltipMessageText.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));

      Group windowBehaviorGroup = new Group(topArea, SWT.NONE);
      windowBehaviorGroup.setText("Window behavior");
      windowBehaviorGroup.setLayoutData(new GridData(SWT.FILL, SWT.FILL, false, false));
      layout = new GridLayout();
      layout.numColumns = 2;
      windowBehaviorGroup.setLayout(layout);

      closeOnDeactivateCheckbox = new Button(windowBehaviorGroup, SWT.CHECK);
      closeOnDeactivateCheckbox.setText("Close on &deactivate");
      gd = new GridData();
      gd.horizontalSpan = layout.numColumns;
      closeOnDeactivateCheckbox.setLayoutData(gd);

      windowPositioning = WidgetHelper.createLabeledCombo(windowBehaviorGroup, SWT.DROP_DOWN | SWT.READ_ONLY, "Positioning",
            new GridData(SWT.FILL, SWT.BOTTOM, true, false));
      windowPositioning.add("Undefined");
      windowPositioning.add("Automatic");
      windowPositioning.add("Top - Left");
      windowPositioning.add("Top - Center");
      windowPositioning.add("Top - Right");
      windowPositioning.add("Middle - Left");
      windowPositioning.add("Middle - Center");
      windowPositioning.add("Middle - Right");
      windowPositioning.add("Bottom - Left");
      windowPositioning.add("Bottom - Center");
      windowPositioning.add("Bottom - Right");
      windowPositioning.select(0);

      notificationTimeout = new LabeledSpinner(windowBehaviorGroup, SWT.NONE);
      notificationTimeout.setLabel("Notification timeout");
      notificationTimeout.setRange(0, 3600);
      notificationTimeout.setLayoutData(new GridData(SWT.FILL, SWT.BOTTOM, true, false));

      desktopWallpaperFile = new LabeledText(topArea, SWT.NONE);
      desktopWallpaperFile.setLabel("Desktop wallpaper file name");
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      desktopWallpaperFile.setLayoutData(gd);

      final String[] columnNames = { "Name", "Display name", "Command", "Icon" };
      final int[] columnWidths = { 300, 300, 300, 300 };         
      viewer = new SortableTreeViewer(this, columnNames, columnWidths, 0, SWT.UP, SortableTableViewer.DEFAULT_STYLE);
      viewer.setContentProvider(new SupportAppMenuItemProvider());
      viewer.setLabelProvider(new SupportAppMenuItemLabelProvider());

      viewer.getControl().setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
            deleteAction.setEnabled(!selection.isEmpty());
            editAction.setEnabled(!selection.isEmpty());            
         }
      });

      updateControlFromPolicy();      

      IPropertyChangeListener colorChangeListener = new IPropertyChangeListener() {
         @Override
         public void propertyChange(PropertyChangeEvent event)
         {
            fireModifyListeners();
         }
      };
      backgroundColor.addListener(colorChangeListener); 
      textColor.addListener(colorChangeListener);
      borderColor.addListener(colorChangeListener); 
      highlightColor.addListener(colorChangeListener); 
      menuBackgroundColor.addListener(colorChangeListener); 
      menuTextColor.addListener(colorChangeListener); 
      menuHighligtColor.addListener(colorChangeListener); 
      menuSelectionColor.addListener(colorChangeListener);
      notificationBackgroundColor.addListener(colorChangeListener);
      notificationTextColor.addListener(colorChangeListener);
      notificationHighligtColor.addListener(colorChangeListener);
      notificationSelectionColor.addListener(colorChangeListener);

      ModifyListener textModifyListener = new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            fireModifyListeners();
         }
      };
      welcomeMessageText.getTextControl().addModifyListener(textModifyListener);
      tooltipMessageText.getTextControl().addModifyListener(textModifyListener);
      desktopWallpaperFile.getTextControl().addModifyListener(textModifyListener);
      notificationTimeout.getSpinnerControl().addModifyListener(textModifyListener);

      customColorSchemaCheckbox.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            boolean enableSelection = customColorSchemaCheckbox.getSelection();
            backgroundColor.setEnabled(enableSelection);
            textColor.setEnabled(enableSelection);
            borderColor.setEnabled(enableSelection);
            highlightColor.setEnabled(enableSelection);
            menuBackgroundColor.setEnabled(enableSelection);
            menuTextColor.setEnabled(enableSelection);
            menuHighligtColor.setEnabled(enableSelection);
            menuSelectionColor.setEnabled(enableSelection);
            notificationBackgroundColor.setEnabled(enableSelection);
            notificationTextColor.setEnabled(enableSelection);
            notificationHighligtColor.setEnabled(enableSelection);
            notificationSelectionColor.setEnabled(enableSelection);
            fireModifyListeners();
         }
         
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }
      });
      
      SelectionListener selectionListener = new SelectionListener() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            fireModifyListeners();
         }

         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }
      };
      closeOnDeactivateCheckbox.addSelectionListener(selectionListener);
      windowPositioning.addSelectionListener(selectionListener);
      
      createActions();
      createPopupMenu();
   }
   
   /**
    * @param parent
    */
   private void createIconSelector(Composite parent)
   {
      Group group = new Group(parent, SWT.NONE);
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
        policyData.menu.addSubItem(dlg.getItem());
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
      WidgetHelper.setFileDialogFilterExtensions(dlg, new String[] { "*.ico", "*.*" });
      WidgetHelper.setFileDialogFilterNames(dlg, new String[] { "Icon files", "All files" });
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
            logger.error("Exception while updating ", e);
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
      iconFile = policyData.getIcon();
      updateIconPreview();
   }
   
   /**
    * @see org.netxms.nxmc.modules.datacollection.widgets.AbstractPolicyEditor#updateControlFromPolicy()
    */
   @Override
   public void updateControlFromPolicy()
   {             
      updateIconFromPolicy();

      if (policyData.backgroundColor != null)
         backgroundColor.setColorValue(ColorConverter.rgbFromInt(policyData.backgroundColor));
      if (policyData.borderColor != null)
         borderColor.setColorValue(ColorConverter.rgbFromInt(policyData.borderColor));
      if (policyData.highlightColor != null)
         highlightColor.setColorValue(ColorConverter.rgbFromInt(policyData.highlightColor));
      if (policyData.textColor != null)
         textColor.setColorValue(ColorConverter.rgbFromInt(policyData.textColor));
      if (policyData.menuBackgroundColor != null)
         menuBackgroundColor.setColorValue(ColorConverter.rgbFromInt(policyData.menuBackgroundColor));
      if (policyData.menuHighligtColor != null)
         menuHighligtColor.setColorValue(ColorConverter.rgbFromInt(policyData.menuHighligtColor));
      if (policyData.menuSelectionColor != null)
         menuSelectionColor.setColorValue(ColorConverter.rgbFromInt(policyData.menuSelectionColor));
      if (policyData.menuTextColor != null)
         menuTextColor.setColorValue(ColorConverter.rgbFromInt(policyData.menuTextColor));
      if (policyData.notificationBackgroundColor != null)
         notificationBackgroundColor.setColorValue(ColorConverter.rgbFromInt(policyData.notificationBackgroundColor));
      if (policyData.notificationHighligtColor != null)
         notificationHighligtColor.setColorValue(ColorConverter.rgbFromInt(policyData.notificationHighligtColor));
      if (policyData.notificationSelectionColor != null)
         notificationSelectionColor.setColorValue(ColorConverter.rgbFromInt(policyData.notificationSelectionColor));
      if (policyData.notificationTextColor != null)
         notificationTextColor.setColorValue(ColorConverter.rgbFromInt(policyData.notificationTextColor));
      
      customColorSchemaCheckbox.setSelection(policyData.customColorSchema);
      backgroundColor.setEnabled(policyData.customColorSchema);
      textColor.setEnabled(policyData.customColorSchema);
      borderColor.setEnabled(policyData.customColorSchema);
      highlightColor.setEnabled(policyData.customColorSchema);
      menuBackgroundColor.setEnabled(policyData.customColorSchema);
      menuTextColor.setEnabled(policyData.customColorSchema);
      menuHighligtColor.setEnabled(policyData.customColorSchema);
      menuSelectionColor.setEnabled(policyData.customColorSchema);
      notificationBackgroundColor.setEnabled(policyData.customColorSchema);
      notificationTextColor.setEnabled(policyData.customColorSchema);
      notificationHighligtColor.setEnabled(policyData.customColorSchema);
      notificationSelectionColor.setEnabled(policyData.customColorSchema);

      welcomeMessageText.setText(policyData.welcomeMessage != null ? policyData.welcomeMessage : "");
      tooltipMessageText.setText(policyData.tooltipMessage != null ? policyData.tooltipMessage : "");
      
      desktopWallpaperFile.setText(policyData.desktopWallpaper);

      closeOnDeactivateCheckbox.setSelection(policyData.closeOnDeactivate);
      windowPositioning.select(windowPositionIndexFromValue(policyData.mainWindowPosition));
      notificationTimeout.setSelection((policyData.notificationTimeout != null) ? policyData.notificationTimeout / 1000 : 0);

      viewer.setInput(new Object[] { policyData.menu });
   }

   /**
    * @see org.netxms.nxmc.modules.datacollection.widgets.AbstractPolicyEditor#updatePolicyFromControl()
    */
   @Override
   public void updatePolicyFromControl()
   {
      policyData.setIcon((iconFile != null) && (iconFile.length > 0) ? iconFile : null);
      policyData.welcomeMessage = welcomeMessageText.getText();
      policyData.tooltipMessage = tooltipMessageText.getText();

      policyData.desktopWallpaper = desktopWallpaperFile.getText();

      policyData.customColorSchema = customColorSchemaCheckbox.getSelection();
      policyData.backgroundColor = ColorConverter.rgbToInt(backgroundColor.getColorValue());
      policyData.borderColor = ColorConverter.rgbToInt(borderColor.getColorValue());
      policyData.highlightColor = ColorConverter.rgbToInt(highlightColor.getColorValue());
      policyData.textColor = ColorConverter.rgbToInt(textColor.getColorValue());
      policyData.menuBackgroundColor = ColorConverter.rgbToInt(menuBackgroundColor.getColorValue());
      policyData.menuHighligtColor = ColorConverter.rgbToInt(menuHighligtColor.getColorValue());
      policyData.menuSelectionColor = ColorConverter.rgbToInt(menuSelectionColor.getColorValue());
      policyData.menuTextColor = ColorConverter.rgbToInt(menuTextColor.getColorValue());
      policyData.notificationBackgroundColor = ColorConverter.rgbToInt(notificationBackgroundColor.getColorValue());
      policyData.notificationHighligtColor = ColorConverter.rgbToInt(notificationHighligtColor.getColorValue());
      policyData.notificationSelectionColor = ColorConverter.rgbToInt(notificationSelectionColor.getColorValue());
      policyData.notificationTextColor = ColorConverter.rgbToInt(notificationTextColor.getColorValue());

      policyData.closeOnDeactivate = closeOnDeactivateCheckbox.getSelection();
      policyData.mainWindowPosition = WINDOW_POSITION_CODES[windowPositioning.getSelectionIndex()];
      int timeout = notificationTimeout.getSelection();
      policyData.notificationTimeout = (timeout > 0) ? timeout * 1000 : null;

      try
      {
         policy.setContent(policyData.createXml());
      }
      catch(Exception e)
      {
         policy.setContent("");
         logger.error("Cannot create XML form support app policy configuration", e);
      }
   }
}
