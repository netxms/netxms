/**
 * 
 */
package org.netxms.ui.eclipse.epp.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CLabel;
import org.eclipse.swt.custom.ScrolledComposite;
import org.eclipse.swt.events.ControlAdapter;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Text;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.netxms.client.NXCSession;
import org.netxms.client.ServerAction;
import org.netxms.client.constants.Severity;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.client.events.EventTemplate;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.epp.Activator;
import org.netxms.ui.eclipse.epp.views.EventProcessingPolicyEditor;
import org.netxms.ui.eclipse.nxsl.widgets.ScriptEditor;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.widgets.DashboardElement;

/**
 * Rule overview widget
 *
 */
public class RuleEditor extends Composite
{
	private static final Color BACKGROUND_COLOR = new Color(Display.getDefault(), 255, 255, 255);
	private static final Color TITLE_BACKGROUND_COLOR = new Color(Display.getDefault(), 153, 180, 209);
	private static final Color CONDITION_BORDER_COLOR = new Color(Display.getDefault(), 127,154,72);
	private static final Color ACTION_BORDER_COLOR = new Color(Display.getDefault(), 105,81,133);
	private static final Color COMMENTS_BORDER_COLOR = new Color(Display.getDefault(), 60,141,163);
	private static final Color TITLE_COLOR = new Color(Display.getDefault(), 255, 255, 255);
	private static final int INDENT = 20;
	
	private EventProcessingPolicyRule rule;
	private EventProcessingPolicyEditor editor;
	private NXCSession session;
	private WorkbenchLabelProvider labelProvider;
	private Image imageAlarm;
	private Image imageExecute;
	private Image imageTerminate;
	private Image imageStop;
	
	/**
	 * @param parent
	 * @param rule
	 */
	public RuleEditor(Composite parent, EventProcessingPolicyRule rule, EventProcessingPolicyEditor editor)
	{
		super(parent, SWT.NONE);
		this.rule = rule;
		this.editor = editor;
		
		session = (NXCSession)ConsoleSharedData.getSession();
		labelProvider = new WorkbenchLabelProvider();
		imageStop = Activator.getImageDescriptor("icons/stop.png").createImage();
		imageAlarm = Activator.getImageDescriptor("icons/alarm.png").createImage();
		imageExecute = Activator.getImageDescriptor("icons/execute.png").createImage();
		imageTerminate = Activator.getImageDescriptor("icons/terminate.png").createImage();
		
		setBackground(BACKGROUND_COLOR);

		GridLayout layout = new GridLayout();
		layout.numColumns = 2;
		setLayout(layout);
		
		createLeftPanel();
		createHeader();
		
		DashboardElement condition = new DashboardElement(this, "Filter") {
			@Override
			protected Control createClientArea(Composite parent)
			{
				setBorderColor(CONDITION_BORDER_COLOR);
				setTitleColor(TITLE_COLOR);
				return createConditionControl(parent, RuleEditor.this.rule);
			}
		};
		configureLayout(condition);

		DashboardElement action = new DashboardElement(this, "Action") {
			@Override
			protected Control createClientArea(Composite parent)
			{
				setBorderColor(ACTION_BORDER_COLOR);
				setTitleColor(TITLE_COLOR);
				return createActionControl(parent, RuleEditor.this.rule);
			}
		};
		configureLayout(action);

		DashboardElement comments = new DashboardElement(this, "Comments") {
			@Override
			protected Control createClientArea(Composite parent)
			{
				setBorderColor(COMMENTS_BORDER_COLOR);
				setTitleColor(TITLE_COLOR);
				
				final Text text = new Text(parent, SWT.MULTI | SWT.WRAP | SWT.READ_ONLY);
				text.setBackground(BACKGROUND_COLOR);
				text.setText(RuleEditor.this.rule.getComments());
				return text;
			}
		};
		configureLayout(comments);
	}
	
	private void createLeftPanel()
	{
		Label label = new Label(this, SWT.NONE);
		label.setText("1");
		label.setBackground(TITLE_BACKGROUND_COLOR);
		
		GridData gd = new GridData();
		gd.verticalSpan = 4;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		gd.widthHint = 40;
		label.setLayoutData(gd);
	}
	
