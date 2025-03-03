/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.dashboard.widgets;

import java.util.HashSet;
import java.util.Set;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
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
import org.netxms.ui.eclipse.tools.ColorConverter;
import com.google.gson.Gson;

/**
 * "Object tools" dashboard element
 */
public class ObjectTools extends ElementWidget
{
   private ObjectToolsConfig config;
   
   /**
    * @param parent
    * @param element
    * @param viewPart
    */
   public ObjectTools(DashboardControl parent, DashboardElement element, IViewPart viewPart)
   {
      super(parent, element, viewPart);
      
      try
      {
         config = new Gson().fromJson(element.getData(), ObjectToolsConfig.class);
      }
      catch(Exception e)
      {
         e.printStackTrace();
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
      
      AbstractObject object = ConsoleSharedData.getSession().findObjectById(getEffectiveObjectId(t.objectId));
      if (object == null)
         return;
      
      Set<ObjectContext> nodes = new HashSet<ObjectContext>();
      if (object instanceof AbstractNode)
      {
         nodes.add(new ObjectContext((AbstractNode)object, null));
      }
      else if ((object instanceof Container) || (object instanceof ServiceRoot) || (object instanceof Subnet) || (object instanceof Cluster))
      {
         for(AbstractObject n : object.getAllChildren(AbstractObject.OBJECT_NODE))
            nodes.add(new ObjectContext((AbstractNode)n, null));
      }
      ObjectToolExecutor.execute(nodes, tool);
   }
}
