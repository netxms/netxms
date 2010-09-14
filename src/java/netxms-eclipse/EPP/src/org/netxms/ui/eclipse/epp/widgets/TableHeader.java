/**
 * 
 */
package org.netxms.ui.eclipse.epp.widgets;

import org.eclipse.swt.widgets.Widget;

/**
 * @author victor
 *
 */
public class TableHeader extends Widget
{
	private int[] columnWidths;
	
	/**
	 * @param parent
	 * @param style
	 */
	public TableHeader(PolicyEditor parent, int style)
	{
		super(parent, style);
	}

	/**
	 * @param columnWidths the columnWidths to set
	 */
	public void setColumnWidths(int[] columnWidths)
	{
		this.columnWidths = columnWidths;
	}

}
