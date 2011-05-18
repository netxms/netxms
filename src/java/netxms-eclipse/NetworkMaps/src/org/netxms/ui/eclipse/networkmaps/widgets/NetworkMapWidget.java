/**
 * 
 */
package org.netxms.ui.eclipse.networkmaps.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.netxms.ui.eclipse.networkmaps.views.helpers.ExtendedGraphViewer;
import org.netxms.ui.eclipse.networkmaps.views.helpers.MapContentProvider;
import org.netxms.ui.eclipse.networkmaps.views.helpers.MapLabelProvider;

/**
 * Widget for embedding network map into dashboards
 *
 */
public class NetworkMapWidget extends Composite
{
	protected ExtendedGraphViewer viewer;
	protected MapLabelProvider labelProvider;

	public NetworkMapWidget(Composite parent, int style)
	{
		super(parent, style);

		viewer = new ExtendedGraphViewer(parent, SWT.NONE);
		viewer.setContentProvider(new MapContentProvider());
		labelProvider = new MapLabelProvider(viewer);
		viewer.setLabelProvider(labelProvider);
	}

}
