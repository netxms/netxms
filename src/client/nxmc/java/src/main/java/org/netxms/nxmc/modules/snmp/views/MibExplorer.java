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
package org.netxms.nxmc.modules.snmp.views;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.SashForm;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.TableColumn;
import org.eclipse.swt.widgets.TableItem;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Node;
import org.netxms.client.snmp.MibObject;
import org.netxms.client.snmp.SnmpValue;
import org.netxms.client.snmp.SnmpWalkListener;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.base.actions.ExportToCsvAction;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.FilterText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.actions.CreateSnmpDci;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.netxms.nxmc.modules.snmp.helpers.SnmpValueLabelProvider;
import org.netxms.nxmc.modules.snmp.shared.MibCache;
import org.netxms.nxmc.modules.snmp.views.helpers.SnmpWalkFilter;
import org.netxms.nxmc.modules.snmp.widgets.MibBrowser;
import org.netxms.nxmc.modules.snmp.widgets.MibObjectDetails;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * SNMP MIB explorer
 */
public class MibExplorer extends ObjectView implements SnmpWalkListener
{
   private static I18n i18n = LocalizationHelper.getI18n(MibExplorer.class);
   private static final String ID = "MibExplorer";

	public static final int COLUMN_NAME = 0;
	public static final int COLUMN_TEXT = 1;
	public static final int COLUMN_TYPE = 2;
	public static final int COLUMN_VALUE = 3;

	private MibBrowser mibBrowser;
	private MibObjectDetails details;
   private FilterText filterText;
	private TableViewer viewer;
	private boolean walkActive = false;
	private List<SnmpValue> walkData = new ArrayList<SnmpValue>();
	private Action actionWalk;
	private Action actionCopyObjectName;
	private Action actionCopy;
	private Action actionCopyName;
   private Action actionCopySymbolicName;
	private Action actionCopyType;
	private Action actionCopyValue;
	private Action actionSelect;
	private Action actionExportToCsv;
   private Action actionShowResultFilter;
	private CreateSnmpDci actionCreateSnmpDci;

	private Composite resultArea;
	private SnmpWalkFilter filter;

