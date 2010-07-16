/**
 * 
 */
package org.netxms.ui.eclipse.epp.widgets;

import java.util.ArrayList;
import java.util.List;

import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CLabel;
import org.eclipse.swt.events.ControlAdapter;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Listener;
import org.eclipse.swt.widgets.TableColumn;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.ui.eclipse.epp.Activator;
import org.netxms.ui.eclipse.epp.widgets.helpers.SeverityComparator;
import org.netxms.ui.eclipse.epp.widgets.helpers.SeverityLabelProvider;

/**
 * Cell for severity filter
 *
 */
public class SeverityCell extends Cell
{
	private EventProcessingPolicyRule eppRule;
	private TableViewer viewer;
	private TableColumn column;
	private CLabel labelAny;
	
	public SeverityCell(Rule rule, Object data)
	{
		super(rule, data);
		eppRule = (EventProcessingPolicyRule)data;
		
		viewer = new TableViewer(this, SWT.MULTI | SWT.NO_SCROLL);
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new SeverityLabelProvider());
		viewer.setComparator(new SeverityComparator());
		viewer.getTable().addListener(SWT.EraseItem, new Listener() {
			@Override
			public void handleEvent(Event event)
			{
				event.detail &= ~(SWT.SELECTED | SWT.FOCUSED | SWT.HOT);
			}
		});
		
		column = new TableColumn(viewer.getTable(), SWT.LEFT);
		column.setResizable(false);

		labelAny = new CLabel(this, SWT.LEFT);
		labelAny.setText("Any");
		labelAny.setImage(Activator.getImageDescriptor("icons/any.gif").createImage());
		labelAny.setBackground(PolicyEditor.COLOR_BACKGROUND);
		
		setLayout(null);
		addControlListener(new ControlAdapter() {
			/* (non-Javadoc)
			 * @see org.eclipse.swt.events.ControlAdapter#controlResized(org.eclipse.swt.events.ControlEvent)
			 */
			@Override
			public void controlResized(ControlEvent e)
			{
				final Point size = getSize();
				column.setWidth(size.x);
				viewer.getTable().setSize(size);
				labelAny.setSize(size);
			}
		});
		
		updateSeverityList();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.widgets.Composite#computeSize(int, int, boolean)
	 */
	@Override
	public Point computeSize(int wHint, int hHint, boolean changed)
	{
		return ((eppRule.getFlags() & EventProcessingPolicyRule.SEVERITY_ANY) == EventProcessingPolicyRule.SEVERITY_ANY) ? labelAny.computeSize(wHint, hHint, changed) : viewer.getTable().computeSize(wHint, hHint, changed);
	}

	/**
	 * Update list of matched severity
	 */
	private void updateSeverityList()
	{
		int flags = eppRule.getFlags();
		if ((flags & EventProcessingPolicyRule.SEVERITY_ANY) == EventProcessingPolicyRule.SEVERITY_ANY)
		{
			viewer.getTable().setVisible(false);
			labelAny.setVisible(true);
		}
		else
		{
			List<Integer> list = new ArrayList<Integer>(5);
			int mask = EventProcessingPolicyRule.SEVERITY_NORMAL;
			for(int i = 0; i < 5; i++)
			{
				if ((flags & mask) == mask)
					list.add(i);
				mask <<= 1;
			}
			viewer.setInput(list.toArray());

			labelAny.setVisible(false);
			viewer.getTable().setVisible(true);
		}
	}
}
