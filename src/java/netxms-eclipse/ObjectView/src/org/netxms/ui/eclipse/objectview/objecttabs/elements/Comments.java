/**
 * 
 */
package org.netxms.ui.eclipse.objectview.objecttabs.elements;

import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.widgets.DashboardElement;

/**
 * Show object's comments
 *
 */
public class Comments extends DashboardElement
{
	private GenericObject object;
	private Text comments;

	/**
	 * The constructor
	 * 
	 * @param parent
	 * @param object
	 */
	public Comments(Composite parent, GenericObject object)
	{
		super(parent, "Comments");
		this.object = object;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.widgets.DashboardElement#createClientArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createClientArea(Composite parent)
	{
		comments = new Text(parent, SWT.MULTI | SWT.READ_ONLY);
		if (object != null)
			comments.setText(object.getComments());
		return comments;
	}
	
	/**
	 * Set new comments text
	 * @param text
	 */
	public void setComments(String text)
	{
		comments.setText(text);
	}
}
