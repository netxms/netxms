package org.netxms.ui.eclipse.objectmanager;

import java.util.Map.Entry;

import org.eclipse.jface.viewers.ILabelProviderListener;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.ui.eclipse.objectmanager.propertypages.CustomAttributes;

public class AttrListLabelProvider implements ITableLabelProvider
{
	@Override
	public void addListener(ILabelProviderListener listener)
	{
		// TODO Auto-generated method stub

	}

	@Override
	public void dispose()
	{
		// TODO Auto-generated method stub

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

	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		return null;
	}

	@SuppressWarnings("unchecked")
	@Override
	public String getColumnText(Object element, int columnIndex)
	{
		if (!(element instanceof Entry))
			return null;

		Object obj;
		switch(columnIndex)
		{
			case CustomAttributes.COLUMN_NAME:
				obj = ((Entry)element).getKey();
				return (obj instanceof String) ? (String)obj : null;
			case CustomAttributes.COLUMN_VALUE:
				obj = ((Entry)element).getValue();
				return (obj instanceof String) ? (String)obj : null;
		}
		return null;
	}
}
