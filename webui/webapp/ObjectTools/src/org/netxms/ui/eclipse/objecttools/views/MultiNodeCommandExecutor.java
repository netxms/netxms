/**
 * NetXMS - open source network management system
 * Copyright (C) 2020 Raden Soultions
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
package org.netxms.ui.eclipse.objecttools.views;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.contexts.IContextService;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.ui.eclipse.objects.ObjectContext;
import org.netxms.ui.eclipse.objecttools.widgets.AbstractObjectToolExecutor;
import org.netxms.ui.eclipse.objecttools.widgets.ActionExecutor;
import org.netxms.ui.eclipse.objecttools.widgets.ServerCommandExecutor;
import org.netxms.ui.eclipse.objecttools.widgets.ServerScriptExecutor;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * View to display command execution results for multiple nodes
 */
public class MultiNodeCommandExecutor extends ViewPart
{
   public static final String ID = "org.netxms.ui.eclipse.objecttools.views.MultiNodeCommandResults"; //$NON-NLS-1$
   
   private SortableTableViewer viewer;
   private Composite resultArea;
   private HashMap<Long, AbstractObjectToolExecutor> resultMap;
   private AbstractNode curentlySelected = null;
   
   	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
	 */
	@Override
	public void init(IViewSite site) throws PartInitException
	{
		super.init(site);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{      
      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.INNER_SPACING;
      layout.horizontalSpacing = WidgetHelper.INNER_SPACING;
      layout.marginTop = 0;
      layout.marginBottom = 0;
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      parent.setLayout(layout);
      
		//Create table with all nodes 
      final String[] names = { "Name" };
      final int[] widths = { 200 };
      viewer = new SortableTableViewer(parent, names, widths, 0, SWT.DOWN, SWT.BORDER);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new LabelProvider() {
         @Override
         public String getText(Object element)
         {
            return ((AbstractNode)element).getObjectName();
         }
      });
      viewer.setComparator(new ViewerComparator() {
         @Override
         public int compare(Viewer viewer, Object e1, Object e2)
         {
            AbstractNode s1 = (AbstractNode)e1;
            AbstractNode s2 = (AbstractNode)e2;
            int result = s1.getObjectName().compareToIgnoreCase(s2.getObjectName());
            return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
         }
      });
      
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      viewer.getControl().setLayoutData(gd);
      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
         
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            IStructuredSelection selection = (IStructuredSelection)event.getSelection();
            if (selection == null)
               return;

            AbstractNode element = (AbstractNode)selection.getFirstElement();    

            if (curentlySelected != null)
               resultMap.get(curentlySelected.getObjectId()).hide();

            resultMap.get(element.getObjectId()).show();
            curentlySelected = element;
         }
      });
      

      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      resultArea = new Composite(parent, SWT.NONE); //this composite will contain resulting bar and test
      resultArea.setLayoutData(gd);
      
		activateContext();
	}

	/**
	 * Activate context
	 */
	private void activateContext()
	{
		IContextService contextService = (IContextService)getSite().getService(IContextService.class);
		if (contextService != null)
		{
			contextService.activateContext("org.netxms.ui.eclipse.objecttools.context.ObjectTools"); //$NON-NLS-1$
		}
	}

	/**
	 * Execute command on nodes
	 * 
	 * @param tool object tool to execute
	 * @param nodes node list to execute on
	 * @param inputValues input values provided by user
	 * @param maskedFields list of the fields that should be masked in audit log
	 * @param expandedText expanded command for local command
	 */
   public void execute(ObjectTool tool, Set<ObjectContext> nodes, Map<String, String> inputValues, List<String> maskedFields,
         List<String> expandedText)
   {
      setPartName(tool.getDisplayName()); //$NON-NLS-1$
      
      List<AbstractNode> nodeList = new ArrayList<AbstractNode>();   
      resultMap = new HashMap<Long, AbstractObjectToolExecutor>();

      int i = 0;
      for(final ObjectContext ctx : nodes)
      {
         AbstractObjectToolExecutor result = null;
         switch(tool.getToolType())
         {
            case ObjectTool.TYPE_ACTION:
               result = new ActionExecutor(resultArea, this, ctx, tool, inputValues, maskedFields);
               break;
               /*
            case ObjectTool.TYPE_LOCAL_COMMAND:
               final String tmp = expandedText.get(i++);
               result = new LocalCommandExecutor(resultArea, this, tmp);
               break;
               */
            case ObjectTool.TYPE_SERVER_COMMAND:
               result = new ServerCommandExecutor(resultArea, this, ctx.object, tool, inputValues, maskedFields);
               break;
            case ObjectTool.TYPE_SERVER_SCRIPT:
               result = new ServerScriptExecutor(resultArea, this, ctx, tool, inputValues);
               break;
         }
         nodeList.add(ctx.object);
         result.execute();
         resultMap.put(ctx.object.getObjectId(), result);
      }
      viewer.setInput(nodeList);  
      viewer.setSelection(new StructuredSelection(nodeList.get(0)), true);      
   }

   @Override
   public void setFocus()
   {
      viewer.getTable().setFocus();      
   }
}
