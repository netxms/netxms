/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2017 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.datacollection.dialogs;

import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.DataOrigin;
import org.netxms.client.constants.DataType;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Template;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.dialogs.helpers.StringFilter;
import org.netxms.nxmc.modules.objects.dialogs.ObjectSelectionDialog;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog for selecting properties provided by SM-CLP
 */
public class SelectSmclpDlg extends Dialog implements IParameterSelectionDialog
{
   private final I18n i18n = LocalizationHelper.getI18n(SelectSmclpDlg.class);
   private final static String SETTINGS_PREFIX =  "SelectAgentParamDlg." + DataOrigin.SMCLP;
   
   private Button queryButton;
   private Action actionQuery;
   private AbstractObject queryObject;
   protected boolean selectTables;
   protected AbstractObject object;
   protected Text filterText;
   protected SortableTableViewer viewer;
   
   protected Object selection;
   protected StringFilter filter;
   

   /**
    * @param parentShell
    * @param nodeId
    */
   public SelectSmclpDlg(Shell parentShell, long nodeId)
   {
      super(parentShell);
      setShellStyle(getShellStyle() | SWT.RESIZE);
      object = Registry.getSession().findObjectById(nodeId);
      queryObject = object;

      actionQuery = new Action(i18n.tr("&Query...")) {
         @Override
         public void run()
         {
            querySelectedParameter();
         }
      };
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(i18n.tr("Parameter Selection"));
      PreferenceStore settings = PreferenceStore.getInstance();
      try
      {
         newShell.setSize(settings.getAsPoint(SETTINGS_PREFIX + ".size", 600, 400)); //$NON-NLS-1$ //$NON-NLS-2$
      }
      catch(NumberFormatException e)
      {
      }
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);
   
      GridLayout layout = new GridLayout();
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
      dialogArea.setLayout(layout);
      
      Label label = new Label(dialogArea, SWT.NONE);
      label.setText(selectTables ? i18n.tr("Available tables") : i18n.tr("Available parameters"));
      
      filterText = new Text(dialogArea, SWT.BORDER);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      filterText.setLayoutData(gd);
      filterText.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            filter.setFilter(filterText.getText());
            viewer.refresh(false);
         }
      });

      final String[] names = { i18n.tr("Property") };
      final int[] widths = { 350 };
      viewer = new SortableTableViewer(dialogArea, names, widths, 0, SWT.UP, SWT.FULL_SELECTION | SWT.BORDER);
      WidgetHelper.restoreTableViewerSettings(viewer, SETTINGS_PREFIX + ".viewer"); //$NON-NLS-1$
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new LabelProvider());
      viewer.setComparator(new ViewerComparator());
      
      filter = new StringFilter();
      viewer.addFilter(filter);
      
      viewer.getTable().addMouseListener(new MouseListener() {
         @Override
         public void mouseDoubleClick(MouseEvent e)
         {
            okPressed();
         }

         @Override
         public void mouseDown(MouseEvent e)
         {
         }

         @Override
         public void mouseUp(MouseEvent e)
         {
         }
      });
      
      viewer.getTable().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveTableViewerSettings(viewer, SETTINGS_PREFIX);
         }
      });

      createPopupMenu();
      
      gd = new GridData();
      gd.heightHint = 250;
      gd.grabExcessVerticalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      viewer.getControl().setLayoutData(gd);
      
      fillList();
      
      return dialogArea;
   }
   
   /**
    * Create pop-up menu
    */
   private void createPopupMenu()
   {
      // Create menu manager.
      MenuManager menuMgr = new MenuManager();
      menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener(new IMenuListener() {
         public void menuAboutToShow(IMenuManager mgr)
         {
            mgr.add(actionQuery);
         }
      });

      // Create menu
      Menu menu = menuMgr.createContextMenu(viewer.getControl());
      viewer.getControl().setMenu(menu);
   }
   
   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
      {
         MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("You must select parameter from the list!"));
         return;
      }
      this.selection = selection.getFirstElement();
      saveSettings();
      super.okPressed();
   }
   
   /**
    * @see org.eclipse.jface.dialogs.Dialog#cancelPressed()
    */
   @Override
   protected void cancelPressed()
   {
      saveSettings();
      super.cancelPressed();
   }

   /**
    * Save dialog settings
    */
   protected void saveSettings()
   {
      Point size = getShell().getSize();
      PreferenceStore settings = PreferenceStore.getInstance();

      settings.set(SETTINGS_PREFIX + ".size", size); 
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createButtonsForButtonBar(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createButtonsForButtonBar(Composite parent)
   {
      ((GridLayout)parent.getLayout()).numColumns++;

      queryButton = new Button(parent, SWT.PUSH);
      queryButton.setText(i18n.tr("&Query..."));
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      queryButton.setLayoutData(gd);
      queryButton.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            querySelectedParameter();
         }

         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }
      });
      super.createButtonsForButtonBar(parent);
   }

   /**
    * Fill list with parameters
    */
   protected void fillList()
   {
      final NXCSession session = Registry.getSession();
      new Job(String.format(i18n.tr("Get list of supported properties for "), object.getObjectName()), null) {
         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Unable to retrieve list of supported properties");
         }

         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final List<String> properties = session.getSmclpSupportedProperties(object.getObjectId());
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  viewer.setInput(properties.toArray());
               }
            });
         }
      }.start();
   }

   /**
    * Query current value of selected parameter
    */
   protected void querySelectedParameter()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;
      
      // Opens Object Selection Dialog if object is not chosen
      if (queryObject instanceof Template)
      {
         final ObjectSelectionDialog sDlg = new ObjectSelectionDialog(getShell(), ObjectSelectionDialog.createNodeSelectionFilter(false));
         sDlg.enableMultiSelection(false);
         
         if (sDlg.open() == Window.OK)
         {
            queryObject = sDlg.getSelectedObjects().get(0);
         }
      }

      String n = (String)selection.getFirstElement();
      final NXCSession session = Registry.getSession();
      final String name = n;
      new Job(i18n.tr("Query SM-CLP"), null) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            try
            {
               final String value = session.queryMetric(queryObject.getObjectId(), DataOrigin.SMCLP, name);
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     MessageDialogHelper.openInformation(getShell(), i18n.tr("Current value"),
                           String.format(i18n.tr("Current value is \"%s\""), value));
                  }
               });
            }
            catch (NXCException e)
            {
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {

                     MessageDialogHelper.openInformation(getShell(), i18n.tr("Current value"),
                           String.format(i18n.tr("Cannot get current parameter value: %s"), e.getMessage()));
                     
                  }
               });
               throw e;
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot get current parameter value");
         }
      }.start();
   }

   /**
    * @see org.netxms.ui.eclipse.datacollection.dialogs.IParameterSelectionDialog#getParameterDataType()
    */
   @Override
   public DataType getParameterDataType()
   {
      return DataType.STRING;
   }

   /**
    * @see org.netxms.ui.eclipse.datacollection.dialogs.IParameterSelectionDialog#getParameterDescription()
    */
   @Override
   public String getParameterDescription()
   {
      return (String)selection;
   }

   /**
    * @see org.netxms.ui.eclipse.datacollection.dialogs.IParameterSelectionDialog#getParameterName()
    */
   @Override
   public String getParameterName()
   {
      return (String)selection;
   }
   
   /**
    * @see org.netxms.ui.eclipse.datacollection.dialogs.IParameterSelectionDialog#getInstanceColumn()
    */
   @Override
   public String getInstanceColumn()
   {
      return ""; 
   }
}
