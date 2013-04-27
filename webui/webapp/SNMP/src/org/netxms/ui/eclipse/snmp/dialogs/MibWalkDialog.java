/**
 * 
 */
package org.netxms.ui.eclipse.snmp.dialogs;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.TableColumn;
import org.netxms.client.NXCSession;
import org.netxms.client.snmp.SnmpObjectId;
import org.netxms.client.snmp.SnmpValue;
import org.netxms.client.snmp.SnmpWalkListener;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.snmp.Activator;
import org.netxms.ui.eclipse.snmp.Messages;
import org.netxms.ui.eclipse.snmp.views.helpers.SnmpValueLabelProvider;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * MIB walk dialog
 */
public class MibWalkDialog extends Dialog implements SnmpWalkListener
{
	private long nodeId;
	private SnmpObjectId rootObject;
	private TableViewer viewer;
	private List<SnmpValue> walkData = new ArrayList<SnmpValue>();
	private SnmpValue value;
	
	/**
	 * @param parentShell
	 * @param rootObject root object to start walk from
	 */
	public MibWalkDialog(Shell parentShell, long nodeId, SnmpObjectId rootObject)
	{
		super(parentShell);
		setShellStyle(getShellStyle() | SWT.RESIZE);
		this.nodeId = nodeId;
		this.rootObject = rootObject;
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText("MIB Walk Results");
		IDialogSettings settings = Activator.getDefault().getDialogSettings();
		try
		{
			newShell.setSize(settings.getInt("MibWalkDialog.cx"), settings.getInt("MibWalkDialog.cy")); //$NON-NLS-1$ //$NON-NLS-2$
		}
		catch(NumberFormatException e)
		{
		}
	}	

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite)super.createDialogArea(parent);
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
		layout.horizontalSpacing = WidgetHelper.DIALOG_SPACING;
		layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
		layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
		dialogArea.setLayout(layout);

		viewer = new TableViewer(dialogArea, SWT.FULL_SELECTION | SWT.SINGLE | SWT.BORDER);
		viewer.getTable().setLinesVisible(true);
		viewer.getTable().setHeaderVisible(true);
		setupViewerColumns();
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new SnmpValueLabelProvider());
		viewer.getTable().addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				WidgetHelper.saveColumnSettings(viewer.getTable(), Activator.getDefault().getDialogSettings(), "MibWalkDialog"); //$NON-NLS-1$
			}
		});
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.grabExcessVerticalSpace = true;
		gd.widthHint = 600;
		gd.heightHint = 400;
		viewer.getTable().setLayoutData(gd);
		viewer.addDoubleClickListener(new IDoubleClickListener() {
			@Override
			public void doubleClick(DoubleClickEvent event)
			{
				okPressed();
			}
		});
		
		return dialogArea;
	}

	/**
	 * Setup viewer columns
	 */
	private void setupViewerColumns()
	{
		TableColumn tc = new TableColumn(viewer.getTable(), SWT.LEFT);
		tc.setText(Messages.MibExplorer_OID);
		tc.setWidth(300);

		tc = new TableColumn(viewer.getTable(), SWT.LEFT);
		tc.setText(Messages.MibExplorer_Type);
		tc.setWidth(100);

		tc = new TableColumn(viewer.getTable(), SWT.LEFT);
		tc.setText(Messages.MibExplorer_Value);
		tc.setWidth(300);
		
		WidgetHelper.restoreColumnSettings(viewer.getTable(), Activator.getDefault().getDialogSettings(), "MibWalkDialog"); //$NON-NLS-1$
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#create()
	 */
	@Override
	public void create()
	{
		super.create();
		startWalk();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#cancelPressed()
	 */
	@Override
	protected void cancelPressed()
	{
		saveSettings();
		super.cancelPressed();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		value = (SnmpValue)selection.getFirstElement();
		
		saveSettings();
		super.okPressed();
	}

	/**
	 * Save dialog settings
	 */
	private void saveSettings()
	{
		Point size = getShell().getSize();
		IDialogSettings settings = Activator.getDefault().getDialogSettings();

		settings.put("MibWalkDialog.cx", size.x); //$NON-NLS-1$
		settings.put("MibWalkDialog.cy", size.y); //$NON-NLS-1$
	}

	/**
	 * Start SNMP walk
	 */
	private void startWalk()
	{
		getButton(IDialogConstants.OK_ID).setEnabled(false);
		getButton(IDialogConstants.CANCEL_ID).setEnabled(false);
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		new ConsoleJob(Messages.MibExplorer_WalkJob_Title, null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				session.snmpWalk(nodeId, rootObject.toString(), MibWalkDialog.this);
			}

			@Override
			protected String getErrorMessage()
			{
				return Messages.MibExplorer_WalkJob_Error;
			}

			@Override
			protected void jobFinalize()
			{
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						getButton(IDialogConstants.OK_ID).setEnabled(true);
						getButton(IDialogConstants.CANCEL_ID).setEnabled(true);
					}
				});
			}
		}.start();
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

	/**
	 * @return
	 */
	public SnmpValue getValue()
	{
		return value;
	}
}
