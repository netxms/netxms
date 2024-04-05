/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2015 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.nxsl.dialogs;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.NXCSession;
import org.netxms.client.Script;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog for selecting library script
 */
public class SelectScriptDialog extends Dialog
{
   private I18n i18n = LocalizationHelper.getI18n(SelectScriptDialog.class);
   
	private TableViewer viewer;
	private List<Script> selection = new ArrayList<Script>(0);
	private boolean multiSelection = false;
	
	/**
	 * @param parentShell
	 */
	public SelectScriptDialog(Shell parentShell)
	{
		super(parentShell);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		newShell.setText(i18n.tr("Select script"));
		super.configureShell(newShell);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite)super.createDialogArea(parent);
		GridLayout layout = new GridLayout();
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      dialogArea.setLayout(layout);
		
		new Label(dialogArea, SWT.NONE).setText(i18n.tr("Available scripts"));
		
      viewer = new TableViewer(dialogArea, SWT.BORDER | SWT.FULL_SELECTION | (multiSelection ? SWT.MULTI : 0));
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new LabelProvider() {
			@Override
			public String getText(Object element)
			{
				return ((Script)element).getName();
			}
      });
      viewer.setComparator(new ViewerComparator() {
      	@Override
      	public int compare(Viewer viewer, Object e1, Object e2)
      	{
      		Script s1 = (Script)e1;
      		Script s2 = (Script)e2;
				return s1.getName().compareToIgnoreCase(s2.getName());
      	}
      });
      viewer.addDoubleClickListener(new IDoubleClickListener() {
			@Override
			public void doubleClick(DoubleClickEvent event)
			{
				SelectScriptDialog.this.okPressed();
			}
      });
      
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.heightHint = 300;
      gd.widthHint = 400;
      viewer.getControl().setLayoutData(gd);
      
		final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Get script list"), null) {
			@Override
			protected void run(IProgressMonitor monitor) throws Exception
			{
				final List<Script> scripts = session.getScriptLibrary();
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						viewer.setInput(scripts.toArray());
					}
				});
			}
			
			@Override
			protected String getErrorMessage()
			{
				return i18n.tr("Cannot get script list from server");
			}
		}.start();
      
      return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		IStructuredSelection s = (IStructuredSelection)viewer.getSelection();
		if (s.isEmpty())
		{
			MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("You must select script from list and then press OK."));
			return;
		}
		for(Object o : s.toList())
		   selection.add((Script)o);
		super.okPressed();
	}

	/**
	 * Get first selected script
	 * 
	 * @return selected script
	 */
	public Script getScript()
	{
		return (selection.size() > 0) ? selection.get(0) : null;
	}

   /**
    * @return the multiSelection
    */
   public boolean isMultiSelection()
   {
      return multiSelection;
   }

   /**
    * @param multiSelection the multiSelection to set
    */
   public void setMultiSelection(boolean multiSelection)
   {
      this.multiSelection = multiSelection;
   }

   /**
    * @return the selection
    */
   public List<Script> getSelection()
   {
      return selection;
   }
}
