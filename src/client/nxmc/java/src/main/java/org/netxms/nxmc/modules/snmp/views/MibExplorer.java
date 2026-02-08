/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
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
import java.util.function.Consumer;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.SashForm;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.TableColumn;
import org.eclipse.swt.widgets.TableItem;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Node;
import org.netxms.client.snmp.MibObject;
import org.netxms.client.snmp.SnmpValue;
import org.netxms.client.snmp.SnmpWalkListener;
import org.netxms.nxmc.Memento;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.base.actions.ExportToCsvAction;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.views.ViewNotRestoredException;
import org.netxms.nxmc.base.widgets.FilterText;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.base.widgets.helpers.SelectorConfigurator;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.actions.CreateSnmpDci;
import org.netxms.nxmc.modules.objects.dialogs.ObjectSelectionDialog;
import org.netxms.nxmc.modules.objects.views.AdHocObjectView;
import org.netxms.nxmc.modules.objects.widgets.ObjectSelector;
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
public class MibExplorer extends AdHocObjectView implements SnmpWalkListener
{
   private I18n i18n = LocalizationHelper.getI18n(MibExplorer.class);

	public static final int COLUMN_NAME = 0;
	public static final int COLUMN_TEXT = 1;
	public static final int COLUMN_TYPE = 2;
	public static final int COLUMN_VALUE = 3;
   public static final int COLUMN_RAW_VALUE = 4;

	private MibBrowser mibBrowser;
	private MibObjectDetails details;
   private FilterText filterText;
   private SortableTableViewer viewer;
   private SnmpValueLabelProvider valuesLabelProvider;
	private boolean walkActive = false;
   private long walkObjectId = 0;
	private List<SnmpValue> walkData = new ArrayList<SnmpValue>();
   private String restoredSelection;
	private Action actionWalk;
	private Action actionCopyObjectName;
	private Action actionCopy;
	private Action actionCopyName;
   private Action actionCopySymbolicName;
	private Action actionCopyType;
	private Action actionCopyValue;
   private Action actionCopyRawValue;
	private Action actionSelect;
	private Action actionExportToCsv;
   private Action actionShowResultFilter;
   private Action actionShortTextualNames;
	private CreateSnmpDci actionCreateSnmpDci;
	private Composite resultArea;
	private SnmpWalkFilter filter;
   private boolean toolView;

   /**
    * Create MIB explorer for given node.
    *
    * @param objectId node object ID
    * @param context context for the view
    */
   public MibExplorer(long objectId, long context, boolean toolView)
   {
      super(LocalizationHelper.getI18n(MibExplorer.class).tr("MIB Explorer"), ResourceManager.getImageDescriptor("icons/object-views/mibexplorer.gif"),
            toolView ? "tools.mib-explorer" : "objects.mib-explorer", objectId, context, false);
      this.toolView = toolView;
   }  

