/**
 * NetXMS - open source network management system
 * Copyright (C) 2020-2022 Raden Soultions
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
package org.netxms.nxmc.modules.objecttools.views;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.Set;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.SashForm;
import org.eclipse.swt.events.ControlAdapter;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.ObjectContext;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.netxms.nxmc.modules.objecttools.views.helpers.ExecutorListLabelProvider;
import org.netxms.nxmc.modules.objecttools.widgets.AbstractObjectToolExecutor;
import org.netxms.nxmc.modules.objecttools.widgets.ActionExecutor;
import org.netxms.nxmc.modules.objecttools.widgets.LocalCommandExecutor;
import org.netxms.nxmc.modules.objecttools.widgets.SSHExecutor;
import org.netxms.nxmc.modules.objecttools.widgets.ServerCommandExecutor;
import org.netxms.nxmc.modules.objecttools.widgets.ServerScriptExecutor;
import org.netxms.nxmc.modules.objecttools.widgets.helpers.ExecutorStateChangeListener;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.xnap.commons.i18n.I18n;

/**
 * View to display command execution results for multiple nodes
 */
public class MultiNodeCommandExecutor extends ObjectView
{
   private I18n i18n = LocalizationHelper.getI18n(MultiNodeCommandExecutor.class);

   private SashForm splitter;
   private SortableTableViewer viewer;
   private Composite resultArea;
   private List<AbstractObjectToolExecutor> executors = new ArrayList<AbstractObjectToolExecutor>();
   private AbstractObjectToolExecutor currentExecutor = null;
   private AbstractObjectToolExecutor.ActionSet actions;
   private Set<ObjectContext> applicableObjects;
   private Set<ObjectContext> sourceObjects;
   private ObjectTool tool;
   private Map<String, String> inputValues;
   private List<String> maskedFields;
   private List<String> expandedText;


   /**
    * 
    * @param tool
    * @param sourceObjects
    * @param nodes
    * @param inputValues
    * @param maskedFields
    * @param expandedText
    */
   public MultiNodeCommandExecutor(ObjectTool tool, Set<ObjectContext> sourceObjects, Set<ObjectContext> nodes, Map<String, String> inputValues, List<String> maskedFields,
         List<String> expandedText)
   {
      super(tool.getDisplayName(), ResourceManager.getImageDescriptor("icons/object-tools/terminal.png"), nodes.toString(), false);
      applicableObjects = nodes;
      this.tool = tool;
      this.inputValues = inputValues;
      this.maskedFields = maskedFields;
      this.expandedText = expandedText;
      this.sourceObjects = sourceObjects;
   }    


   /**
    * Clone constructor
    */
   protected MultiNodeCommandExecutor()
   {
      super(null, ResourceManager.getImageDescriptor("icons/object-tools/terminal.png"), null, false);
   } 

   /**
    * @see org.netxms.nxmc.base.views.View#cloneView()
    */
   @Override
   public View cloneView()
   {
      MultiNodeCommandExecutor view = (MultiNodeCommandExecutor)super.cloneView();
      view.applicableObjects = applicableObjects;
      view.tool = tool;
      view.inputValues = inputValues;
      view.maskedFields = maskedFields;
      view.expandedText = expandedText;
      view.sourceObjects = sourceObjects;
      return view;
   }
   
   /* (non-Javadoc)
    * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
	{      
      parent.setLayout(new FillLayout());
      
      splitter = new SashForm(parent, SWT.VERTICAL);

      Composite topPart = new Composite(splitter, SWT.NONE);
      GridLayout layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.verticalSpacing = 0;
      topPart.setLayout(layout);

      final String[] names = { "Name", "Status" };
      final int[] widths = { 600, 200 };
      viewer = new SortableTableViewer(topPart, names, widths, 0, SWT.DOWN, SWT.FULL_SELECTION);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new ExecutorListLabelProvider());
      viewer.setComparator(new ViewerComparator() {
         @Override
         public int compare(Viewer viewer, Object e1, Object e2)
         {
            AbstractObjectToolExecutor s1 = (AbstractObjectToolExecutor)e1;
            AbstractObjectToolExecutor s2 = (AbstractObjectToolExecutor)e2;
            int result = s1.getObject().getObjectName().compareToIgnoreCase(s2.getObject().getObjectName());
            return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
         }
      });
      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            onSelectionChange();
         }
      });
      viewer.getControl().setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));

      new Label(topPart, SWT.SEPARATOR | SWT.HORIZONTAL).setLayoutData(new GridData(SWT.FILL, SWT.BOTTOM, true, false));

      Composite bottomPart = new Composite(splitter, SWT.NONE);
      layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.verticalSpacing = 0;
      bottomPart.setLayout(layout);

      new Label(bottomPart, SWT.SEPARATOR | SWT.HORIZONTAL).setLayoutData(new GridData(SWT.FILL, SWT.BOTTOM, true, false));

      resultArea = new Composite(bottomPart, SWT.NONE); // this composite will contain resulting bar and test
      resultArea.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
      resultArea.addControlListener(new ControlAdapter() {
         @Override
         public void controlResized(ControlEvent e)
         {
            Rectangle clientArea = resultArea.getClientArea();
            for(Control c : resultArea.getChildren())
               c.setSize(clientArea.width, clientArea.height);
         }
      });

      createActions();
	}


   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#postClone(org.netxms.nxmc.base.views.View)
    */
   @Override
   protected void postClone(View view)
   {
      super.postClone(view);
   }      

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#postContentCreate()
    */
   @Override
   protected void postContentCreate()
   {
      execute();
      super.postContentCreate();
   }

