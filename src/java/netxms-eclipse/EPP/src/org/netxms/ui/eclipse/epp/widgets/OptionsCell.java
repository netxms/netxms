/**
 * 
 */
package org.netxms.ui.eclipse.epp.widgets;

import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CLabel;
import org.eclipse.swt.events.ControlAdapter;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.graphics.Point;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.ui.eclipse.epp.Activator;

/**
 * @author victor
 *
 */
public class OptionsCell extends Cell
{
	private EventProcessingPolicyRule eppRule;
	private CLabel labelNone;
	private CLabel labelStop;
	private Action actionStopProcessing;

	/**
	 * @param rule
	 * @param data
	 */
	public OptionsCell(Rule rule, Object data)
	{
		super(rule, data);
		eppRule = (EventProcessingPolicyRule)data;

		labelNone = new CLabel(this, SWT.LEFT);
		labelNone.setText("None");
		labelNone.setImage(Activator.getImageDescriptor("icons/none.gif").createImage());
		labelNone.setBackground(PolicyEditor.COLOR_BACKGROUND);
		
		labelStop = new CLabel(this, SWT.LEFT);
		labelStop.setText("Stop processing");
		labelStop.setImage(Activator.getImageDescriptor("icons/stop.png").createImage());
		labelStop.setBackground(PolicyEditor.COLOR_BACKGROUND);
		
		setLayout(null);
		addControlListener(new ControlAdapter() {
			/* (non-Javadoc)
			 * @see org.eclipse.swt.events.ControlAdapter#controlResized(org.eclipse.swt.events.ControlEvent)
			 */
			@Override
			public void controlResized(ControlEvent e)
			{
				final Point size = getSize();
				labelNone.setSize(size);
				labelStop.setSize(size);
			}
		});
		
		makeActions();
		createContextMenu();

		updateOptionList();
	}

	/**
	 * Make actions
	 */
	private void makeActions()
	{
		actionStopProcessing = new Action() {
			/* (non-Javadoc)
			 * @see org.eclipse.jface.action.Action#run()
			 */
			@Override
			public void run()
			{
				int flags = eppRule.getFlags();
				if ((flags & EventProcessingPolicyRule.STOP_PROCESSING) != 0)
				{
					flags &= ~EventProcessingPolicyRule.STOP_PROCESSING;
				}
				else
				{
					flags |= EventProcessingPolicyRule.STOP_PROCESSING;
				}
				eppRule.setFlags(flags);
				setChecked((flags & EventProcessingPolicyRule.STOP_PROCESSING) != 0);
				contentChanged();
			}
		};
		actionStopProcessing.setText("Stop processing");
		actionStopProcessing.setChecked((eppRule.getFlags() & EventProcessingPolicyRule.STOP_PROCESSING) != 0);
	}
	
	/**
	 * Create context menu for cell 
	 */
	private void createContextMenu()
	{
		MenuManager menuMgr = new MenuManager();
		menuMgr.setRemoveAllWhenShown(true);
		menuMgr.addMenuListener(new IMenuListener()
		{
			public void menuAboutToShow(IMenuManager mgr)
			{
				fillContextMenu(mgr);
			}
		});

		// Create menu
		labelNone.setMenu(menuMgr.createContextMenu(labelNone));
		labelStop.setMenu(menuMgr.createContextMenu(labelStop));
	}

	/**
	 * Fill cell's context menu
	 * 
	 * @param mgr Menu manager
	 */
	private void fillContextMenu(IMenuManager mgr)
	{
		mgr.add(actionStopProcessing);
	}

	/**
	 * Update list of selected options
	 */
	private void updateOptionList()
	{
		if ((eppRule.getFlags() & EventProcessingPolicyRule.STOP_PROCESSING) != 0)
		{
			labelNone.setVisible(false);
			labelStop.setVisible(true);
		}
		else
		{
			labelNone.setVisible(true);
			labelStop.setVisible(false);
		}
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.epp.widgets.Cell#contentChanged()
	 */
	@Override
	protected void contentChanged()
	{
		updateOptionList();
		super.contentChanged();
	}
}
