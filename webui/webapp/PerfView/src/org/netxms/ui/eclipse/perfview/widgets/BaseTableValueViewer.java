/**
 * 
 */
package org.netxms.ui.eclipse.perfview.widgets;

import java.util.Arrays;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.jface.viewers.ViewerRow;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CLabel;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IViewPart;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.netxms.client.NXCSession;
import org.netxms.client.Table;
import org.netxms.client.TableColumnDefinition;
import org.netxms.client.constants.Severity;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.perfview.Activator;
import org.netxms.ui.eclipse.perfview.widgets.helpers.CellSelectionManager;
import org.netxms.ui.eclipse.perfview.widgets.helpers.TableContentProvider;
import org.netxms.ui.eclipse.perfview.widgets.helpers.TableItemComparator;
import org.netxms.ui.eclipse.perfview.widgets.helpers.TableLabelProvider;
import org.netxms.ui.eclipse.perfview.widgets.helpers.TableValueFilter;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.FilterText;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Base class for displaying table values
 */
public abstract class BaseTableValueViewer extends Composite
{
   protected NXCSession session;
   protected IViewPart viewPart;
   protected String configId;
   protected FilterText filterText;
   protected TableValueFilter filter;
   protected Table currentData = null;
   protected SortableTableViewer viewer;
   protected TableLabelProvider labelProvider;
   protected CLabel errorLabel;
   protected CellSelectionManager cellSelectionManager;
   protected Action actionUseMultipliers;
   protected Action actionShowFilter;

