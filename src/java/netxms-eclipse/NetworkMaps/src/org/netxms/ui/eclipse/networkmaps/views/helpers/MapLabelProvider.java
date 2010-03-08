package org.netxms.ui.eclipse.networkmaps.views.helpers;

import org.eclipse.draw2d.IFigure;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.widgets.Display;
import org.eclipse.zest.core.viewers.IFigureProvider;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.networkmaps.Activator;
import org.netxms.ui.eclipse.shared.StatusDisplayInfo;

/**
 * Label provider for map
 */
public class MapLabelProvider extends LabelProvider implements IFigureProvider
{
	private Image[] statusImages;
	private Image imgNode;
	private Image imgSubnet;
	private Font font;
	private boolean showStatusIcons = false;
	private boolean showStatusBackground = true;
	
	/**
	 * Create map label provider
	 */
	public MapLabelProvider()
	{
		statusImages = new Image[9];
		for(int i = 0; i < statusImages.length; i++)
			statusImages[i] = StatusDisplayInfo.getStatusImageDescriptor(i).createImage();
		
		imgNode = Activator.getImageDescriptor("icons/node.png").createImage();
		imgSubnet = Activator.getImageDescriptor("icons/subnet.png").createImage();
		
		font = new Font(Display.getDefault(), "Verdana", 7, SWT.NORMAL);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.LabelProvider#getText(java.lang.Object)
	 */
	@Override
	public String getText(Object element)
	{
		if (element instanceof GenericObject)
			return ((GenericObject)element).getObjectName();
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.LabelProvider#getImage(java.lang.Object)
	 */
	@Override
	public Image getImage(Object element)
	{
		if (element instanceof GenericObject)
			return ((GenericObject)element).getObjectClass() == GenericObject.OBJECT_NODE ? imgNode : imgSubnet;
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.zest.core.viewers.IFigureProvider#getFigure(java.lang.Object)
	 */
	@Override
	public IFigure getFigure(Object element)
	{
		return new ObjectFigure((GenericObject)element, this);
	}
	
	/**
	 * Get status image for given object
	 * @param object
	 * @return
	 */
	public Image getStatusImage(GenericObject object)
	{
		Image image = null;
		try
		{
			image = statusImages[object.getStatus()];
		}
		catch(IndexOutOfBoundsException e)
		{
		}
		return image;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.BaseLabelProvider#dispose()
	 */
	@Override
	public void dispose()
	{
		for(int i = 0; i < statusImages.length; i++)
			statusImages[i].dispose();
		imgNode.dispose();
		imgSubnet.dispose();
		font.dispose();
		super.dispose();
	}

	/**
	 * @return the font
	 */
	public Font getFont()
	{
		return font;
	}

	/**
	 * @return the showStatusIcons
	 */
	public boolean isShowStatusIcons()
	{
		return showStatusIcons;
	}

	/**
	 * @param showStatusIcons the showStatusIcons to set
	 */
	public void setShowStatusIcons(boolean showStatusIcons)
	{
		this.showStatusIcons = showStatusIcons;
	}

	/**
	 * @return the showStatusBackground
	 */
	public boolean isShowStatusBackground()
	{
		return showStatusBackground;
	}

	/**
	 * @param showStatusBackground the showStatusBackground to set
	 */
	public void setShowStatusBackground(boolean showStatusBackground)
	{
		this.showStatusBackground = showStatusBackground;
	}
}
