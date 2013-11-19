/**
 * 
 */
package org.netxms.ui.eclipse.datacollection.widgets;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CCombo;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Text;
import org.eclipse.ui.forms.events.HyperlinkAdapter;
import org.eclipse.ui.forms.events.HyperlinkEvent;
import org.eclipse.ui.forms.widgets.FormToolkit;
import org.eclipse.ui.forms.widgets.ImageHyperlink;
import org.eclipse.ui.forms.widgets.ScrolledForm;
import org.netxms.client.datacollection.TableCondition;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.datacollection.Messages;
import org.netxms.ui.eclipse.widgets.DashboardComposite;

/**
 * Table threshold conditions editor
 */
public class TableConditionsEditor extends Composite
{
	private FormToolkit toolkit;
	private ScrolledForm form;
	private ImageHyperlink addColumnLink;
	private List<GroupEditor> groups = new ArrayList<GroupEditor>();

	/**
	 * @param parent
	 * @param style
	 */
	public TableConditionsEditor(Composite parent, int style)
	{
		super(parent, style);

		setLayout(new FillLayout());
		
		toolkit = new FormToolkit(getDisplay());
		form = toolkit.createScrolledForm(this);
		form.getBody().setLayout(new GridLayout());
		
		addColumnLink = toolkit.createImageHyperlink(form.getBody(), SWT.NONE);
		addColumnLink.setText(Messages.get().TableConditionsEditor_Add);
		addColumnLink.setImage(SharedIcons.IMG_ADD_OBJECT);
		addColumnLink.setToolTipText(Messages.get().TableConditionsEditor_AddCondGroup);
		addColumnLink.addHyperlinkListener(new HyperlinkAdapter() {
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
				addGroup(addColumnLink, null);
			}
		});
	}

	/**
	 * @param conditions
	 */
	public void setConditions(List<List<TableCondition>> conditions)
	{
		for(List<TableCondition> g : conditions)
		{
			addGroup(addColumnLink, g);
		}
		form.reflow(true);
	}

	/**
	 * @return
	 */
	public List<List<TableCondition>> getConditions()
	{
		List<List<TableCondition>> result = new ArrayList<List<TableCondition>>(groups.size());
		for(GroupEditor ge : groups)
		{
			result.add(ge.getConditions());
		}
		return result;
	}

	/**
	 * Add condition group
	 */
	private void addGroup(final Control lastControl, List<TableCondition> initialData)
	{
		final GroupEditor editor = new GroupEditor(form.getBody());
		editor.moveAbove(lastControl);
		editor.setLayoutData(new GridData(SWT.FILL, SWT.TOP, true, false));
		groups.add(editor);
		if (initialData != null)
		{
			for(TableCondition c : initialData)
			{
				editor.addCondition(c);
			}
		}
		else
		{
			form.reflow(true);
		}
	}
	
	/**
	 * Delete condition group
	 * 
	 * @param editor
	 */
	private void deleteGroup(final GroupEditor editor)
	{
		groups.remove(editor);
		editor.dispose();
		form.reflow(true);
	}
	
	/**
	 * Group editor widget
	 */
	private class GroupEditor extends DashboardComposite
	{
		private Composite content;
		private List<ConditionEditor> conditions = new ArrayList<ConditionEditor>();
		
		public GroupEditor(Composite parent)
		{
			super(parent, SWT.BORDER);

			toolkit.adapt(this);
			
			GridLayout layout = new GridLayout();
			layout.numColumns = 2;
			setLayout(layout);
			
			content = toolkit.createComposite(this);
			layout = new GridLayout();
			layout.marginWidth = 0;
			layout.marginHeight = 0;
			layout.numColumns = 4;
			content.setLayout(layout);
			content.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
			
			Composite buttons = toolkit.createComposite(this);
			layout = new GridLayout();
			layout.numColumns = 2;
			layout.marginWidth = 0;
			layout.marginHeight = 0;
			buttons.setLayout(layout);
			buttons.setLayoutData(new GridData(SWT.FILL, SWT.TOP, false, false));

			ImageHyperlink link = toolkit.createImageHyperlink(buttons, SWT.NONE);
			link.setImage(SharedIcons.IMG_DELETE_OBJECT);
			link.setToolTipText(Messages.get().TableConditionsEditor_DeleteCondGroup);
			link.addHyperlinkListener(new HyperlinkAdapter() {
				@Override
				public void linkActivated(HyperlinkEvent e)
				{
					deleteGroup(GroupEditor.this);
				}
			});

			link = toolkit.createImageHyperlink(this, SWT.NONE);
			link.setImage(SharedIcons.IMG_ADD_OBJECT);
			link.setText(Messages.get().TableConditionsEditor_Add);
			link.setToolTipText(Messages.get().TableConditionsEditor_AddCond);
			link.addHyperlinkListener(new HyperlinkAdapter() {
				@Override
				public void linkActivated(HyperlinkEvent e)
				{
					addCondition(null);
				}
			});

		}
		
		/**
		 * Get consfigured conditions
		 * 
		 * @return
		 */
		public List<TableCondition> getConditions()
		{
			List<TableCondition> result = new ArrayList<TableCondition>(conditions.size());
			for(ConditionEditor ce : conditions)
			{
				result.add(ce.getCondition());
			}
			return result;
		}

		/**
		 * Add condition to group
		 */
		private void addCondition(TableCondition initialData)
		{
			ConditionEditor editor = new ConditionEditor(content, this);
			conditions.add(editor);
			if (initialData != null)
			{
				editor.column.setText(initialData.getColumn());
				editor.operation.select(initialData.getOperation());
				editor.value.setText(initialData.getValue());
			}
			else
			{
				form.reflow(true);
			}
		}
		
		/**
		 * @param editor
		 */
		void deleteCondition(ConditionEditor editor)
		{
			conditions.remove(editor);
			editor.dispose();
			form.reflow(true);
		}
	}
	
	/**
	 * Condition editor widget
	 */
	private class ConditionEditor
	{
		private CCombo column;
		private CCombo operation;
		private Text value;
		
		public ConditionEditor(Composite parent, final GroupEditor group)
		{
			column = new CCombo(parent, SWT.BORDER);
			toolkit.adapt(column);
			column.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

			operation = new CCombo(parent, SWT.BORDER | SWT.READ_ONLY);
			toolkit.adapt(operation);
			operation.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, false, false));
			operation.add(Messages.get().EditThresholdDialog_LT);
			operation.add(Messages.get().EditThresholdDialog_LE);
			operation.add(Messages.get().EditThresholdDialog_EQ);
			operation.add(Messages.get().EditThresholdDialog_GE);
			operation.add(Messages.get().EditThresholdDialog_GT);
			operation.add(Messages.get().EditThresholdDialog_NE);
			operation.add(Messages.get().EditThresholdDialog_LIKE);
			operation.add(Messages.get().EditThresholdDialog_NOTLIKE);
			
			value = toolkit.createText(parent, ""); //$NON-NLS-1$
			value.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
			
			ImageHyperlink link = toolkit.createImageHyperlink(parent, SWT.NONE);
			link.setImage(SharedIcons.IMG_DELETE_OBJECT);
			link.setToolTipText(Messages.get().TableConditionsEditor_DeleteCond);
			link.addHyperlinkListener(new HyperlinkAdapter() {
				@Override
				public void linkActivated(HyperlinkEvent e)
				{
					group.deleteCondition(ConditionEditor.this);
				}
			});
			link.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, false, false));
		}

		/**
		 * Dispose condition editor
		 */
		public void dispose()
		{
			column.dispose();
			operation.dispose();
			value.dispose();
		}

		/**
		 * @return
		 */
		public TableCondition getCondition()
		{
			return new TableCondition(column.getText(), operation.getSelectionIndex(), value.getText());
		}	
	}
}
