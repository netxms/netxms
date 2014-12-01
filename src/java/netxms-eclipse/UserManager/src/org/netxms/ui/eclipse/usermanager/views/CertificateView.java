/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Raden Solutions
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
package org.netxms.ui.eclipse.usermanager.views;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.GroupMarker;
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
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.part.ViewPart;
import org.netxms.api.client.SessionListener;
import org.netxms.api.client.SessionNotification;
import org.netxms.api.client.users.AuthCertificate;
import org.netxms.client.NXCNotification;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.usermanager.Activator;
import org.netxms.ui.eclipse.usermanager.Messages;
import org.netxms.ui.eclipse.usermanager.dialogs.CreateNewCertificateDialog;
import org.netxms.ui.eclipse.usermanager.dialogs.EditCertificateDialog;
import org.netxms.ui.eclipse.usermanager.views.helpers.CertificateComparator;
import org.netxms.ui.eclipse.usermanager.views.helpers.CertificateLabelProvider;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Editor for certificates
 */
public class CertificateView extends ViewPart implements SessionListener
{
   public static final String ID = "org.netxms.ui.eclipse.usermanager.view.user_certificate"; //$NON-NLS-1$
   
   private static final String TABLE_CONFIG_PREFIX = "CertificatesEditor"; //$NON-NLS-1$

   // Columns
   public static final int COLUMN_ID = 0;
   public static final int COLUMN_TYPE = 1;
   public static final int COLUMN_COMMENTS = 2;
   public static final int COLUMN_SUBJECT = 3;
	
	private Map<Long, AuthCertificate> certificates = new HashMap<Long, AuthCertificate>();
	private SortableTableViewer viewer;
	private NXCSession session;
	private Action actionRefresh;
	private Action actionNew;
	private Action actionEdit;
	private Action actionDelete;

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		session = (NXCSession)ConsoleSharedData.getSession();
		
		parent.setLayout(new FillLayout());
		
