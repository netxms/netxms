/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Victor Kirhenshtein
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
import org.eclipse.swt.custom.CLabel;
import org.eclipse.swt.custom.SashForm;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.TableColumn;
import org.eclipse.swt.widgets.TableItem;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Node;
import org.netxms.client.snmp.MibObject;
import org.netxms.client.snmp.SnmpValue;
import org.netxms.client.snmp.SnmpWalkListener;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.actions.ExportToCsvAction;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.actions.CreateSnmpDci;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.netxms.nxmc.modules.snmp.helpers.SnmpValueLabelProvider;
import org.netxms.nxmc.modules.snmp.shared.MibCache;
import org.netxms.nxmc.modules.snmp.views.helpers.SnmpWalkFilter;
import org.netxms.nxmc.modules.snmp.widgets.MibBrowser;
import org.netxms.nxmc.modules.snmp.widgets.MibObjectDetails;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.ThemeEngine;
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
	
	private CLabel header;
	private Font headerFont;
	private MibBrowser mibBrowser;
	private MibObjectDetails details;
	private TableViewer viewer;
	private AbstractNode currentNode = null;
	private NXCSession session;
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
      super(i18n.tr("MIB Explorer"), ResourceManager.getImageDescriptor("icons/object-views/mibexplorer.gif"), ID, true);
      session = Registry.getSession();
   }

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
    */
	@Override
	public void createContent(Composite parent)
	{
		headerFont = new Font(parent.getDisplay(), "Verdana", 11, SWT.BOLD); //$NON-NLS-1$
		
		GridLayout layout = new GridLayout();
		layout.marginHeight = 0;
		layout.marginWidth = 0;
		layout.verticalSpacing = 0;
		parent.setLayout(layout);
		
		header = new CLabel(parent, SWT.BORDER);
		header.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false));
		header.setFont(headerFont);
      header.setBackground(ThemeEngine.getBackgroundColor(ID + ".Header"));
      header.setForeground(ThemeEngine.getForegroundColor(ID + ".Header"));
		header.setText((currentNode != null) ? currentNode.getObjectName() : ""); //$NON-NLS-1$
		
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
		resultArea.setLayout(new FillLayout());
      
      // walk results
		viewer = new TableViewer(resultArea, SWT.FULL_SELECTION | SWT.MULTI | SWT.BORDER);
		viewer.getTable().setLinesVisible(true);
		viewer.getTable().setHeaderVisible(true);
		setupViewerColumns();
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new SnmpValueLabelProvider());
		filter = new SnmpWalkFilter();
      setFilterClient(viewer, filter);
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
				IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
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
		createTreePopupMenu();
		createResultsPopupMenu();
	}
   
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
		actionWalk.setEnabled(currentNode != null);

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
						sb.append(" ["); //$NON-NLS-1$
						sb.append(selection[i].getText(2));
						sb.append("] = "); //$NON-NLS-1$
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
	 * Create popup menu for MIB tree
	 */
	private void createTreePopupMenu()
	{
		// Create menu manager
		MenuManager menuMgr = new MenuManager();
		menuMgr.setRemoveAllWhenShown(true);
		menuMgr.addMenuListener(new IMenuListener() {
			public void menuAboutToShow(IMenuManager mgr)
			{
				fillTreeContextMenu(mgr);
			}
		});

      // Create menu
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
		// Create menu manager
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
	
	
	@Override
   protected void onObjectChange(AbstractObject object)
   {
      currentNode = (AbstractNode)object;
      actionWalk.setEnabled((object != null) && !walkActive);
      header.setText((currentNode != null) ? currentNode.getObjectName() : ""); //$NON-NLS-1$
   }
	
	/**
	 * Do SNMP walk
	 */
	private void doWalk()
	{
		if (walkActive || (currentNode == null))
			return;
		
		final MibObject object = mibBrowser.getSelection();
		if (object == null)
			return;
		
		walkActive = true;
		actionWalk.setEnabled(false);
		viewer.setInput(new SnmpValue[0]);
		walkData.clear();
		
		Job job = new Job(i18n.tr("Walk MIB tree"), this) {
			@Override
			protected void run(IProgressMonitor monitor) throws Exception
			{
				session.snmpWalk(currentNode.getObjectId(), object.getObjectId().toString(), MibExplorer.this);
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
						actionWalk.setEnabled(currentNode != null);
					}
				});
			}
		};
		job.setUser(false);
		job.start();
	}

	/* (non-Javadoc)
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

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#dispose()
	 */
	@Override
	public void dispose()
	{
		headerFont.dispose();		
		super.dispose();
	}

   @Override
   public boolean isValidForContext(Object context)
   {
      return (context != null) && (context instanceof Node) && ((Node)context).hasSnmpAgent();
   }
}
