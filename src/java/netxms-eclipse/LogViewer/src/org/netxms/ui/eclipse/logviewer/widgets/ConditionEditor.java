/**
 * 
 */
package org.netxms.ui.eclipse.logviewer.widgets;

import java.util.Calendar;
import java.util.Date;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CLabel;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.DateTime;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Text;
import org.eclipse.ui.forms.events.HyperlinkAdapter;
import org.eclipse.ui.forms.events.HyperlinkEvent;
import org.eclipse.ui.forms.widgets.FormToolkit;
import org.eclipse.ui.forms.widgets.ImageHyperlink;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.netxms.client.constants.Severity;
import org.netxms.client.events.EventTemplate;
import org.netxms.client.log.LogColumn;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.eventmanager.dialogs.EventSelectionDialog;
import org.netxms.ui.eclipse.logviewer.widgets.helpers.FilterTreeElement;
import org.netxms.ui.eclipse.objectbrowser.dialogs.ObjectSelectionDialog;
import org.netxms.ui.eclipse.shared.SharedIcons;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.ImageCombo;

/**
 * Editor widget for single log viewer condition
 */
/**
 * @author Victor
 *
 */
public class ConditionEditor extends Composite
{
	private static final String EMPTY_SELECTION_TEXT = "<none>";
	
	private FormToolkit toolkit;
	private WorkbenchLabelProvider labelProvider;
	private LogColumn column;
	private FilterTreeElement parentElement;
	private Runnable deleteHandler;
	private Label logicalOperation;
	private Combo operation;
	private ImageCombo severity;
	private Text value;
	private DateTime datePicker;
	private DateTime timePicker;
	private long objectId = 0;
	private long eventCode = 0;
	private CLabel objectName;
	