		final String[] columnNames = {Messages.get().CertificateView_ID, Messages.get().CertificateView_Type, Messages.get().CertificateView_Comments, Messages.get().CertificateView_Subject};
		final int[] columnWidths = { 30, 100, 200, 200 };
		viewer = new SortableTableViewer(parent, columnNames, columnWidths, COLUMN_ID, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
		WidgetHelper.restoreTableViewerSettings(viewer, Activator.getDefault().getDialogSettings(), TABLE_CONFIG_PREFIX);
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new CertificateLabelProvider());
		viewer.setComparator(new CertificateComparator());
		viewer.addSelectionChangedListener(new ISelectionChangedListener()
		{
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
				IStructuredSelection selection = (IStructuredSelection)event.getSelection();
				if (selection != null)
				{
					actionEdit.setEnabled(selection.size() == 1);
					actionDelete.setEnabled(selection.size() > 0);
				}
			}
		});
		viewer.addDoubleClickListener(new IDoubleClickListener() {
			@Override
			public void doubleClick(DoubleClickEvent event)
			{
				actionEdit.run();
			}
		});
		viewer.getTable().addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				WidgetHelper.saveTableViewerSettings(viewer, Activator.getDefault().getDialogSettings(), TABLE_CONFIG_PREFIX);
			}
		});
		
		createActions();
		contributeToActionBars();
		createPopupMenu();
		
		session.addListener(this);
		
		refreshCertificateList();
	}

	/**
	 * Create actions
	 */
	private void createActions()
	{
		actionRefresh = new RefreshAction(this) {
			@Override
			public void run()
			{
				refreshCertificateList();
			}
		};
		
		actionNew = new Action(Messages.get().CertificateView_CreateNewCertif) {
			@Override
			public void run()
			{
				createCertificate();
			}
		};
		actionNew.setImageDescriptor(SharedIcons.ADD_OBJECT);

		actionEdit = new Action(Messages.get().CertificateView_EditCertifData) {
			@Override
			public void run()
			{
				IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
				if (selection.size() != 1)
					return;
				final long certId = ((AuthCertificate)selection.getFirstElement()).getId(); 
				
				AuthCertificate cert = certificates.get(certId);
				if ((cert != null))
				{
					saveCertificate(cert);
				}
			}
		};
		actionEdit.setImageDescriptor(SharedIcons.EDIT);
		
		actionDelete = new Action(Messages.get().CertificateView_DeleteCertif) {
			@Override
			public void run()
			{
				deleteCertificate();
			}
		};
		actionDelete.setImageDescriptor(SharedIcons.DELETE_OBJECT);
	}

	/**
	 * Contribute actions to action bar
	 */
	private void contributeToActionBars()
	{
		IActionBars bars = getViewSite().getActionBars();
		fillLocalPullDown(bars.getMenuManager());
		fillLocalToolBar(bars.getToolBarManager());
	}

	/**
	 * Fill local pull-down menu
	 * 
	 * @param manager
	 *           Menu manager for pull-down menu
	 */
	private void fillLocalPullDown(IMenuManager manager)
	{
		manager.add(actionNew);
		manager.add(actionDelete);
		manager.add(actionEdit);
		manager.add(new Separator());
		manager.add(actionRefresh);
	}

	/**
	 * Fill local tool bar
	 * 
	 * @param manager
	 *           Menu manager for local tool bar
	 */
	private void fillLocalToolBar(IToolBarManager manager)
	{
		manager.add(actionNew);
		manager.add(actionDelete);
		manager.add(actionEdit);
		manager.add(new Separator());
		manager.add(actionRefresh);
	}
	
	/**
	 * Create pop-up menu for user list
	 */
	private void createPopupMenu()
	{
		// Create menu manager
		MenuManager menuMgr = new MenuManager();
		menuMgr.setRemoveAllWhenShown(true);
		menuMgr.addMenuListener(new IMenuListener() {
			public void menuAboutToShow(IMenuManager mgr)
			{
				fillContextMenu(mgr);
			}
		});

		// Create menu
		Menu menu = menuMgr.createContextMenu(viewer.getControl());
		viewer.getControl().setMenu(menu);

		// Register menu for extension.
		getSite().registerContextMenu(menuMgr, viewer);
	}

	/**
	 * Fill context menu
	 * 
	 * @param mgr Menu manager
	 */
	protected void fillContextMenu(final IMenuManager mgr)
	{
		mgr.add(actionNew);
		mgr.add(actionDelete);
		mgr.add(new Separator());
		mgr.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
		mgr.add(new Separator());
		mgr.add(actionEdit);
	}

	/**
	 * Refresh certificate list
	 */
	private void refreshCertificateList()
	{
		new ConsoleJob(Messages.get().CertificateView_RefreshCertif, this, Activator.PLUGIN_ID, Activator.PLUGIN_ID) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final List<AuthCertificate> cl = session.getCertificateList();
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
					   certificates.clear();
						for(AuthCertificate c : cl)
						{
						   certificates.put(c.getId(), c);
						}
						viewer.setInput(certificates.values().toArray());
					}
				});
			}

			@Override
			protected String getErrorMessage()
			{
				return Messages.get().CertificateView_ErrorUnableGetCertifList;
			}
		}.start();
	}
	
	/**
	 * Create new certificate
	 */
	private void createCertificate()
	{
		final CreateNewCertificateDialog dlg = new CreateNewCertificateDialog(getSite().getShell());
		if (dlg.open() == Window.OK)
		{
			new ConsoleJob(Messages.get().CertificateView_CreateNewCertif, this, Activator.PLUGIN_ID, Activator.PLUGIN_ID) {
				@Override
				protected void runInternal(IProgressMonitor monitor) throws Exception
				{
					final String comment = dlg.getComment();
					final byte[] data = dlg.getFileContent();
					if(data != null)
					{
					   session.createNewCertificate(data, comment);
					}
					else
					{
					   throw new Exception(Messages.get().CertificateView_ErrorIncorrectCertifFile);
					}
				}

				@Override
				protected String getErrorMessage()
				{
					return Messages.get().CertificateView_ErrorNewCertifCreation;
				}
			}.start();
		}
	}
	
	/**
	 * Delete selected certificate
	 */
	private void deleteCertificate()
	{
		IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		if (selection.isEmpty())
			return;
		
		if (!MessageDialogHelper.openConfirm(getSite().getShell(), Messages.get().CertificateView_Confirmation, Messages.get().CertificateView_AckDeleteCertif))
			return;
		
		final Object[] objects = selection.toArray();
		new ConsoleJob(Messages.get().CertificateView_DeleteCertif, this, Activator.PLUGIN_ID, Activator.PLUGIN_ID) {
			@Override
			protected String getErrorMessage()
			{
				return Messages.get().CertificateView_ErrorDeleteCert;
			}

			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				for(int i = 0; i < objects.length; i++)
				{
					session.deleteCertificate(((AuthCertificate)objects[i]).getId());
				}
			}
		}.start();
	}
	
	/**
	 * Save certificate changes
	 * 
	 * @param cert selected certificate 
	 */
	private void saveCertificate(final AuthCertificate cert)
	{
	   final EditCertificateDialog dlg = new EditCertificateDialog(getSite().getShell(), cert);
	   if (dlg.open() == Window.OK)
      {
   		new ConsoleJob(Messages.get().CertificateView_SaveCertif, this, Activator.PLUGIN_ID, Activator.PLUGIN_ID) {
   
   			@Override
   			protected void runInternal(IProgressMonitor monitor) throws Exception
   			{
   				session.updateCertificate(cert.getId(), dlg.getComments());
   			}
   
   			@Override
   			protected String getErrorMessage()
   			{
   				return Messages.get().CertificateView_ErrorSaveCertif;
   			}
   
   		}.schedule();
      }
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		viewer.getControl().setFocus();
	}

	/* (non-Javadoc)
	 * @see org.netxms.api.client.SessionListener#notificationHandler(org.netxms.api.client.SessionNotification)
	 */
	@Override
	public void notificationHandler(final SessionNotification n)
	{
	   if(n.getCode() == NXCNotification.DCI_CERTIFICATE_CHANGED)
	   {
	      refreshCertificateList();
	   }
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#dispose()
	 */
	@Override
	public void dispose()
	{
		session.removeListener(this);
		super.dispose();
	}
}
