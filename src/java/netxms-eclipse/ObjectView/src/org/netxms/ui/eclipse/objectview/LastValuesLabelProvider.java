/**
 * 
 */
package org.netxms.ui.eclipse.objectview;

import java.text.DateFormat;

import org.eclipse.jface.viewers.ILabelProviderListener;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.NXCDCIValue;


/**
 * @author Victor
 *
 */
public class LastValuesLabelProvider implements ITableLabelProvider
{
	// State images
	private Image[] stateImages = new Image[3];
	
	/**
	 * Default constructor 
	 */
	public LastValuesLabelProvider()
	{
		super();

		stateImages[0] = Activator.getImageDescriptor("icons/active.ico").createImage();
		stateImages[1] = Activator.getImageDescriptor("icons/disabled.ico").createImage();
		stateImages[2] = Activator.getImageDescriptor("icons/unsupported.ico").createImage();
	}

	
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		return (columnIndex == LastValuesView.COLUMN_ID) ? stateImages[((NXCDCIValue)element).getStatus()] : null;
	}

	
	@Override
	public String getColumnText(Object element, int columnIndex)
	{
		switch(columnIndex)
		{
			case LastValuesView.COLUMN_ID:
				return Long.toString(((NXCDCIValue)element).getId());
			case LastValuesView.COLUMN_DESCRIPTION:
				return ((NXCDCIValue)element).getDescription();
			case LastValuesView.COLUMN_VALUE:
				return ((NXCDCIValue)element).getValue();
			case LastValuesView.COLUMN_TIMESTAMP:
				return DateFormat.getDateTimeInstance(DateFormat.SHORT, DateFormat.SHORT).format(((NXCDCIValue)element).getTimestamp());
		}
		return null;
	}

	@Override
	public void addListener(ILabelProviderListener listener)
	{
		// TODO Auto-generated method stub
		
	}

	@Override
	public void dispose()
	{
		for(int i = 0; i < stateImages.length; i++)
			stateImages[i].dispose();
	}

	@Override
	public boolean isLabelProperty(Object element, String property)
	{
		// TODO Auto-generated method stub
		return false;
	}

	@Override
	public void removeListener(ILabelProviderListener listener)
	{
		// TODO Auto-generated method stub
		
	}
}
