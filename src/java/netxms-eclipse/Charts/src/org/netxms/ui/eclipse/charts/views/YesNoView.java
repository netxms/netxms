package org.netxms.ui.eclipse.charts.views;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.part.ViewPart;

public class YesNoView extends ViewPart {

    public static final String ID = "org.netxms.ui.eclipse.charts.views.YesNoView";

    @Override
    public void createPartControl(final Composite parent) {
	final GridLayout gridLayout = new GridLayout();
	gridLayout.marginTop = 0;
	gridLayout.marginBottom = 0;
	gridLayout.marginLeft = 0;
	gridLayout.marginRight = 0;

	parent.setLayout(gridLayout);
	final GridData data = new GridData(GridData.FILL_BOTH);
	gridLayout.numColumns = 2;

	final Canvas drawingCanvas = new Canvas(parent, SWT.NO_BACKGROUND);
	drawingCanvas.setLayoutData(data);
	drawingCanvas.addPaintListener(new PaintListener() {

	    @Override
	    public void paintControl(PaintEvent e) {
		Canvas canvas = (Canvas) e.widget;
		int x = canvas.getBounds().width;
		int y = canvas.getBounds().height;

		final Color bgColor = parent.getShell().getDisplay().getSystemColor(SWT.COLOR_GRAY);
		e.gc.setBackground(bgColor);
		e.gc.fillRectangle(canvas.getBounds());

		final Color color = parent.getShell().getDisplay().getSystemColor(SWT.COLOR_RED);
		e.gc.setBackground(color);
		e.gc.fillOval(0, 0, x, y);
	    }
	});
    }

    @Override
    public void setFocus() {
    }

}
