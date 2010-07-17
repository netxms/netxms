/**
 * 
 */
package org.netxms.ui.eclipse.epp.widgets;

import java.util.ArrayList;
import java.util.List;

import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CLabel;
import org.eclipse.swt.events.ControlAdapter;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Composite;

/**
 * Generic cell with list
 *
 */
public abstract class ListCell extends Cell
{
	private Composite list;
	private List<CLabel> labels = new ArrayList<CLabel>(5);
	private MenuManager contextMenuMgr;

	/**
	 * @param rule
	 * @param data
	 */
	public ListCell(Rule rule, Object data)
	{
		super(rule, data);

		list = new Composite(this, SWT.NONE);
		list.setBackground(getBackground());
		
		RowLayout layout = new RowLayout();
		layout.type = SWT.VERTICAL;
		layout.fill = true;
		layout.center = true;
		layout.marginBottom = 0;
		layout.marginLeft = 0;
		layout.marginRight = 0;
		layout.marginTop = 0;
		layout.spacing = 0;
		list.setLayout(layout);
		
		setLayout(null);
		addControlListener(new ControlAdapter() {
			/* (non-Javadoc)
			 * @see org.eclipse.swt.events.ControlAdapter#controlResized(org.eclipse.swt.events.ControlEvent)
			 */
			@Override
			public void controlResized(ControlEvent e)
			{
				final Point size = getSize();
				final Point reqSize = list.computeSize(SWT.DEFAULT, SWT.DEFAULT);
				if (reqSize.y < size.y)
				{
					list.setSize(reqSize);
					list.setLocation(0, (size.y - reqSize.y) / 2);
				}
				else
				{
					list.setSize(size);
					list.setLocation(0, 0);
				}
			}
		});
		
		contextMenuMgr = new MenuManager();
		contextMenuMgr.setRemoveAllWhenShown(true);
		contextMenuMgr.addMenuListener(new IMenuListener()
		{
			public void menuAboutToShow(IMenuManager mgr)
			{
				fillContextMenu(mgr);
			}
		});
		setMenu(contextMenuMgr.createContextMenu(this));
		list.setMenu(contextMenuMgr.createContextMenu(list));
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.widgets.Composite#computeSize(int, int, boolean)
	 */
	@Override
	public Point computeSize(int wHint, int hHint, boolean changed)
	{
		return list.computeSize(wHint, hHint, changed);
	}

	/**
	 * Clear cell
	 */
	protected void clear()
	{
		for(int i = 0; i < labels.size(); i++)
		{
			labels.get(i).dispose();
		}
		labels.clear();
	}
	
	/**
	 * Add entry to list
	 * @param text Entry text
	 * @param image Entry image
	 */
	protected void addEntry(String text, Image image)
	{
		CLabel label = new CLabel(list, SWT.NONE);
		label.setText(text);
		label.setImage(image);
		label.setBackground(getBackground());
		label.setMenu(contextMenuMgr.createContextMenu(label));
		labels.add(label);
	}
	
	/**
	 * Fill cell's context menu.
	 * @param mgr Menu manager
	 */
	abstract protected void fillContextMenu(IMenuManager mgr);
}
