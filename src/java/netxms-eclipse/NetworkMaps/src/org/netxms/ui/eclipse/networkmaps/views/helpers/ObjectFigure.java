/**
 * 
 */
package org.netxms.ui.eclipse.networkmaps.views.helpers;

import org.eclipse.draw2d.Figure;
import org.eclipse.draw2d.Graphics;
import org.eclipse.draw2d.geometry.Point;
import org.eclipse.draw2d.geometry.Rectangle;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.shared.StatusDisplayInfo;

/**
 * @author victor
 *
 */
public class ObjectFigure extends Figure
{
	private GenericObject object;
	private MapLabelProvider labelProvider;
	
	/**
	 * Constructor
	 * @param object Object represented by this figure
	 */
	public ObjectFigure(GenericObject object, MapLabelProvider labelProvider)
	{
		this.object = object;
		this.labelProvider = labelProvider;
		setSize(40, 40);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.draw2d.Figure#paintFigure(org.eclipse.draw2d.Graphics)
	 */
	@Override
	protected void paintFigure(Graphics gc)
	{
		/*
		//gc.setLineWidth(1);
		//gc.setForegroundColor(StatusDisplayInfo.getStatusColor(object.getStatus()));
		gc.setBackgroundColor(StatusDisplayInfo.getStatusColor(object.getStatus()));
		gc.setAntialias(SWT.ON);
		
		//gc.drawRoundRectangle(rect, 16, 16);
		//gc.setAlpha(64);
		gc.fillRoundRectangle(rect, 16, 16);
		//gc.setAlpha(255);
		gc.drawRoundRectangle(rect, 16, 16);
		*/
		
		Rectangle rect = new Rectangle(getBounds());
		rect.height--;
		rect.width--;
		
		Image image = labelProvider.getImage(object);
		if (image != null)
		{
			gc.drawImage(image, rect.x + 4, rect.y + 4);
		}
		
		image = labelProvider.getStatusImage(object);
		if (image != null)
		{
			org.eclipse.swt.graphics.Rectangle imgSize = image.getBounds();
			gc.drawImage(image, rect.x + rect.width - imgSize.width, rect.y);  // rect.y + rect.height - imgSize.height
		}
	}
}
