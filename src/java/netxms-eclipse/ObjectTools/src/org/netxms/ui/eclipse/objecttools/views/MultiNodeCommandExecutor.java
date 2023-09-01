/**
 * NetXMS - open source network management system
 * Copyright (C) 2020-2022 Raden Solutions
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
import java.util.List;
import java.util.Map;
import java.util.Set;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.commands.ActionHandler;
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
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.contexts.IContextService;
import org.eclipse.ui.handlers.IHandlerService;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.objects.ObjectContext;
import org.netxms.ui.eclipse.objecttools.Activator;
import org.netxms.ui.eclipse.objecttools.Messages;
import org.netxms.ui.eclipse.objecttools.views.helpers.ExecutorListLabelProvider;
import org.netxms.ui.eclipse.objecttools.widgets.AbstractObjectToolExecutor;
import org.netxms.ui.eclipse.objecttools.widgets.ActionExecutor;
import org.netxms.ui.eclipse.objecttools.widgets.LocalCommandExecutor;
import org.netxms.ui.eclipse.objecttools.widgets.SSHExecutor;
import org.netxms.ui.eclipse.objecttools.widgets.ServerCommandExecutor;
import org.netxms.ui.eclipse.objecttools.widgets.ServerScriptExecutor;
import org.netxms.ui.eclipse.objecttools.widgets.helpers.ExecutorStateChangeListener;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * View to display command execution results for multiple nodes
 */
public class MultiNodeCommandExecutor extends ViewPart
{
   public static final String ID = "org.netxms.ui.eclipse.objecttools.views.MultiNodeCommandExecutor"; //$NON-NLS-1$

   private SashForm splitter;
   private SortableTableViewer viewer;
   private Composite resultArea;
   private List<AbstractObjectToolExecutor> executors = new ArrayList<AbstractObjectToolExecutor>();
   private AbstractObjectToolExecutor currentExecutor = null;
   private AbstractObjectToolExecutor.ActionSet actions;

   /**
    * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
    */
	@Override
	public void init(IViewSite site) throws PartInitException
	{
		super.init(site);
	}

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
    */
	@Override
	public void createPartControl(Composite parent)
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
		activateContext();
	}

   /**
    * Create actions
    */
   protected void createActions()
   {
      final IHandlerService handlerService = (IHandlerService)getSite().getService(IHandlerService.class);

      actions = new AbstractObjectToolExecutor.ActionSet();

      actions.actionClear = new Action(Messages.get().LocalCommandResults_ClearConsole, SharedIcons.CLEAR_LOG) {
         @Override
         public void run()
         {
            if (currentExecutor != null)
               currentExecutor.clearOutput();
         }
      };
      actions.actionClear.setActionDefinitionId("org.netxms.ui.eclipse.objecttools.commands.clear_output"); //$NON-NLS-1$
      handlerService.activateHandler(actions.actionClear.getActionDefinitionId(), new ActionHandler(actions.actionClear));

      actions.actionScrollLock = new Action(Messages.get().LocalCommandResults_ScrollLock, Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            if (currentExecutor != null)
               currentExecutor.setAutoScroll(!actions.actionScrollLock.isChecked());
         }
      };
      actions.actionScrollLock.setImageDescriptor(Activator.getImageDescriptor("icons/scroll_lock.gif")); //$NON-NLS-1$
      actions.actionScrollLock.setChecked(false);
      actions.actionScrollLock.setActionDefinitionId("org.netxms.ui.eclipse.objecttools.commands.scroll_lock"); //$NON-NLS-1$
      handlerService.activateHandler(actions.actionScrollLock.getActionDefinitionId(), new ActionHandler(actions.actionScrollLock));

      actions.actionCopy = new Action(Messages.get().LocalCommandResults_Copy) {
         @Override
         public void run()
         {
            if (currentExecutor != null)
               currentExecutor.copyOutput();
         }
      };
      actions.actionCopy.setEnabled(false);
      actions.actionCopy.setActionDefinitionId("org.netxms.ui.eclipse.objecttools.commands.copy"); //$NON-NLS-1$
      handlerService.activateHandler(actions.actionCopy.getActionDefinitionId(), new ActionHandler(actions.actionCopy));

      actions.actionSelectAll = new Action(Messages.get().LocalCommandResults_SelectAll) {
         @Override
         public void run()
         {
            if (currentExecutor != null)
               currentExecutor.selectAll();
         }
      };
      actions.actionSelectAll.setActionDefinitionId("org.netxms.ui.eclipse.objecttools.commands.select_all"); //$NON-NLS-1$
      handlerService.activateHandler(actions.actionSelectAll.getActionDefinitionId(), new ActionHandler(actions.actionSelectAll));

      actions.actionRestart = new Action(Messages.get().LocalCommandResults_Restart, SharedIcons.RESTART) {
         @Override
         public void run()
         {
            if (currentExecutor != null)
               currentExecutor.execute();
         }
      };
      actions.actionRestart.setEnabled(false);

      actions.actionTerminate = new Action(Messages.get().LocalCommandResults_Terminate, SharedIcons.TERMINATE) {
         @Override
         public void run()
         {
            if (currentExecutor != null)
               currentExecutor.terminate();
         }
      };
      actions.actionTerminate.setEnabled(false);
      actions.actionTerminate.setActionDefinitionId("org.netxms.ui.eclipse.objecttools.commands.terminate_process"); //$NON-NLS-1$
      handlerService.activateHandler(actions.actionTerminate.getActionDefinitionId(), new ActionHandler(actions.actionTerminate));
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
   public void execute(ObjectTool tool, Set<ObjectContext> nodes, Map<String, String> inputValues, List<String> maskedFields, List<String> expandedText)
   {
      setPartName(tool.getDisplayName()); //$NON-NLS-1$
      executors.clear();
      
      int i = 0;
      for(final ObjectContext ctx : nodes)
      {
         final AbstractObjectToolExecutor executor;
         switch(tool.getToolType())
         {
            case ObjectTool.TYPE_ACTION:
               executor = new ActionExecutor(resultArea, this, ctx, actions, tool, inputValues, maskedFields);
               break;
            case ObjectTool.TYPE_LOCAL_COMMAND:
               executor = new LocalCommandExecutor(resultArea, this, ctx, actions, tool, expandedText.get(i++));
               break;
            case ObjectTool.TYPE_SERVER_COMMAND:
               executor = new ServerCommandExecutor(resultArea, this, ctx, actions, tool, inputValues, maskedFields);
               break;
            case ObjectTool.TYPE_SSH_COMMAND:
               executor = new SSHExecutor(resultArea, this, ctx, actions, tool, inputValues, maskedFields);
               break;
            case ObjectTool.TYPE_SERVER_SCRIPT:
               executor = new ServerScriptExecutor(resultArea, this, ctx, actions, tool, inputValues, maskedFields);
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
}