	private void createHeader()
	{
		Label label = new Label(this, SWT.NONE);
		label.setText(rule.getComments());
		label.setBackground(TITLE_BACKGROUND_COLOR);
		label.setForeground(TITLE_COLOR);
		
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		label.setLayoutData(gd);
	}
	
	/**
	 * Configure layout for child element
	 * 
	 * @param ctrl child control
	 */
	private void configureLayout(Control ctrl)
	{
		GridData gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		ctrl.setLayoutData(gd);
	}
	
	/**
	 * Create condition summary control
	 * 
	 * @param parent
	 * @param rule
	 * @return
	 */
	private Control createConditionControl(Composite parent, final EventProcessingPolicyRule rule)
	{
		Composite clientArea = new Composite(parent, SWT.NONE);
		
		clientArea.setBackground(BACKGROUND_COLOR);
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = 0;
		clientArea.setLayout(layout);
		
		boolean needAnd = false;
		createLabel(clientArea, 0, true, "IF");
		
		/* events */
		if (rule.getEvents().size() > 0)
		{
			addConditionGroupLabel(clientArea, "event code is one of the following:", needAnd);
			
			for(Long code : rule.getEvents())
			{
				CLabel clabel = createCLabel(clientArea, 2, false);
				
				EventTemplate event = session.findEventTemplateByCode(code);
				if (event != null)
				{
					clabel.setText(event.getName());
					clabel.setImage(StatusDisplayInfo.getStatusImage(event.getSeverity()));
				}
				else
				{
					clabel.setText("<" + code.toString() + ">");
					clabel.setImage(StatusDisplayInfo.getStatusImage(Severity.UNKNOWN));
				}
			}
			needAnd = true;
		}
		
		/* source */
		if (rule.getSources().size() > 0)
		{
			addConditionGroupLabel(clientArea, "source object is one of the following:", needAnd);
			
			for(Long id : rule.getSources())
			{
				CLabel clabel = createCLabel(clientArea, 2, false);
				
				GenericObject object = session.findObjectById(id);
				if (object != null)
				{
					clabel.setText(object.getObjectName());
					clabel.setImage(labelProvider.getImage(object));
				}
				else
				{
					clabel.setText("[" + id.toString() + "]");
				}
			}
			needAnd = true;
		}
		
		/* severity */
		if ((rule.getFlags() & EventProcessingPolicyRule.SEVERITY_ANY) != EventProcessingPolicyRule.SEVERITY_ANY)
		{
			addConditionGroupLabel(clientArea, "event severity is one of the following:", needAnd);
			
			if ((rule.getFlags() & EventProcessingPolicyRule.SEVERITY_NORMAL) != 0)
				addSeverityLabel(clientArea, Severity.NORMAL);

			if ((rule.getFlags() & EventProcessingPolicyRule.SEVERITY_WARNING) != 0)
				addSeverityLabel(clientArea, Severity.WARNING);

			if ((rule.getFlags() & EventProcessingPolicyRule.SEVERITY_MINOR) != 0)
				addSeverityLabel(clientArea, Severity.MINOR);

			if ((rule.getFlags() & EventProcessingPolicyRule.SEVERITY_MAJOR) != 0)
				addSeverityLabel(clientArea, Severity.MAJOR);

			if ((rule.getFlags() & EventProcessingPolicyRule.SEVERITY_CRITICAL) != 0)
				addSeverityLabel(clientArea, Severity.CRITICAL);
			
			needAnd = true;
		}
		
		/* script */
		if ((rule.getScript() != null) && !rule.getScript().isEmpty())
		{
			addConditionGroupLabel(clientArea, "the following script returns true:", needAnd);
			
			ScriptEditor scriptEditor = new ScriptEditor(clientArea, SWT.BORDER);
			GridData gd = new GridData();
			gd.horizontalIndent = INDENT * 2;
			gd.horizontalAlignment = SWT.FILL;
			gd.grabExcessHorizontalSpace = true;
			scriptEditor.setLayoutData(gd);
			scriptEditor.setText(rule.getScript());
			scriptEditor.getTextWidget().setEditable(false);
		}
		
		return clientArea;
	}
	
	/**
	 * Create label with given text and indent
	 * 
	 * @param parent
	 * @param indent
	 * @param bold
	 * @param text
	 * @return
	 */
	private Label createLabel(Composite parent, int indent, boolean bold, String text)
	{
		Label label = new Label(parent, SWT.NONE);
		label.setBackground(BACKGROUND_COLOR);
		label.setFont(bold ? editor.getBoldFont() : editor.getNormalFont());
		label.setText(text);

		GridData gd = new GridData();
		gd.horizontalIndent = INDENT * indent;
		label.setLayoutData(gd);
		
		return label;
	}

