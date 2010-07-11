/**
 * 
 */
package org.netxms.ui.eclipse.objectbrowser;

import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.jface.viewers.DecorationOverlayIcon;
import org.eclipse.jface.viewers.IDecoration;
import org.eclipse.jface.viewers.ILabelDecorator;
import org.eclipse.jface.viewers.ILabelProviderListener;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.objects.GenericObject;

/**
 * @author Victor
 * 
 * Label decorator for NetXMS objects
 */
public class ObjectDecorator implements ILabelDecorator
{
	// Status images
	private ImageDescriptor[] statusImages;
	
	/**
	 * Default constructor
	 */
	public ObjectDecorator()
	{
		statusImages = new ImageDescriptor[9];
		statusImages[1] = Activator.getImageDescriptor("icons/status/warning.png"); //$NON-NLS-1$
		statusImages[2] = Activator.getImageDescriptor("icons/status/minor.png"); //$NON-NLS-1$
		statusImages[3] = Activator.getImageDescriptor("icons/status/major.png"); //$NON-NLS-1$
		statusImages[4] = Activator.getImageDescriptor("icons/status/critical.png"); //$NON-NLS-1$
		statusImages[5] = Activator.getImageDescriptor("icons/status/unknown.gif"); //$NON-NLS-1$
		statusImages[6] = Activator.getImageDescriptor("icons/status/unmanaged.gif"); //$NON-NLS-1$
		statusImages[7] = Activator.getImageDescriptor("icons/status/disabled.gif"); //$NON-NLS-1$
		statusImages[8] = Activator.getImageDescriptor("icons/status/testing.png"); //$NON-NLS-1$
	}
	                           
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ILabelDecorator#decorateImage(org.eclipse.swt.graphics.Image, java.lang.Object)
	 */
	@Override
	public Image decorateImage(Image image, Object element)
	{
		int status = ((GenericObject)element).getStatus();
		if (statusImages[status] == null)
			return null;
		DecorationOverlayIcon overlay = new DecorationOverlayIcon(image, statusImages[status], IDecoration.BOTTOM_RIGHT);
		return overlay.createImage();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ILabelDecorator#decorateText(java.lang.String, java.lang.Object)
	 */
	@Override
	public String decorateText(String text, Object element)
	{
		// TODO Auto-generated method stub
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IBaseLabelProvider#addListener(org.eclipse.jface.viewers.ILabelProviderListener)
	 */
	@Override
	public void addListener(ILabelProviderListener listener)
	{
		// TODO Auto-generated method stub

	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IBaseLabelProvider#dispose()
	 */
	@Override
	public void dispose()
	{
		// TODO Auto-generated method stub

	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IBaseLabelProvider#isLabelProperty(java.lang.Object, java.lang.String)
	 */
	@Override
	public boolean isLabelProperty(Object element, String property)
	{
		// TODO Auto-generated method stub
		return false;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IBaseLabelProvider#removeListener(org.eclipse.jface.viewers.ILabelProviderListener)
	 */
	@Override
	public void removeListener(ILabelProviderListener listener)
	{
		// TODO Auto-generated method stub

	}
}
