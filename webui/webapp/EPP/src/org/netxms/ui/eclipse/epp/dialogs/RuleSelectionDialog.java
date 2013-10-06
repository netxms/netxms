/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.epp.dialogs;

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
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.NXCSession;
import org.netxms.client.events.EventProcessingPolicy;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.ui.eclipse.epp.Activator;
import org.netxms.ui.eclipse.epp.Messages;
import org.netxms.ui.eclipse.epp.dialogs.helpers.RuleComparator;
import org.netxms.ui.eclipse.epp.dialogs.helpers.RuleLabelProvider;
import org.netxms.ui.eclipse.epp.dialogs.helpers.RuleListFilter;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Rule selection dialog
 */
public class RuleSelectionDialog extends Dialog
{
	private List<EventProcessingPolicyRule> rulesCache;
	private boolean multiSelection = true;
	private Text filterText;
	private TableViewer viewer;
	private RuleListFilter filter;
	private List<EventProcessingPolicyRule> selectedRules = new ArrayList<EventProcessingPolicyRule>();

	/**
	 * @param parentShell
	 * @param rulesCache 
	 */
	public RuleSelectionDialog(Shell parentShell, List<EventProcessingPolicyRule> rulesCache)
	{
		super(parentShell);
		setShellStyle(getShellStyle() | SWT.RESIZE);
		this.rulesCache = rulesCache;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText("Select Rule");
		IDialogSettings settings = Activator.getDefault().getDialogSettings();
		try
		{
			newShell.setSize(settings.getInt("SelectRule.cx"), settings.getInt("SelectRule.cy")); //$NON-NLS-1$ //$NON-NLS-2$
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
		IDialogSettings settings = Activator.getDefault().getDialogSettings();
		Composite dialogArea = (Composite)super.createDialogArea(parent);
		
		GridLayout layout = new GridLayout();
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.horizontalSpacing = WidgetHelper.OUTER_SPACING;
		layout.numColumns = 2;
		dialogArea.setLayout(layout);
		
		new Label(dialogArea, SWT.NONE).setText(Messages.ActionSelectionDialog_Filter);
		
		filterText = new Text(dialogArea, SWT.NONE);
		GridData gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		filterText.setLayoutData(gd);
		final String filterString = settings.get("SelectRule.Filter"); //$NON-NLS-1$
		if (filterString != null)
			filterText.setText(filterString);
		
		viewer = new TableViewer(dialogArea, SWT.BORDER | SWT.FULL_SELECTION | (multiSelection ? SWT.MULTI : SWT.SINGLE) | SWT.H_SCROLL | SWT.V_SCROLL);
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setComparator(new RuleComparator());
		viewer.setLabelProvider(new RuleLabelProvider());
		filter = new RuleListFilter();
		if (filterString != null)
			filter.setFilterString(filterString);
		viewer.addFilter(filter);
		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		gd.horizontalSpan = 2;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		gd.heightHint = 350;
		viewer.getTable().setLayoutData(gd);
		
		filterText.addModifyListener(new ModifyListener() {
			@Override
			public void modifyText(ModifyEvent e)
			{
				filter.setFilterString(filterText.getText());
				viewer.refresh();
			}
		});
		
		viewer.addDoubleClickListener(new IDoubleClickListener() {
			@Override
			public void doubleClick(DoubleClickEvent event)
			{
				RuleSelectionDialog.this.okPressed();
			}
		});
		
		if (rulesCache == null)
		{
			viewer.getTable().setEnabled(false);
			getButton(IDialogConstants.OK_ID).setEnabled(false);
			final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
			ConsoleJob job = new ConsoleJob("Get event processing rules", null, Activator.PLUGIN_ID, null) {
				@Override
				protected void runInternal(IProgressMonitor monitor) throws Exception
				{
					final EventProcessingPolicy policy = session.getEventProcessingPolicy();
					runInUIThread(new Runnable() {		
						@Override
						public void run()
						{
							viewer.getTable().setEnabled(true);
							getButton(IDialogConstants.OK_ID).setEnabled(true);
							viewer.setInput(policy.getRules().toArray());
						}
					});
				}
				
				@Override
				protected String getErrorMessage()
				{
					return "Cannot get event processing rules from server";
				}
			};
			job.setUser(false);
			job.start();
		}
		else
		{
			viewer.setInput(rulesCache.toArray());
		}
		
		return dialogArea;
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
		final IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		for(Object o : selection.toList())
			selectedRules.add((EventProcessingPolicyRule)o);
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

		settings.put("SelectRule.cx", size.x); //$NON-NLS-1$
		settings.put("SelectRule.cy", size.y); //$NON-NLS-1$
		settings.put("SelectRule.Filter", filterText.getText()); //$NON-NLS-1$
	}

	/**
	 * @return true if multiple event selection is enabled
	 */
	public boolean isMultiSelectionEnabled()
	{
		return multiSelection;
	}

	/**
	 * Enable or disable selection of multiple events.
	 * 
	 * @param enable true to enable multiselection, false to disable
	 */
	public void enableMultiSelection(boolean enable)
	{
		this.multiSelection = enable;
	}

	/**
	 * @return the selectedRules
	 */
	public List<EventProcessingPolicyRule> getSelectedRules()
	{
		return selectedRules;
	}
}
