/**
 * 
 */
package org.netxms.ui.eclipse.widgets;

import java.net.URL;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;

/**
 * Displays animated image
 */
public class AnimatedImage extends Composite implements DisposeListener
{
	private static final long serialVersionUID = 1L;

	private Image image;
	private Label label;
	
	/**
	 * @param parent
	 * @param style
	 */
	public AnimatedImage(Composite parent, int style)
	{
		super(parent, style | SWT.NO_BACKGROUND);
		addDisposeListener(this);
		setLayout(new FillLayout());
		label = new Label(this, SWT.NO_BACKGROUND);
	}

	/**
	 * Load animated image from URL. For loading image from
	 * plugin, use URL like platform:/plugin/<plugin_id>/<path>
	 * 
	 * @param url image file's URL
	 */
	public void setImage(URL url)
	{
		if (image != null)
		{
			image.dispose();
			image = null;
		}
		
		if (url == null)
		{
			label.setImage(null);
			return;
		}
		
		ImageDescriptor d = ImageDescriptor.createFromURL(url);
		if (d == null)
		{
			label.setImage(null);
			return;
		}
		
		image = d.createImage();
		label.setImage(image);
		
		layout(true, true);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.events.DisposeListener#widgetDisposed(org.eclipse.swt.events.DisposeEvent)
	 */
	@Override
	public void widgetDisposed(DisposeEvent e)
	{
		if (image != null)
			image.dispose();
	}
}
