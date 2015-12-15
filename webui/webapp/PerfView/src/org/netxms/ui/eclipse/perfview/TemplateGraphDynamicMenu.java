package org.netxms.ui.eclipse.perfview;

import java.util.HashMap;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.ContributionItem;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.MenuItem;
import org.eclipse.ui.ISources;
import org.eclipse.ui.menus.IWorkbenchContribution;
import org.eclipse.ui.services.IEvaluationService;
import org.eclipse.ui.services.IServiceLocator;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.datacollection.GraphSettings;
import org.netxms.client.objects.AbstractNode;
import org.netxms.ui.eclipse.datacollection.widgets.LastValuesWidget;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.perfview.api.GraphTemplateCache;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

public class TemplateGraphDynamicMenu extends ContributionItem implements IWorkbenchContribution
{
   private IEvaluationService evalService;
   
   /**
    * Creates a contribution item with a null id.
    */
   public TemplateGraphDynamicMenu()
   {
      super();
   }

   /**
    * Creates a contribution item with the given (optional) id.
    * 
    * @param id the contribution item identifier, or null
    */
   public TemplateGraphDynamicMenu(String id)
   {
      super(id);
   }
   
   @Override
   public void initialize(IServiceLocator serviceLocator)
   {
      evalService = (IEvaluationService)serviceLocator.getService(IEvaluationService.class);
   }
   
   /* (non-Javadoc)
    * @see org.eclipse.jface.action.ContributionItem#fill(org.eclipse.swt.widgets.Menu, int)
    */
   @Override
   public void fill(Menu menu, int index)
   {
      Object selection = evalService.getCurrentState().getVariable(ISources.ACTIVE_MENU_SELECTION_NAME);
      if ((selection == null) || !(selection instanceof IStructuredSelection) || ((IStructuredSelection)selection).size() != 1)
         return;
      
      AbstractNode firstNode = null;
      for(Object o : ((IStructuredSelection)selection).toList())
      {
         if (o instanceof AbstractNode)
         {
            firstNode = (AbstractNode)o;
            break;
         }
      }
      
      if(firstNode == null)
         return;

      final AbstractNode node = firstNode;
      GraphSettings[] settings = GraphTemplateCache.getInstance().getGraphTemplates(); //array should be already sorted
      final Menu graphMenu = new Menu(menu);
      
      Map<String, Menu> menus = new HashMap<String, Menu>();
      int added = 0;
      for(int i = 0; i < settings.length; i++)
      {
         if(settings[i].isApplicableForNode(node))
         {
            String[] path = settings[i].getName().split("\\-\\>"); //$NON-NLS-1$
            
            Menu rootMenu = graphMenu;
            for(int j = 0; j < path.length - 1; j++)
            {
               final String key = rootMenu.hashCode() + "@" + path[j].replace("&", ""); //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$
               Menu currMenu = menus.get(key);
               if (currMenu == null)
               {
                  currMenu = new Menu(rootMenu);
                  MenuItem item = new MenuItem(rootMenu, SWT.CASCADE);
                  item.setText(path[j]);
                  item.setMenu(currMenu);
                  menus.put(key, currMenu);
               }
               rootMenu = currMenu;
            }
            
            final MenuItem item = new MenuItem(rootMenu, SWT.PUSH);
            item.setText(path[path.length - 1]);
            item.setData(settings[i]);
            item.addSelectionListener(new SelectionAdapter() {
               @Override
               public void widgetSelected(SelectionEvent e)
               {
                  final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
                  ConsoleJob job = new ConsoleJob("Get last values of " + node.getObjectName() , null, Activator.PLUGIN_ID, null) {
                     @Override
                     protected String getErrorMessage()
                     {
                        return "Not possible to get last values for node" + node.getObjectName();
                     }

                     @Override
                     protected void runInternal(IProgressMonitor monitor) throws Exception
                     {
                        
                        final DciValue[] data = session.getLastValues(node.getObjectId());
                        runInUIThread(new Runnable() {
                           @Override
                           public void run()
                           {
                              GraphTemplateCache.execute(node, (GraphSettings)item.getData(), data);
                           }
                        });
                     }
                  };
                  job.setUser(false);
                  job.start();
               }
            });
            
            added++;
         }
      }
      
      if (added > 0)
      {
         MenuItem graphMenuItem = new MenuItem(menu, SWT.CASCADE, index);
         graphMenuItem.setText("Graphs");
         graphMenuItem.setMenu(graphMenu);
      }
      else
      {
         graphMenu.dispose();
      }
   }
}
