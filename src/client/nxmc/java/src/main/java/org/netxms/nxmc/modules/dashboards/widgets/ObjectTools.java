/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.dashboards.widgets;

import java.util.HashSet;
import java.util.Set;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.Collector;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.ServiceRoot;
import org.netxms.client.objects.Subnet;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.views.ViewPlacement;
import org.netxms.nxmc.modules.dashboards.config.ObjectToolsConfig;
import org.netxms.nxmc.modules.dashboards.config.ObjectToolsConfig.Tool;
import org.netxms.nxmc.modules.dashboards.views.AbstractDashboardView;
import org.netxms.nxmc.modules.objects.ObjectContext;
import org.netxms.nxmc.modules.objecttools.ObjectToolExecutor;
import org.netxms.nxmc.modules.objecttools.ObjectToolsCache;
import org.netxms.nxmc.tools.ColorConverter;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import com.google.gson.Gson;

/**
 * "Object tools" dashboard element
 */
public class ObjectTools extends ElementWidget
{
   private static final Logger logger = LoggerFactory.getLogger(ObjectTools.class);

   private ObjectToolsConfig config;
   
   /**
    * @param parent
    * @param element
    * @param view
    */
   public ObjectTools(DashboardControl parent, DashboardElement element, AbstractDashboardView view)
   {
      super(parent, element, view);
      
      try
      {
         config = new Gson().fromJson(element.getData(), ObjectToolsConfig.class);
      }
      catch(Exception e)
      {
         logger.error("Cannot parse dashboard element configuration", e);
         config = new ObjectToolsConfig();
      }

      processCommonSettings(config);

      GridLayout layout = new GridLayout();
      layout.numColumns = config.getNumColumns();
      getContentArea().setLayout(layout);

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
      Button b = new Button(getContentArea(), SWT.PUSH | SWT.FLAT);
      b.setText(t.name);
      b.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            executeTool(t);
         }
      });
      b.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false));
      if (t.color > 0)
      {
         b.setBackground(colors.create(ColorConverter.rgbFromInt(t.color)));
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
         return;
      
      AbstractObject object = Registry.getSession().findObjectById(getEffectiveObjectId(t.objectId));
      if (object == null)
         return;
      
      Set<ObjectContext> allObjects = new HashSet<ObjectContext>();
      Set<ObjectContext> nodes = new HashSet<ObjectContext>();
      if (object instanceof AbstractNode)
      {
         ObjectContext oc = new ObjectContext((AbstractNode)object, null, getDashboardObjectId());
         nodes.add(oc);
         allObjects.add(oc);
      }
      else if ((object instanceof Collector) || (object instanceof Container) || (object instanceof ServiceRoot) || (object instanceof Subnet) || (object instanceof Cluster))
      {
         for(AbstractObject n : object.getAllChildren(AbstractObject.OBJECT_NODE))
         {
            ObjectContext oc = new ObjectContext((AbstractNode)n, null, getDashboardObjectId());
            nodes.add(oc);
            allObjects.add(oc);
            allObjects.add(new ObjectContext((AbstractNode)object, null, getDashboardObjectId()));
         }
      }
      ObjectToolExecutor.execute(allObjects, nodes, tool, new ViewPlacement(view));
   }
}