   /**
    * Create actions
    */
   protected void createActions()
   {
      actions = new AbstractObjectToolExecutor.ActionSet();

      actions.actionClear = new Action(i18n.tr("C&lear console"), SharedIcons.CLEAR_LOG) {
         @Override
         public void run()
         {
            if (currentExecutor != null)
               currentExecutor.clearOutput();
         }
      };

      actions.actionScrollLock = new Action(i18n.tr("&Scroll lock"), Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            if (currentExecutor != null)
               currentExecutor.setAutoScroll(!actions.actionScrollLock.isChecked());
         }
      };
      actions.actionScrollLock.setImageDescriptor(ResourceManager.getImageDescriptor("icons/scroll-lock.png"));
      actions.actionScrollLock.setChecked(false);

      actions.actionCopy = new Action(i18n.tr("&Copy")) {
         @Override
         public void run()
         {
            if (currentExecutor != null)
               currentExecutor.copyOutput();
         }
      };
      actions.actionCopy.setEnabled(false);

      actions.actionSelectAll = new Action(i18n.tr("Select &all")) {
         @Override
         public void run()
         {
            if (currentExecutor != null)
               currentExecutor.selectAll();
         }
      };

      actions.actionRestart = new Action(i18n.tr("&Restart"), SharedIcons.RESTART) {
         @Override
         public void run()
         {
            if (currentExecutor != null)
               currentExecutor.execute();
         }
      };
      actions.actionRestart.setEnabled(false);

      actions.actionTerminate = new Action(i18n.tr("&Terminate"), SharedIcons.TERMINATE) {
         @Override
         public void run()
         {
            if (currentExecutor != null)
               currentExecutor.terminate();
         }
      };
      actions.actionTerminate.setEnabled(false);
   }

   /**
    * Handler for viewer selection change
    */
   private void onSelectionChange()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection == null)
         return;

      AbstractObjectToolExecutor executor = (AbstractObjectToolExecutor)selection.getFirstElement();
      if (currentExecutor != executor)
      {
         if (currentExecutor != null)
            currentExecutor.hide();
         currentExecutor = executor;
         if (currentExecutor != null)
            currentExecutor.show();
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
   public void execute()
   {
      executors.clear();
      
      int i = 0;
      for(final ObjectContext ctx : applicableObjects)
      {
         final AbstractObjectToolExecutor executor;
         switch(tool.getToolType())
         {
            case ObjectTool.TYPE_ACTION:
               executor = new ActionExecutor(resultArea, ctx, actions, tool, inputValues, maskedFields);
               break;
            case ObjectTool.TYPE_LOCAL_COMMAND:
               executor = new LocalCommandExecutor(resultArea, ctx, actions, tool, expandedText.get(i++));
               break;
            case ObjectTool.TYPE_SERVER_COMMAND:
               executor = new ServerCommandExecutor(resultArea, ctx, actions, tool, inputValues, maskedFields);
               break;
            case ObjectTool.TYPE_SSH_COMMAND:
               executor = new SSHExecutor(resultArea, ctx, actions, tool, inputValues, maskedFields);
               break;
            case ObjectTool.TYPE_SERVER_SCRIPT:
               executor = new ServerScriptExecutor(resultArea, ctx, actions, tool, inputValues, maskedFields);
               break;
            default:
               executor = null;
               break;
         }
         executor.addStateChangeListener(new ExecutorStateChangeListener() {
            @Override
            public void runningStateChanged(boolean running)
            {
               viewer.update(executor, null);
            }
         });
         executors.add(executor);
         executor.execute();
      }
      viewer.setInput(executors.toArray());
      viewer.getTable().setSelection(0);
      onSelectionChange();
   }

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
    */
   @Override
   public void setFocus()
   {
      viewer.getTable().setFocus();      
   }

   @Override
   public boolean isValidForContext(Object context)
   {
      if (context != null && context instanceof AbstractObject && (sourceObjects.size() > 0))
      {
         for (ObjectContext object : sourceObjects)
         {
            if (((AbstractObject)context).getObjectId() == object.object.getObjectId())
               return true;
         }
         if (((AbstractObject)context).getObjectId() == sourceObjects.iterator().next().contextId)
            return true;
      }
      return false;
   }

   @Override
   public boolean isCloseable()
   {
      return true;
   }
}
