/**
 * 
 */
package org.netxms.ui.eclipse.datacollection.widgets.internal;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.jface.viewers.IStructuredContentProvider;
import org.eclipse.jface.viewers.Viewer;
import org.netxms.client.NXCSession;
import org.netxms.client.Table;
import org.netxms.client.TableRow;
import org.netxms.client.events.Alarm;
import org.netxms.client.objects.AbstractNode;
import org.netxms.ui.eclipse.objects.ObjectWrapper;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Content provider for NetXMS table viewer
 *
 */
public class TableContentProvider implements IStructuredContentProvider
{
	private Table table = null;
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IContentProvider#inputChanged(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public void inputChanged(Viewer viewer, Object oldInput, Object newInput)
	{
		if ((newInput != null) && (newInput instanceof Table))
		{
			table = (Table)newInput;
		}
		else
		{
			table = null;
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IStructuredContentProvider#getElements(java.lang.Object)
	 */
	@Override
	public Object[] getElements(Object inputElement)
	{
		if (table == null)
			return new Object[0];
		
		List <RowWrapper> list = new ArrayList<RowWrapper>();
		TableRow[] rows = table.getAllRows();
		for(int i = 0; i < rows.length; i++)
		{
		   list.add(new RowWrapper(rows[i]));		   
		}
		
		return list.toArray();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IContentProvider#dispose()
	 */
	@Override
	public void dispose()
	{
	}
	
	class RowWrapper extends TableRow implements ObjectWrapper
	{
	   /**
	    * Abstract object that will be given to selector
	    */
	   public AbstractNode object;  

      public RowWrapper(TableRow src)
      {
         super(src);
         final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
         object = (AbstractNode)session.findObjectByName(src.get(0).getValue());
      }

      @Override
      public AbstractNode getObject()
      {
         return object;
      }
	   
	}
}
