/**
 * 
 */
package org.netxms.ui.eclipse.dashboard.widgets;

import java.util.HashSet;
import java.util.Set;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Label;
import org.eclipse.ui.IViewPart;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.ServiceRoot;
import org.netxms.client.objects.Subnet;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.ui.eclipse.dashboard.widgets.internal.ObjectToolsConfig;
import org.netxms.ui.eclipse.dashboard.widgets.internal.ObjectToolsConfig.Tool;
import org.netxms.ui.eclipse.objects.ObjectContext;
import org.netxms.ui.eclipse.objecttools.api.ObjectToolExecutor;
import org.netxms.ui.eclipse.objecttools.api.ObjectToolsCache;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.ColorCache;
import org.netxms.ui.eclipse.tools.ColorConverter;

/**
 * @author victor
 *
 */
public class ObjectTools extends ElementWidget
{
   private ObjectToolsConfig config;
   private ColorCache colorCache;
   
   /**
    * @param parent
    * @param element
    * @param viewPart
    */
   public ObjectTools(DashboardControl parent, DashboardElement element, IViewPart viewPart)
   {
      super(parent, element, viewPart);
      
      colorCache = new ColorCache(this);
      
      try
      {
         config = ObjectToolsConfig.createFromXml(element.getData());
      }
      catch(Exception e)
      {
         e.printStackTrace();
         config = new ObjectToolsConfig();
      }

      GridLayout layout = new GridLayout();
      layout.numColumns = config.getNumColumns();
      setLayout(layout);
      
      if (!config.getTitle().isEmpty())
      {
         Label title = createTitleLabel(this, config.getTitle());
         GridData gd = new GridData();
         gd.grabExcessHorizontalSpace = true;
         gd.horizontalAlignment = SWT.CENTER;
         gd.horizontalSpan = layout.numColumns;
         title.setLayoutData(gd);
      }
      
      if (config.getTools() != null)
      {
         for(Tool t : config.getTools())
         {
            createToolButton(t);
         }
      }
   }

   /**
    * Create button for the tool
    * 
    * @param t
    */
   private void createToolButton(final Tool t)
   {
      Button b = new Button(this, SWT.PUSH | SWT.FLAT);
      b.setText(t.name);
      b.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            executeTool(t);
         }
         
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }
      });
      b.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false));
      if (t.color > 0)
      {
         b.setBackground(colorCache.create(ColorConverter.rgbFromInt(t.color)));
      }
   }

   /**
    * Execute selected tool
    * 
    * @param t
    */
   protected void executeTool(Tool t)
   {
      ObjectTool tool = ObjectToolsCache.getInstance().findTool(t.toolId);
      if (tool == null)
      {
         return;
      }
      
      AbstractObject object = ConsoleSharedData.getSession().findObjectById(t.objectId);
      if (object == null)
      {
         return;
      }
      
      Set<ObjectContext> nodes = new HashSet<ObjectContext>();
      
      if (object instanceof AbstractNode)
      {
         nodes.add(new ObjectContext((AbstractNode)object, null));
      }
      else if ((object instanceof Container) || (object instanceof ServiceRoot) || (object instanceof Subnet) || (object instanceof Cluster))
      {
         for(AbstractObject n : object.getAllChilds(AbstractObject.OBJECT_NODE))
            nodes.add(new ObjectContext((AbstractNode)n, null));
      }
      
      ObjectToolExecutor.execute(nodes, tool);
   }
}
