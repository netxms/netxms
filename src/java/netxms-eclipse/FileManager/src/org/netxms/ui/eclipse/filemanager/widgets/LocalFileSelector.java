/**
 * 
 */
package org.netxms.ui.eclipse.filemanager.widgets;

import java.io.File;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.FileDialog;
import org.netxms.ui.eclipse.widgets.AbstractSelector;

/**
 * @author Victor
 *
 */
public class LocalFileSelector extends AbstractSelector
{
	private File file = null;
	
	/**
	 * @param parent
	 * @param style
	 */
	public LocalFileSelector(Composite parent, int style)
	{
		super(parent, style);

		setImage(null);
		setText("<none>");
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.widgets.AbstractSelector#selectionButtonHandler()
	 */
	@Override
	protected void selectionButtonHandler()
	{
		FileDialog fd = new FileDialog(getShell(), SWT.OPEN);
		fd.setText("Select File");
		fd.setFilterExtensions(new String[] { "*.*" });
		fd.setFilterNames(new String[] { "All files" });
		String selected = fd.open();
		if (selected != null)
			setFile(new File(selected));
		else
			setFile(null);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.widgets.AbstractSelector#getButtonToolTip()
	 */
	@Override
	protected String getButtonToolTip()
	{
		return "Select file";
	}

	/**
	 * @return the file
	 */
	public File getFile()
	{
		return file;
	}

	/**
	 * @param file the file to set
	 */
	public void setFile(File file)
	{
		this.file = file;
		if (file != null)
		{
			setText(file.getAbsolutePath());
		}
		else
		{
			setImage(null);
			setText("<none>");
		}
	}
}
