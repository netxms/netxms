/**
 * 
 */
package org.netxms.nxmc.modules.datacollection.widgets;

import java.util.Arrays;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ViewerRow;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CLabel;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.NXCSession;
import org.netxms.client.Table;
import org.netxms.client.TableColumnDefinition;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.AbstractViewerFilter;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.CellSelectionManager;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.TableContentProvider;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.TableItemComparator;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.TableLabelProvider;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.TableValueFilter;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.netxms.nxmc.tools.WidgetHelper;

/**
 * Base class for displaying table values
 */
public abstract class BaseTableValueViewer extends Composite
{
   protected NXCSession session;
   protected ObjectView viewPart;
   protected String configId;
   protected TableValueFilter filter;
   protected Table currentData = null;
   protected SortableTableViewer viewer;
   protected TableLabelProvider labelProvider;
   protected CLabel errorLabel;
   protected CellSelectionManager cellSelectionManager;
   protected Action actionUseMultipliers;
   protected Action actionShowFilter;
   protected boolean saveTableSettings;

   /**
    * @param parent
    * @param style
    */
   public BaseTableValueViewer(Composite parent, int style, ObjectView viewPart, String configSubId)
   {
      this(parent, style, viewPart, configSubId, false);
   }

   /**
    * @param parent
    * @param style
    */
   public BaseTableValueViewer(Composite parent, int style, ObjectView viewPart, String configSubId, boolean saveTableSettings)
   {
      super(parent, style);
      this.viewPart = viewPart;
      this.saveTableSettings = saveTableSettings;
      
      configId = buildConfigId(configSubId);
      session = Registry.getSession();

      setLayout(new FillLayout());

      viewer = new SortableTableViewer(this, SWT.FULL_SELECTION | SWT.MULTI);
      viewer.setContentProvider(new TableContentProvider());
      labelProvider = new TableLabelProvider();
      viewer.setLabelProvider(labelProvider);
      filter = new TableValueFilter();
      viewer.addFilter(filter);
      cellSelectionManager = new CellSelectionManager(viewer);

      final PreferenceStore ds = PreferenceStore.getInstance(); 
      labelProvider.setUseMultipliers(ds.getAsBoolean(configId + ".useMultipliers", false));
      
      if (saveTableSettings)
      {
         viewer.getTable().addDisposeListener(new DisposeListener() {
            @Override
            public void widgetDisposed(DisposeEvent e)
            {
               WidgetHelper.saveTableViewerSettings(viewer, configId);
            }
         });
      }
      
      createActions();
      createPopupMenu();
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
   }

   /**
    * Fill context menu
    * 
    * @param manager
    */
   protected void fillContextMenu(IMenuManager manager)
   {
      manager.add(actionUseMultipliers);
      manager.add(new Separator());
   }
   
   /**
    * Update viewer with fresh table data
    * 
    * @param table new table DCI data
    */
   protected void updateViewer(final Table table)
   {
      final PreferenceStore ds = PreferenceStore.getInstance();
      
      if (!viewer.isInitialized())
      {
         final String[] names = table.getColumnDisplayNames();
         final int[] widths = new int[names.length];
         Arrays.fill(widths, 150);
         viewer.createColumns(names, widths, 0, SWT.UP);
         
         if (saveTableSettings)
            WidgetHelper.restoreTableViewerSettings(viewer, configId); 
         viewer.getTable().addDisposeListener(new DisposeListener() {
            @Override
            public void widgetDisposed(DisposeEvent e)
            {
               if (saveTableSettings)
                  WidgetHelper.saveTableViewerSettings(viewer, configId); 
               ds.set(configId + ".useMultipliers", labelProvider.areMultipliersUsed());
            }
         });
         viewer.setComparator(new TableItemComparator(table.getColumnDataTypes()));        
      }
      
      labelProvider.setColumns(table.getColumns());
      viewer.setInput(table);
      if (!saveTableSettings)
         viewer.packColumns();
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
               instance.append("~~~"); 
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
      return (currentData != null) ? currentData.getTitle() : ""; 
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
    * Refresh table
    */
   public void refresh(final Runnable postRefreshHook)
   {
      viewer.setInput(null);
      Job job = new Job(getReadJobName(), viewPart) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
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
      };
      job.setUser(false);
      job.start();
   }

   public AbstractViewerFilter getFilter()
   {
      return filter;
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