	/**
	 * @param parent
	 * @param style
	 */
	public ConditionEditor(Composite parent, FormToolkit toolkit, LogColumn column, FilterTreeElement parentElement)
	{
		super(parent, SWT.NONE);
		
		this.toolkit = toolkit;
		this.column = column;
		this.parentElement = parentElement;

		labelProvider = new WorkbenchLabelProvider();
		addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				labelProvider.dispose();
			}
		});
		
		GridLayout layout = new GridLayout();
		layout.numColumns = 4;
		setLayout(layout);
		
		logicalOperation = new Label(this, SWT.NONE);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.CENTER;
		gd.widthHint = 30;
		logicalOperation.setLayoutData(gd);
		
		switch(column.getType())
		{
			case LogColumn.LC_TIMESTAMP:
				createTimestampMatch();
				break;
			case LogColumn.LC_SEVERITY:
				createSeverityMatch();
				break;
			case LogColumn.LC_OBJECT_ID:
				createObjectMatch();
				break;
			case LogColumn.LC_EVENT_CODE:
				createEventMatch();
				break;
			default:
				createTextPatternMatch();
				break;
		}

		ImageHyperlink link = new ImageHyperlink(this, SWT.NONE);
		link.setImage(SharedIcons.IMG_DELETE_OBJECT);
		link.addHyperlinkListener(new HyperlinkAdapter() {
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
				ConditionEditor.this.dispose();
				deleteHandler.run();
			}
		});
	}

	/**
	 * 
	 */
	private void createTextPatternMatch()
	{
		operation = new Combo(this, SWT.READ_ONLY);
		toolkit.adapt(operation);
		operation.add("LIKE");
		operation.add("NOT LIKE");
		operation.select(0);
		
		value = toolkit.createText(this, "%");
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		value.setLayoutData(gd);
	}
	
	/**
	 * 
	 */
	private void createObjectMatch()
	{
		operation = new Combo(this, SWT.READ_ONLY);
		toolkit.adapt(operation);
		operation.add("IS");
		operation.add("IS NOT");
		operation.select(0);
		
		Composite group = new Composite(this, SWT.NONE);
		GridLayout layout = new GridLayout();
		layout.marginHeight = 0;
		layout.marginWidth = 0;
		layout.horizontalSpacing = WidgetHelper.INNER_SPACING;
		layout.numColumns = 2;
		group.setLayout(layout);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		group.setLayoutData(gd);

		objectName = new CLabel(group, SWT.NONE);
		toolkit.adapt(objectName);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.TOP;
		objectName.setLayoutData(gd);
		objectName.setText(EMPTY_SELECTION_TEXT);
		
		final ImageHyperlink selectionLink = toolkit.createImageHyperlink(group, SWT.NONE);
		selectionLink.setImage(SharedIcons.IMG_FIND);
		selectionLink.addHyperlinkListener(new HyperlinkAdapter() {
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
				selectObject();
			}
		});
	}
	
	/**
	 * Select object
	 */
	private void selectObject()
	{
		ObjectSelectionDialog dlg = new ObjectSelectionDialog(getShell(), null, null);
		dlg.enableMultiSelection(false);
		if (dlg.open() == Window.OK)
		{
			GenericObject[] objects = dlg.getSelectedObjects(GenericObject.class);
			if (objects.length > 0)
			{
				objectId = objects[0].getObjectId();
				objectName.setText(objects[0].getObjectName());
				objectName.setImage(labelProvider.getImage(objects[0]));
			}
			else
			{
				objectId = 0;
				objectName.setText(EMPTY_SELECTION_TEXT);
				objectName.setImage(null);
			}
		}
	}

	/**
	 * 
	 */
	private void createSeverityMatch()
	{
		operation = new Combo(this, SWT.READ_ONLY);
		toolkit.adapt(operation);
		operation.add("IS");
		operation.add("IS NOT");
		operation.select(0);
		
		severity = new ImageCombo(this, SWT.READ_ONLY | SWT.BORDER);
		toolkit.adapt(severity);
		for(int i = Severity.NORMAL; i <= Severity.CRITICAL; i++)
			severity.add(StatusDisplayInfo.getStatusImage(i), StatusDisplayInfo.getStatusText(i));
		severity.select(0);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		severity.setLayoutData(gd);
	}
	
	/**
	 * 
	 */
	private void createTimestampMatch()
	{
		operation = new Combo(this, SWT.READ_ONLY);
		toolkit.adapt(operation);
		operation.add("BEFORE");
		operation.add("AFTER");
		operation.select(0);
		
		Composite group = new Composite(this, SWT.NONE);
		RowLayout layout = new RowLayout();
		layout.type = SWT.HORIZONTAL;
		layout.marginBottom = 0;
		layout.marginTop = 0;
		layout.marginLeft = 0;
		layout.marginRight = 0;
		group.setLayout(layout);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		group.setLayoutData(gd);

		final Calendar c = Calendar.getInstance();
		c.setTime(new Date());

		datePicker = new DateTime(group, SWT.DATE | SWT.DROP_DOWN);
		datePicker.setDate(c.get(Calendar.YEAR), c.get(Calendar.MONTH), c.get(Calendar.DAY_OF_MONTH));

		timePicker = new DateTime(group, SWT.TIME);
		timePicker.setTime(c.get(Calendar.HOUR_OF_DAY), c.get(Calendar.MINUTE), c.get(Calendar.SECOND));
	}
	
	/**
	 * 
	 */
	private void createEventMatch()
	{
		operation = new Combo(this, SWT.READ_ONLY);
		toolkit.adapt(operation);
		operation.add("IS");
		operation.add("IS NOT");
		operation.select(0);
		
		Composite group = new Composite(this, SWT.NONE);
		GridLayout layout = new GridLayout();
		layout.marginHeight = 0;
		layout.marginWidth = 0;
		layout.horizontalSpacing = WidgetHelper.INNER_SPACING;
		layout.numColumns = 2;
		group.setLayout(layout);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		group.setLayoutData(gd);

		objectName = new CLabel(group, SWT.NONE);
		toolkit.adapt(objectName);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.TOP;
		objectName.setLayoutData(gd);
		objectName.setText(EMPTY_SELECTION_TEXT);
		
		final ImageHyperlink selectionLink = toolkit.createImageHyperlink(group, SWT.NONE);
		selectionLink.setImage(SharedIcons.IMG_FIND);
		selectionLink.addHyperlinkListener(new HyperlinkAdapter() {
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
				selectEvent();
			}
		});
	}

	/**
	 * Select event
	 */
	private void selectEvent()
	{
		EventSelectionDialog dlg = new EventSelectionDialog(getShell());
		dlg.enableMultiSelection(false);
		if (dlg.open() == Window.OK)
		{
			EventTemplate[] events = dlg.getSelectedEvents();
			if (events.length > 0)
			{
				eventCode = events[0].getCode();
				objectName.setText(events[0].getName());
				objectName.setImage(labelProvider.getImage(events[0]));
			}
			else
			{
				eventCode = 0;
				objectName.setText(EMPTY_SELECTION_TEXT);
				objectName.setImage(null);
			}
		}
	}

	/**
	 * @param name
	 */
	public void setLogicalOperation(String name)
	{
		logicalOperation.setText(name);
	}

	/**
	 * @param deleteHandler the deleteHandler to set
	 */
	public void setDeleteHandler(Runnable deleteHandler)
	{
		this.deleteHandler = deleteHandler;
	}
}