   /**
    * Create agent configuration editor for given node.
    *
    * @param node node object
    */
   protected MibExplorer()
   {
      this(0, 0, false);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.AdHocObjectView#cloneView()
    */
   @Override
   public View cloneView()
   {
      MibExplorer view = (MibExplorer)super.cloneView();
      view.toolView = toolView;
      return view;
   }

   /**
    * @see org.netxms.nxmc.base.views.View#postClone(org.netxms.nxmc.base.views.View)
    */
   @Override
   protected void postClone(View origin)
   {
      super.postClone(origin);

      MibExplorer view = (MibExplorer)origin;
      if (!view.walkActive)
      {
         walkData.addAll(view.walkData);
         viewer.setInput(walkData);         
         viewer.packColumns();
      }
      mibBrowser.getTreeViewer().setExpandedElements(view.mibBrowser.getTreeViewer().getVisibleExpandedElements());    
      mibBrowser.getTreeViewer().setSelection(view.mibBrowser.getTreeViewer().getSelection());
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

      if (toolView)
      {
         Group selectorArea = new Group(parent, SWT.NONE);
         selectorArea.setText(i18n.tr("Context"));
         selectorArea.setLayout(new GridLayout());
         selectorArea.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false));

         ObjectSelector objectSelector = new ObjectSelector(selectorArea, SWT.NONE, new SelectorConfigurator().setShowClearButton(true).setShowLabel(false));
         objectSelector.setClassFilter(ObjectSelectionDialog.createNodeSelectionFilter(false));
         objectSelector.setObjectClass(Node.class);
         objectSelector.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false));
         objectSelector.addModifyListener((e) -> {
            AbstractObject object = objectSelector.getObject();
            setObjectId((object != null) ? object.getObjectId() : 0);
            setContext(object);
         });
      }

		SashForm splitter = new SashForm(parent, SWT.VERTICAL);
		splitter.setLayout(new FillLayout());
      GridData gd = new GridData(SWT.FILL, SWT.FILL, true, true);
      gd.verticalIndent = toolView ? 3 : 0;
      splitter.setLayoutData(gd);

		SashForm mibViewSplitter = new SashForm(splitter, SWT.HORIZONTAL);
		mibViewSplitter.setLayout(new FillLayout());

		mibBrowser = new MibBrowser(mibViewSplitter, SWT.BORDER);
		mibBrowser.addSelectionChangedListener(new ISelectionChangedListener() {
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
				MibObject object = mibBrowser.getSelection();
				details.setObject(object);
            actionWalk.setEnabled((object != null) && (object.getObjectId() != null));
			}
		});

		details = new MibObjectDetails(mibViewSplitter, SWT.BORDER, true, mibBrowser, new Consumer<String>() {
		   @Override
         public void accept(String oid)
         {
            doWalk(oid);
         }
		});

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
      filterText.setCloseCallback(() -> showResultFilter(false));

      // walk results
      viewer = new SortableTableViewer(resultArea, SWT.FULL_SELECTION | SWT.MULTI);
		setupViewerColumns();
		viewer.setContentProvider(new ArrayContentProvider());
      valuesLabelProvider = new SnmpValueLabelProvider();
      viewer.setLabelProvider(valuesLabelProvider);
		filter = new SnmpWalkFilter();
		viewer.addFilter(filter);
		viewer.addSelectionChangedListener(new ISelectionChangedListener() {
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
            IStructuredSelection selection = viewer.getStructuredSelection();
				actionSelect.setEnabled(selection.size() == 1);
				actionCreateSnmpDci.selectionChanged(viewer.getSelection());
			}
		});
      viewer.addDoubleClickListener((e) -> selectInTree());
      viewer.setInput(walkData);

      splitter.setWeights(new int[] { 70, 30 });

      createActions();
      createTreeContextMenu();
      createResultsPopupMenu();

      boolean shortTextualNames = PreferenceStore.getInstance().getAsBoolean(getBaseId() + ".shortTextualNames", true);
      actionShortTextualNames.setChecked(shortTextualNames);
      valuesLabelProvider.setShortTextualNames(shortTextualNames);

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

      if ((restoredSelection != null) && !restoredSelection.isBlank())
      {
         details.setOid(restoredSelection);
         restoredSelection = null;
      }
      else
      {
         AbstractObject object = getObject();
         if ((object != null) && (object instanceof Node))
         {
            details.setOid(((Node)object).getSnmpOID());
         }
      }
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
				doWalk(null);
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

      actionCopyRawValue = new Action(i18n.tr("Copy &raw value to clipboard")) {
         @Override
         public void run()
         {
            copyColumnToClipboard(4);
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

      actionShortTextualNames = new Action("S&hort textual names", Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            valuesLabelProvider.setShortTextualNames(actionShortTextualNames.isChecked());
            viewer.refresh();
            viewer.packColumns();
         }
      };

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
      manager.add(actionShortTextualNames);
   }

   /**
    * Create context menu for MIB tree
    */
	private void createTreeContextMenu()
	{
		MenuManager menuMgr = new MenuManager();
		menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener((m) -> fillTreeContextMenu(m));

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
      menuMgr.addMenuListener((m) -> fillResultsContextMenu(m));

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
      manager.add(actionShortTextualNames);
      manager.add(new Separator());

		if (viewer.getSelection().isEmpty())
			return;

		manager.add(actionCopy);
		manager.add(actionCopyName);
      manager.add(actionCopySymbolicName);
		manager.add(actionCopyType);
		manager.add(actionCopyValue);
      manager.add(actionCopyRawValue);
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

      tc = new TableColumn(viewer.getTable(), SWT.LEFT);
      tc.setText(i18n.tr("Raw value"));
      tc.setWidth(300);
	}

	/**
	 * Do SNMP walk
	 */
	public void doWalk(String oid)
	{
      if (walkActive || (getObject() == null))
			return;

      
      if (oid == null)
      {
         final MibObject mibObject = mibBrowser.getSelection();
         if (mibObject == null)
            return;
         oid = mibObject.getObjectId().toString();
      }

		walkActive = true;
		actionWalk.setEnabled(false);
      walkData.clear();
      viewer.refresh();

      final long nodeId = getObjectId();
      final String queryOid = oid;
      walkObjectId = nodeId;

      Job job = new Job(i18n.tr("Walking MIB tree"), this) {
			@Override
			protected void run(IProgressMonitor monitor) throws Exception
			{
            session.snmpWalk(nodeId, queryOid, MibExplorer.this);
			}

			@Override
			protected String getErrorMessage()
			{
				return i18n.tr("Cannot do SNMP MIB tree walk");
			}

			@Override
			protected void jobFinalize()
			{
            runInUIThread(() -> {
               walkActive = false;
               actionWalk.setEnabled(getObject() != null);
				});
			}
		};
		job.setUser(false);
		job.start();
	}

   /**
    * @see org.netxms.client.snmp.SnmpWalkListener#onSnmpWalkData(long, java.util.List)
    */
	@Override
   public void onSnmpWalkData(long nodeId, final List<SnmpValue> data)
	{
		viewer.getControl().getDisplay().asyncExec(new Runnable() {
			@Override
			public void run()
			{
            if (nodeId != walkObjectId)
               return; // Ignore data from incorrect node

				walkData.addAll(data);
            viewer.refresh();
            viewer.packColumns();
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

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#getFullName()
    */
   @Override
   public String getFullName()
   {
      return toolView ? getName() : super.getFullName();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#saveState(org.netxms.nxmc.Memento)
    */
   @Override
   public void saveState(Memento memento)
   {
      super.saveState(memento);
      memento.set("selection", mibBrowser.getSelection().getObjectId().toString());
   }

   /**
    * @throws ViewNotRestoredException 
    * @see org.netxms.nxmc.base.views.ViewWithContext#restoreState(org.netxms.nxmc.Memento)
    */
   @Override
   public void restoreState(Memento memento) throws ViewNotRestoredException
   {      
      super.restoreState(memento);
      restoredSelection = memento.getAsString("selection", null);
   }
}
