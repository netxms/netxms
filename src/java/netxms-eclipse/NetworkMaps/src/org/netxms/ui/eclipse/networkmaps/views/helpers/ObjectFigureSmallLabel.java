/**
 * 
 */
package org.netxms.ui.eclipse.networkmaps.views.helpers;

import org.eclipse.draw2d.Graphics;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.netxms.client.maps.elements.NetworkMapObject;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;

/**
 * Figure which represents NetXMS object as small icon with label on the right
 */
public class ObjectFigureSmallLabel extends ObjectFigure
{
	/**
	 * @param element
	 * @param labelProvider
	 */
	public ObjectFigureSmallLabel(NetworkMapObject element, MapLabelProvider labelProvider)
	{
		super(element, labelProvider);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.networkmaps.views.helpers.ObjectFigure#onObjectUpdate()
	 */
	@Override
	protected void onObjectUpdate()
	{
	}

	/* (non-Javadoc)
	 * @see org.eclipse.draw2d.Figure#paintFigure(org.eclipse.draw2d.Graphics)
	 */
	@Override
	protected void paintFigure(Graphics gc)
	{
/*		final String text = object.getObjectName();
		final Image image = labelProvider.getImage(object);
		final Point textSize = gc.textExtent(text);

		Rectangle rect = new Rectangle(x - LABEL_ARROW_OFFSET, y - LABEL_ARROW_HEIGHT - textSize.y, textSize.x
				+ image.getImageData().width + LABEL_X_MARGIN * 2 + LABEL_SPACING, Math.max(image.getImageData().height, textSize.y)
				+ LABEL_Y_MARGIN * 2);
		
		gc.setBackground(LABEL_BACKGROUND);
		gc.setForeground(StatusDisplayInfo.getStatusColor(object.getStatus()));
		gc.setLineWidth(2);
		gc.fillRoundRectangle(rect.x, rect.y, rect.width, rect.height, 8, 8);
		gc.drawRoundRectangle(rect.x, rect.y, rect.width, rect.height, 8, 8);

		final int[] arrow = new int[] { rect.x + LABEL_ARROW_OFFSET - 4, rect.y + rect.height, x, y, rect.x + LABEL_ARROW_OFFSET + 4,
				rect.y + rect.height };
		gc.fillPolygon(arrow);
		gc.setForeground(LABEL_BACKGROUND);
		gc.drawLine(arrow[0], arrow[1], arrow[4], arrow[5]);
		gc.setForeground(StatusDisplayInfo.getStatusColor(object.getStatus()));
		gc.drawPolyline(arrow);

		gc.setForeground(LABEL_TEXT);
		gc.drawImage(image, rect.x + LABEL_X_MARGIN, rect.y + LABEL_Y_MARGIN);
		gc.drawText(text, rect.x + LABEL_X_MARGIN + image.getImageData().width + LABEL_SPACING, rect.y + LABEL_Y_MARGIN);
		*/
	}
}