   /**
    * @param parent
    * @param style
    */
   public BaseTableValueViewer(Composite parent, int style, IViewPart viewPart, String configSubId)
   {
      super(parent, style);
      this.viewPart = viewPart;
      
      configId = buildConfigId(configSubId);
      session = ConsoleSharedData.getSession();

      setLayout(new FormLayout());
      
      // Create filter area
      filterText = new FilterText(this, SWT.NONE);
      filterText.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            onFilterModify();
         }
      });
      filterText.setCloseAction(new Action() {
         @Override
         public void run()
         {
            enableFilter(false);
         }
      });

      viewer = new SortableTableViewer(this, SWT.FULL_SELECTION | SWT.MULTI);
      viewer.setContentProvider(new TableContentProvider());
      labelProvider = new TableLabelProvider();
      viewer.setLabelProvider(labelProvider);
      filter = new TableValueFilter();
      viewer.addFilter(filter);
      cellSelectionManager = new CellSelectionManager(viewer);
      
      // Setup layout
      FormData fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(filterText);
      fd.right = new FormAttachment(100, 0);
      fd.bottom = new FormAttachment(100, 0);
      viewer.getControl().setLayoutData(fd);

      fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(0, 0);
      fd.right = new FormAttachment(100, 0);
      filterText.setLayoutData(fd);

      final IDialogSettings ds = Activator.getDefault().getDialogSettings();
      labelProvider.setUseMultipliers(getBoolean(ds, configId + ".useMultipliers", false));
      
      viewer.getTable().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveTableViewerSettings(viewer, ds, configId);
            ds.put(configId + ".initShowfilter", actionShowFilter.isChecked());
         }
      });
      
      createActions();
      createPopupMenu();
      
      // Set initial focus to filter input line
      if (getBoolean(ds, configId + ".initShowfilter", false))
      {
         filterText.setFocus();
         actionShowFilter.setChecked(true);
      }
      else
      {
         enableFilter(false); // Will hide filter area correctly
      }
   }

   /**
    * Build configuration ID
    *  
    * @param configSubId configuration sub-ID
    * @return configuration ID
    */
   protected String buildConfigId(String configSubId)
   {
      return "BaseTableValueViewer." + configSubId;
   }

   /**
    * Create actions
    */
   protected void createActions()
   {
      actionUseMultipliers = new Action("Use &multipliers", Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            labelProvider.setUseMultipliers(actionUseMultipliers.isChecked());
            viewer.refresh(true);
         }
      };
      actionUseMultipliers.setChecked(labelProvider.areMultipliersUsed());
      
      actionShowFilter = new Action() {
         @Override
         public void run()
         {
            enableFilter(actionShowFilter.isChecked());
         }
      };
      actionShowFilter.setText("Show filter");
      actionShowFilter.setActionDefinitionId("org.netxms.ui.eclipse.perfview.commands.show_table_values_filter"); //$NON-NLS-1$
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
            fillContextMenu(mgr);
         }
      });

      // Create menu.
      Menu menu = menuMgr.createContextMenu(viewer.getControl());
      viewer.getControl().setMenu(menu);

      // Register menu for extension
      if (viewPart != null)
      {
         viewPart.getSite().registerContextMenu(menuMgr, viewer);
      }
   }

   /**
    * Fill context menu
    * 
    * @param manager
    */
   protected void fillContextMenu(IMenuManager manager)
   {
      manager.add(actionUseMultipliers);
      manager.add(actionShowFilter);
      manager.add(new Separator());
      manager.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
   }
   
   /**
    * Update viewer with fresh table data
    * 
    * @param table new table DCI data
    */
   protected void updateViewer(final Table table)
   {
      final IDialogSettings ds = Activator.getDefault().getDialogSettings();
      
      if (!viewer.isInitialized())
      {
         final String[] names = table.getColumnDisplayNames();
         final int[] widths = new int[names.length];
         Arrays.fill(widths, 150);
         viewer.createColumns(names, widths, 0, SWT.UP);
         
         WidgetHelper.restoreTableViewerSettings(viewer, Activator.getDefault().getDialogSettings(), configId); //$NON-NLS-1$
         viewer.getTable().addDisposeListener(new DisposeListener() {
            @Override
            public void widgetDisposed(DisposeEvent e)
            {
               WidgetHelper.saveTableViewerSettings(viewer, Activator.getDefault().getDialogSettings(), configId); //$NON-NLS-1$
               ds.put(configId + ".useMultipliers", labelProvider.areMultipliersUsed()); //$NON-NLS-1$
            }
         });
         viewer.setComparator(new TableItemComparator(table.getColumnDataTypes()));        
      }
      
      labelProvider.setColumns(table.getColumns());
      viewer.setInput(table);
      currentData = table;
   }

   /**
    * @param viewerRow
    * @return
    */
   protected String buildInstanceString(ViewerRow viewerRow)
   {
      StringBuilder instance = new StringBuilder();
      boolean first = true;
      for(int i = 0; i < currentData.getColumnCount(); i++)
      {
         TableColumnDefinition cd = currentData.getColumnDefinition(i);
         if (cd.isInstanceColumn())
         {
            if (!first)
               instance.append("~~~"); //$NON-NLS-1$
            instance.append(viewerRow.getText(i));
            first = false;
         }
      }
      return instance.toString();
   }
   
   /**
    * @return
    */
   public String getTitle()
   {
      return (currentData != null) ? currentData.getTitle() : ""; //$NON-NLS-1$
   }

   /**
    * @return
    */
   public SortableTableViewer getViewer()
   {
      return viewer;
   }
   
   /**
    * @return
    */
   public Action getActionUseMultipliers()
   {
      return actionUseMultipliers;
   }
   
   /**
    * Enable or disable filter
    * 
    * @param enable New filter state
    */
   public void enableFilter(boolean enable)
   {
      actionShowFilter.setChecked(enable);
      filterText.setVisible(enable);
      FormData fd = (FormData)viewer.getControl().getLayoutData();
      fd.top = enable ? new FormAttachment(filterText) : new FormAttachment(0, 0);
      layout();
      if (enable)
         filterText.setFocus();
      else
         setFilter(""); //$NON-NLS-1$
   }

   /**
    * Set filter text
    * 
    * @param text New filter text
    */
   public void setFilter(final String text)
   {
      filterText.setText(text);
      onFilterModify();
   }

   /**
    * Get filter text
    * 
    * @return Current filter text
    */
   public String getFilterText()
   {
      return filterText.getText();
   }

   /**
    * Handler for filter modification
    */
   private void onFilterModify()
   {
      final String text = filterText.getText();
      filter.setFilterString(text);
      viewer.refresh(false);
   }
   
   /**
    * Get show filter action
    */
   public Action getShowFilterAction()
   {
      return actionShowFilter;
   }

   /**
    * @param ds IDialogSettings
    * @param s Settings string
    * @param defval default value
    * @return
    */
   protected static boolean getBoolean(IDialogSettings ds, String s, boolean defval)
   {
      return ds.getBoolean(s) ? true : defval;
   }

   /**
    * Refresh table
    */
   public void refresh(final Runnable postRefreshHook)
   {
      viewer.setInput(null);
      ConsoleJob job = new ConsoleJob(getReadJobName(), viewPart, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            final Table table = readData();
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  if (viewer.getControl().isDisposed())
                     return;
                  
                  if (errorLabel != null)
                  {
                     errorLabel.dispose();
                     errorLabel = null;
                     viewer.getControl().setVisible(true);
                     viewer.getControl().getParent().layout(true, true);
                  }
                  updateViewer(table);
                  if (postRefreshHook != null)
                  {
                     postRefreshHook.run();
                  }
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return getReadJobErrorMessage();
         }

         @Override
         protected IStatus createFailureStatus(final Exception e)
         {
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  if (viewer.getControl().isDisposed())
                     return;
                  
                  if (errorLabel == null)
                  {
                     errorLabel = new CLabel(viewer.getControl().getParent(), SWT.NONE);
                     errorLabel.setFont(JFaceResources.getBannerFont());
                     errorLabel.setImage(StatusDisplayInfo.getStatusImage(Severity.CRITICAL));
                     errorLabel.moveAbove(viewer.getControl());
                     FormData fd = new FormData();
                     fd.top = new FormAttachment(0, 0);
                     fd.left = new FormAttachment(0, 0);
                     fd.right = new FormAttachment(100, 0);
                     fd.bottom = new FormAttachment(100, 0);
                     errorLabel.setLayoutData(fd);
                     viewer.getControl().getParent().layout(true, true);
                     viewer.getControl().setVisible(false);
                  }
                  errorLabel.setText(getErrorMessage() + " (" + e.getLocalizedMessage() + ")"); //$NON-NLS-1$ //$NON-NLS-2$
               }
            });
            return Status.OK_STATUS;
         }
      };
      job.setUser(false);
      job.start();
   }
   
   /**
    * Read data to display
    * 
    * @return table data
    * @throws Exception on error
    */
   protected abstract Table readData() throws Exception;
   
   /**
    * Get name of read job
    * 
    * @return name of read job
    */
   protected abstract String getReadJobName();
   
   /**
    * Get error message for read job
    * 
    * @return error message for read job
    */
   protected abstract String getReadJobErrorMessage();
}
