package org.netxms.nxmc.modules.datacollection.api;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.regex.Pattern;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.datacollection.ChartDciConfig;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.datacollection.GraphDefinition;
import org.netxms.client.objects.AbstractNode;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.views.Perspective;
import org.netxms.nxmc.base.views.ViewPlacement;
import org.netxms.nxmc.base.windows.PopOutViewWindow;
import org.netxms.nxmc.modules.datacollection.views.HistoricalGraphView;
import org.netxms.nxmc.modules.objects.ObjectContext;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Cache for graph templates
 */
public class GraphTemplateCache
{
   private static final Logger logger = LoggerFactory.getLogger(GraphTemplateCache.class);

   private NXCSession session = null;
   private List<GraphDefinition> templateList = new ArrayList<GraphDefinition>();
   
   /**
    * GraphTemplateCache constructor
    * 
    * @param session
    */
   public GraphTemplateCache(NXCSession session)
   {
      this.session = session;
      
      reload();
      
      session.addListener(new SessionListener() {
         @Override
         public void notificationHandler(SessionNotification n)
         {
            switch(n.getCode())
            {
               case SessionNotification.PREDEFINED_GRAPHS_DELETED:
                  for(int i = 0; i < templateList.size(); i++)
                  {
                     GraphDefinition o = templateList.get(i);
                     if (o.getId() == n.getSubCode())
                     {
                        templateList.remove(i);
                        break;
                     }
                  }
                  break;
               case SessionNotification.PREDEFINED_GRAPHS_CHANGED:
                  if ((n.getObject() instanceof GraphDefinition) && ((GraphDefinition)n.getObject()).isTemplate())
                  {                  
                     boolean objectUpdated = false;
                     for(int i = 0; i < templateList.size(); i++)
                     {
                        if(templateList.get(i).getId() == n.getSubCode())
                        {
                           templateList.set(i, (GraphDefinition)n.getObject());
                           objectUpdated = true;
                           break;
                        }
                     }
                     if (!objectUpdated)
                     {
                        templateList.add((GraphDefinition)n.getObject());
                     }
                  }
                  break;
            }
         }
      });
   }

   /**
    * Attach session to cache
    * 
    * @param session
    */
   public static void attachSession(Display display,NXCSession session)
   {
      GraphTemplateCache instance = new GraphTemplateCache(session);
      Registry.setSingleton(display, GraphTemplateCache.class, instance);
   }

   /**
    * Get cache instance
    * 
    * @return
    */
   public static GraphTemplateCache getInstance()
   {
      return Registry.getSingleton(GraphTemplateCache.class);
   }

   /**
    * Reload graph templates from server
    */
   private void reload()
   {
      try
      {
         synchronized(templateList)
         {
            templateList.clear();
            templateList = session.getPredefinedGraphs(true);
         }
      }
      catch(Exception e)
      {
         logger.error("Exception in ObjectToolsCache.reload()", e); //$NON-NLS-1$
      }      
   }
   
   /**
    * Returns array of graph templates
    * 
    * @return
    */
   public GraphDefinition[] getGraphTemplates()
   {
      GraphDefinition[] graphs = templateList.toArray(new GraphDefinition[templateList.size()]);
      Arrays.sort(graphs, new Comparator<GraphDefinition>() {
         @Override
         public int compare(GraphDefinition arg0, GraphDefinition arg1)
         {
            return arg0.getName().replace("&", "").compareToIgnoreCase(arg1.getName().replace("&", "")); //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$ //$NON-NLS-4$
         }
      });
      
      return graphs;
   }

   /**
    * Instantiate graph template.
    *
    * @param node
    * @param template
    * @param dciList list of DCIs on target object
    * @param session
    * @param viewPlacement
    * @throws IOException
    * @throws NXCException
    */
   public static void instantiate(final AbstractNode node, long contextId, GraphDefinition template, final DciValue[] dciList, NXCSession session, ViewPlacement viewPlacement) throws IOException, NXCException
   {
      List<String> textsToExpand = new ArrayList<String>();
      textsToExpand.add(template.getTitle());
      String name = session.substituteMacros(new ObjectContext(node, null, contextId), textsToExpand, new HashMap<String, String>()).get(0);
      final GraphDefinition graphDefinition = new GraphDefinition(template, name);

      final HashSet<ChartDciConfig> chartMetrics = new HashSet<ChartDciConfig>();
      for(ChartDciConfig dci : graphDefinition.getDciList())
      {
         Pattern namePattern = dci.dciName.isEmpty() ? null : Pattern.compile(dci.dciName);
         Pattern descriptionPattern = dci.dciDescription.isEmpty() ? null : Pattern.compile(dci.dciDescription);
         for(int j = 0; j < dciList.length; j++)
         {
            if ((!dci.dciName.isEmpty() && namePattern.matcher(dciList[j].getName()).find()) ||
                (!dci.dciDescription.isEmpty() && descriptionPattern.matcher(dciList[j].getDescription()).find()))
            {
               chartMetrics.add(new ChartDciConfig(dci, dciList[j]));
               if (!dci.multiMatch)
                  break;
            }
         }
      }
      Display display = viewPlacement.getWindow().getShell().getDisplay();
      if (!chartMetrics.isEmpty())
      {
         display.syncExec(new Runnable() {
            @Override
            public void run()
            {
               graphDefinition.setDciList(chartMetrics.toArray(new ChartDciConfig[chartMetrics.size()]));
               showPredefinedGraph(graphDefinition, node, contextId, viewPlacement);               
            }
         });
      }
      else
      {
         display.syncExec(new Runnable() {
            @Override
            public void run()
            {
               MessageDialogHelper.openError(viewPlacement.getWindow().getShell(), "Error", "None of template DCI were found on a node.");
            }
         });
      }
   }

   /**
    * Show predefined graph view
    * 
    * @param graphDefinition graph definition
    * @param viewPlacement 
    */
   private static void showPredefinedGraph(GraphDefinition graphDefinition, AbstractNode node, long contextId, ViewPlacement viewPlacement)
   {      
      Perspective p = viewPlacement.getPerspective();         
      HistoricalGraphView view = new HistoricalGraphView(node, Arrays.asList(graphDefinition.getDciList()), contextId);
      if (p != null)
      {
         p.addMainView(view, true, false);
      }
      else
      {
         PopOutViewWindow.open(view);
      }
      view.initPredefinedGraph(graphDefinition);
   }
}