   /**
    * Constructor
    * 
    * @param name
    * @param image
    * @param id
    * @param hasFilter
    */
   public MibExplorer()
   {
      super(i18n.tr("MIB Explorer"), ResourceManager.getImageDescriptor("icons/object-views/mibexplorer.gif"), ID, false);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return (context != null) && (context instanceof Node) && ((Node)context).hasSnmpAgent();
   }

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
    */
	@Override
	public void createContent(Composite parent)
	{
		GridLayout layout = new GridLayout();
		layout.marginHeight = 0;
		layout.marginWidth = 0;
		layout.verticalSpacing = 0;
		parent.setLayout(layout);

		SashForm splitter = new SashForm(parent, SWT.VERTICAL);
		splitter.setLayout(new FillLayout());
		splitter.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));

		SashForm mibViewSplitter = new SashForm(splitter, SWT.HORIZONTAL);
		mibViewSplitter.setLayout(new FillLayout());

		mibBrowser = new MibBrowser(mibViewSplitter, SWT.BORDER);
		mibBrowser.addSelectionChangedListener(new ISelectionChangedListener() {
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
				MibObject object = mibBrowser.getSelection();
				details.setObject(object);
			}
		});

		details = new MibObjectDetails(mibViewSplitter, SWT.BORDER, true, mibBrowser);

		// Create result area
		resultArea = new Composite(splitter, SWT.BORDER);
      resultArea.setLayout(new FormLayout());

      filterText = new FilterText(resultArea, SWT.NONE);
      FormData fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(0, 0);
      fd.right = new FormAttachment(100, 0);
      filterText.setLayoutData(fd);
      filterText.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            filter.setFilterString(filterText.getText());
            viewer.refresh();
         }
      });
      filterText.setCloseCallback(new Runnable() {
         @Override
         public void run()
         {
            showResultFilter(false);
         }
      });

      // walk results
      viewer = new TableViewer(resultArea, SWT.FULL_SELECTION | SWT.MULTI);
		viewer.getTable().setLinesVisible(true);
		viewer.getTable().setHeaderVisible(true);
		setupViewerColumns();
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new SnmpValueLabelProvider());
		filter = new SnmpWalkFilter();
		viewer.addFilter(filter);
		viewer.getTable().addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				WidgetHelper.saveColumnSettings(viewer.getTable(), ID); //$NON-NLS-1$
			}
		});
		viewer.addSelectionChangedListener(new ISelectionChangedListener() {
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
            IStructuredSelection selection = viewer.getStructuredSelection();
				actionSelect.setEnabled(selection.size() == 1);
				actionCreateSnmpDci.selectionChanged(viewer.getSelection());
			}
		});
		viewer.addDoubleClickListener(new IDoubleClickListener() {
			@Override
			public void doubleClick(DoubleClickEvent event)
			{
				selectInTree();
			}
		});

      splitter.setWeights(new int[] { 70, 30 });

      createActions();
      createTreeContextMenu();
      createResultsPopupMenu();

      // Setup result filter visibility
      boolean showResultFilter = PreferenceStore.getInstance().getAsBoolean(getBaseId() + ".showResultFilter", true);
      filterText.setVisible(showResultFilter);
      actionShowResultFilter.setChecked(showResultFilter);

      fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = showResultFilter ? new FormAttachment(filterText) : new FormAttachment(0, 0);
      fd.right = new FormAttachment(100, 0);
      fd.bottom = new FormAttachment(100, 0);
      viewer.getControl().setLayoutData(fd);
	}

   /**
    * Show or hide results filter.
    *
    * @param show true to show filter
    */
   public void showResultFilter(boolean show)
   {
      filterText.setVisible(show);
      FormData fd = (FormData)viewer.getControl().getLayoutData();
      fd.top = show ? new FormAttachment(filterText) : new FormAttachment(0, 0);
      getClientArea().layout(true, true);
      if (show)
      {
         filterText.setFocus();
      }
      else
      {
         filterText.setText("");
         onFilterModify();
      }
      PreferenceStore.getInstance().set(getBaseId() + ".showResultFilter", show);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
	@Override
   public void refresh()
   {
      mibBrowser.refreshTree();
   }

   /**
	 * Create actions
	 */
	private void createActions()
	{		
		actionWalk = new Action(i18n.tr("&Walk")) {
			@Override
			public void run()
			{
				doWalk();
			}
		};
      actionWalk.setEnabled(getObject() != null);

		actionCopyObjectName = new Action(i18n.tr("Copy &name to clipboard")) {
			@Override
			public void run()
			{
				final MibObject object = mibBrowser.getSelection();
				if (object != null)
					WidgetHelper.copyToClipboard(object.getName());
			}
		};

		actionCopy = new Action(i18n.tr("&Copy to clipboard")) {
			@Override
			public void run()
			{
				TableItem[] selection = viewer.getTable().getSelection();
				if (selection.length > 0)
				{
               final String newLine = WidgetHelper.getNewLineCharacters();
					StringBuilder sb = new StringBuilder();
					for(int i = 0; i < selection.length; i++)
					{
						if (i > 0)
							sb.append(newLine);
						sb.append(selection[i].getText(0));
                  sb.append(" [");
						sb.append(selection[i].getText(2));
                  sb.append("] = ");
						sb.append(selection[i].getText(3));
					}
					WidgetHelper.copyToClipboard(sb.toString());
				}
			}
		};

		actionCopyName = new Action(i18n.tr("Copy &name to clipboard")) {
			@Override
			public void run()
			{
				copyColumnToClipboard(0);
			}
		};

      actionCopySymbolicName = new Action(i18n.tr("Copy &symbolic name to clipboard")) {
         @Override
         public void run()
         {
            copyColumnToClipboard(1);
         }
      };

		actionCopyType = new Action(i18n.tr("Copy &type to clipboard")) {
			@Override
			public void run()
			{
				copyColumnToClipboard(2);
			}
		};

		actionCopyValue = new Action(i18n.tr("Copy &value to clipboard")) {
			@Override
			public void run()
			{
				copyColumnToClipboard(3);
			}
		};
		
		actionSelect = new Action(i18n.tr("Select in MIB tree")) {
			@Override
			public void run()
			{
				selectInTree();
			}
		};
		actionSelect.setEnabled(false);

		actionExportToCsv = new ExportToCsvAction(this, viewer, true);
		
		actionCreateSnmpDci = new CreateSnmpDci(this);

      actionShowResultFilter = new Action("Show result &filter", Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            showResultFilter(actionShowResultFilter.isChecked());
         }
      };
      actionShowResultFilter.setImageDescriptor(SharedIcons.FILTER);
      addKeyBinding("M1+F2", actionShowResultFilter);
	}

	/**
	 * Select current result in tree
	 */
	private void selectInTree()
	{
		final TableItem[] selection = viewer.getTable().getSelection();
		if (selection.length == 1)
		{
			MibObject o = MibCache.findObject(selection[0].getText(COLUMN_NAME), false);
			if (o != null)
			{
				mibBrowser.setSelection(o);
			}
		}
	}
	
	/**
	 * Copy values in given column and selected rows to clipboard
	 * 
	 * @param column column index
	 */
	private void copyColumnToClipboard(int column)
	{
		final TableItem[] selection = viewer.getTable().getSelection();
		if (selection.length > 0)
		{
         final String newLine = WidgetHelper.getNewLineCharacters();
			final StringBuilder sb = new StringBuilder();
			for(int i = 0; i < selection.length; i++)
			{
				if (i > 0)
					sb.append(newLine);
				sb.append(selection[i].getText(column));
			}
			WidgetHelper.copyToClipboard(sb.toString());
		}
	}

	/**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(org.eclipse.jface.action.IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionShowResultFilter);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionShowResultFilter);
   }

   /**
    * Create context menu for MIB tree
    */
	private void createTreeContextMenu()
	{
		MenuManager menuMgr = new MenuManager();
		menuMgr.setRemoveAllWhenShown(true);
		menuMgr.addMenuListener(new IMenuListener() {
			public void menuAboutToShow(IMenuManager mgr)
			{
				fillTreeContextMenu(mgr);
			}
		});

      Menu menu = menuMgr.createContextMenu(mibBrowser.getTreeControl());
      mibBrowser.getTreeControl().setMenu(menu);
	}

	/**
	 * Fill MIB tree context menu
	 * 
	 * @param mgr Menu manager
	 */
	protected void fillTreeContextMenu(final IMenuManager manager)
	{
		manager.add(actionWalk);
		manager.add(new Separator());
		manager.add(actionCopyObjectName);
		manager.add(new Separator());
	}

	/**
	 * Create popup menu for results table
	 */
	private void createResultsPopupMenu()
	{
		MenuManager menuMgr = new MenuManager();
		menuMgr.setRemoveAllWhenShown(true);
		menuMgr.addMenuListener(new IMenuListener() {
			public void menuAboutToShow(IMenuManager mgr)
			{
				fillResultsContextMenu(mgr);
			}
		});

		// Create menu
		Menu menu = menuMgr.createContextMenu(viewer.getTable());
		viewer.getTable().setMenu(menu);
	}

	/**
	 * Fill walk results table context menu
	 * 
	 * @param mgr Menu manager
	 */
	protected void fillResultsContextMenu(final IMenuManager manager)
	{
      manager.add(actionShowResultFilter);
      manager.add(new Separator());

		if (viewer.getSelection().isEmpty())
			return;

		manager.add(actionCopy);
		manager.add(actionCopyName);
      manager.add(actionCopySymbolicName);
		manager.add(actionCopyType);
		manager.add(actionCopyValue);
		manager.add(actionExportToCsv);
		manager.add(new Separator());
		manager.add(actionSelect);
		manager.add(new Separator());
      manager.add(actionCreateSnmpDci);
	}

	/**
	 * Setup viewer columns
	 */
	private void setupViewerColumns()
	{
		TableColumn tc = new TableColumn(viewer.getTable(), SWT.LEFT);
		tc.setText(i18n.tr("OID"));
		tc.setWidth(300);
		
		tc = new TableColumn(viewer.getTable(), SWT.LEFT);
      tc.setText("OID as text");
      tc.setWidth(300);

		tc = new TableColumn(viewer.getTable(), SWT.LEFT);
		tc.setText(i18n.tr("Type"));
		tc.setWidth(100);

		tc = new TableColumn(viewer.getTable(), SWT.LEFT);
		tc.setText(i18n.tr("Value"));
		tc.setWidth(300);
		
		WidgetHelper.restoreColumnSettings(viewer.getTable(), ID); //$NON-NLS-1$
	}

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectChange(org.netxms.client.objects.AbstractObject)
    */
	@Override
   protected void onObjectChange(AbstractObject object)
   {
      actionWalk.setEnabled((object != null) && !walkActive);
   }

	/**
	 * Do SNMP walk
	 */
	private void doWalk()
	{
      if (walkActive || (getObject() == null))
			return;

		final MibObject mibObject = mibBrowser.getSelection();
		if (mibObject == null)
			return;

		walkActive = true;
		actionWalk.setEnabled(false);
		viewer.setInput(new SnmpValue[0]);
		walkData.clear();

      final long nodeId = getObjectId();
      Job job = new Job(i18n.tr("Walking MIB tree"), this) {
			@Override
			protected void run(IProgressMonitor monitor) throws Exception
			{
            session.snmpWalk(nodeId, mibObject.getObjectId().toString(), MibExplorer.this);
			}

			@Override
			protected String getErrorMessage()
			{
				return i18n.tr("Cannot do SNMP MIB tree walk");
			}

			@Override
			protected void jobFinalize()
			{
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						walkActive = false;
                  actionWalk.setEnabled(getObject() != null);
					}
				});
			}
		};
		job.setUser(false);
		job.start();
	}

   /**
    * @see org.netxms.client.snmp.SnmpWalkListener#onSnmpWalkData(java.util.List)
    */
	@Override
	public void onSnmpWalkData(final List<SnmpValue> data)
	{
		viewer.getControl().getDisplay().asyncExec(new Runnable() {
			@Override
			public void run()
			{
				walkData.addAll(data);
				viewer.setInput(walkData.toArray());
				try
				{
					viewer.getTable().showItem(viewer.getTable().getItem(viewer.getTable().getItemCount() - 1));
				}
				catch(Exception e)
				{
					// silently ignore any possible problem with table scrolling
				}
			}
		});
	}
}