	/**
	 * Create CLable with given indent
	 * 
	 * @param parent
	 * @param indent
	 * @param bold
	 * @return
	 */
	private CLabel createCLabel(Composite parent, int indent, boolean bold)
	{
		CLabel label = new CLabel(parent, SWT.NONE);
		label.setBackground(BACKGROUND_COLOR);
		label.setFont(bold ? editor.getBoldFont() : editor.getNormalFont());

		GridData gd = new GridData();
		gd.horizontalIndent = INDENT * indent;
		label.setLayoutData(gd);
		
		return label;
	}
	
	/**
	 * Add condition group opening text
	 * 
	 * @param parent parent composite
	 * @param title group's title
	 * @param needAnd true if AND clause have to be added
	 */
	private void addConditionGroupLabel(Composite parent, String title, boolean needAnd)
	{
		if (needAnd)
			createLabel(parent, 0, true, "AND");
		createLabel(parent, 1, false, title);
	}
	
	/**
	 * Add severity label
	 * 
	 * @param parent parent composite
	 * @param severity severity code
	 */
	private void addSeverityLabel(Composite parent, int severity)
	{
		CLabel clabel = createCLabel(parent, 2, false);
		clabel.setText(StatusDisplayInfo.getStatusText(severity));
		clabel.setImage(StatusDisplayInfo.getStatusImage(severity));
	}

	/**
	 * Create action summary control
	 * 
	 * @param parent
	 * @param rule
	 * @return
	 */
	private Control createActionControl(Composite parent, final EventProcessingPolicyRule rule)
	{
		Composite clientArea = new Composite(parent, SWT.NONE);
		
		clientArea.setBackground(BACKGROUND_COLOR);
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = 0;
		clientArea.setLayout(layout);
		
		/* alarm */
		if ((rule.getFlags() & EventProcessingPolicyRule.GENERATE_ALARM) != 0)
		{
			if (rule.getAlarmSeverity() < Severity.UNMANAGED)
			{
				addActionGroupLabel(clientArea, "Generate alarm", imageAlarm);
				
				CLabel clabel = createCLabel(clientArea, 1, false);
				clabel.setImage(StatusDisplayInfo.getStatusImage(rule.getAlarmSeverity()));
				clabel.setText(rule.getAlarmMessage());
				
				if ((rule.getAlarmKey() != null) && !rule.getAlarmKey().isEmpty())
				{
					createLabel(clientArea, 1, false, "with key \"" + rule.getAlarmKey() + "\"");
				}
			}
			else
			{
				addActionGroupLabel(clientArea, "Terminate alarms with key \"" + rule.getAlarmKey() + "\"", imageTerminate);
			}
		}
		
		/* actions */
		if (rule.getActions().size() > 0)
		{
			addActionGroupLabel(clientArea, "Execute the following predefined actions:", imageExecute);
			for(Long id : rule.getActions())
			{
				CLabel clabel = createCLabel(clientArea, 1, false);
				ServerAction action = editor.findActionById(id);
				if (action != null)
				{
					clabel.setText(action.getName());
					clabel.setImage(labelProvider.getImage(action));
				}
				else
				{
					clabel.setText("<" + id.toString() + ">");
				}
			}
		}
		
		/* flags */
		if ((rule.getFlags() & EventProcessingPolicyRule.STOP_PROCESSING) != 0)
		{
			addActionGroupLabel(clientArea, "Stop event processing", imageStop);
		}
		
		return clientArea;
	}

	/**
	 * Add condition group opening text
	 * 
	 * @param parent parent composite
	 * @param title group's title
	 * @param needAnd true if AND clause have to be added
	 */
	private void addActionGroupLabel(Composite parent, String title, Image image)
	{
		CLabel label = createCLabel(parent, 0, true);
		label.setImage(image);
		label.setText(title);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.widgets.Widget#dispose()
	 */
	@Override
	public void dispose()
	{
		labelProvider.dispose();
		imageStop.dispose();
		imageAlarm.dispose();
		imageExecute.dispose();
		imageTerminate.dispose();
		super.dispose();
	}
}
