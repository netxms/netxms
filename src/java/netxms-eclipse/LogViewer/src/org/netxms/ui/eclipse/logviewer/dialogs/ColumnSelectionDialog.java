/**
 * 
 */
package org.netxms.ui.eclipse.logviewer.dialogs;

import java.util.Collection;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.log.Log;
import org.netxms.client.log.LogColumn;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * @author Victor
 *
 */
public class ColumnSelectionDialog extends Dialog
{
	private Log logHandle;
	private Combo columnList;
	private LogColumn[] columns;
	private LogColumn selectedColumn;
	
	/**
	 * @param parentShell
	 */
	public ColumnSelectionDialog(Shell parentShell, Log logHandle)
	{
		super(parentShell);
		this.logHandle = logHandle;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite)super.createDialogArea(parent);
		GridLayout layout = new GridLayout();
		layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
		layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
		layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
		dialogArea.setLayout(layout);
		
		Label label = new Label(dialogArea, SWT.NONE);
		label.setText("Log columns");
		
		columnList = new Combo(dialogArea, SWT.DROP_DOWN | SWT.READ_ONLY);
		GridData gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		gd.widthHint = 250;
		columnList.setLayoutData(gd);
		
		Collection<LogColumn> c = logHandle.getColumns();
		columns = c.toArray(new LogColumn[c.size()]);
		for(int i = 0; i < columns.length; i++)
			columnList.add(columns[i].getDescription());
		
		columnList.setFocus();
		
		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		int index = columnList.getSelectionIndex();
		if (index >= 0)
		{
			selectedColumn = columns[index];
			super.okPressed();
		}
		else
		{
			MessageDialog.openWarning(getShell(), "Warning", "You should select column first!");
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText("Select Column");
	}

	/**
	 * @return the selectedColumn
	 */
	public LogColumn getSelectedColumn()
	{
		return selectedColumn;
	}
}
