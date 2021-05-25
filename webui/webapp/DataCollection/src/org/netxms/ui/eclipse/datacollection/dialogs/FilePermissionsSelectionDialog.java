package org.netxms.ui.eclipse.datacollection.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;


public class FilePermissionsSelectionDialog extends Dialog {
	
	private Integer permissions;
	private Button[] buttons;

   public FilePermissionsSelectionDialog(Shell parentShell, Integer oldPermissions) {
      super(parentShell);
      permissions = oldPermissions;
   }
    
   private void addButton(Composite container, String text, Integer order) {
      buttons[order] = new Button(container, SWT.CHECK);
      buttons[order].setLayoutData(new GridData(SWT.FILL, SWT.CENTER, false, false));
      buttons[order].setText(text);
      buttons[order].setSelection((permissions & (1 << order)) > 0);
   }

    @Override
    protected Control createDialogArea(Composite parent) {
        Composite container = (Composite) super.createDialogArea(parent);
        GridLayout gridLayout = new GridLayout(4, true);
        container.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
        container.setLayout(gridLayout);
        buttons = new Button[9];
        
        Label lblUser = new Label(container, SWT.NONE);
        lblUser.setText("User:");
        
        addButton(container, "Read", 0);
        addButton(container, "Write", 1);
        addButton(container, "Execute", 2);
        
        Label lblGroup = new Label(container, SWT.NONE);
        lblGroup.setText("Group:");
        
        addButton(container, "Read", 3);
        addButton(container, "Write", 4);
        addButton(container, "Execute", 5);
        
        Label lblOther = new Label(container, SWT.NONE);
        lblOther.setText("Other:");
        
        addButton(container, "Read", 6);
        addButton(container, "Write", 7);
        addButton(container, "Execute", 8);

        return container;
    }

    // overriding this methods allows you to set the
    // title of the custom dialog
    @Override
    protected void configureShell(Shell newShell) {
        super.configureShell(newShell);
        newShell.setText("File access rights selection");
    }

    @Override
    protected Point getInitialSize() {
        return new Point(370, 200);
    }
    
	/**
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
    	permissions = 0;
    	for (Integer i = 0; i < 9; i++) {
    		permissions |= buttons[i].getSelection() ? 1 << i : 0;
    	}
		super.okPressed();
	}

    
    public Integer getValue() {
    	return permissions;
    }

}