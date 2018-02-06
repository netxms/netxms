/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.ui.eclipse.alarmviewer.objecttabs;

import org.eclipse.core.commands.Command;
import org.eclipse.core.commands.State;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.util.IPropertyChangeListener;
import org.eclipse.jface.util.PropertyChangeEvent;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.commands.ICommandService;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.alarmviewer.Activator;
import org.netxms.ui.eclipse.alarmviewer.widgets.AlarmList;
import org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab;
import org.netxms.ui.eclipse.tools.VisibilityValidator;

/**
 * Alarm tab
 */
public class AlarmTab extends ObjectTab
{
	private AlarmList alarmList;
	private IPropertyChangeListener propertyChangeListener;
	private boolean initShowFilter = true;
	
	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#createTabContent(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected void createTabContent(Composite parent)
	{
      final IDialogSettings settings = Activator.getDefault().getDialogSettings();
      initShowFilter = settings.getBoolean("AlarmTab.showFilter");
      
      alarmList = new AlarmList(getViewPart(), parent, SWT.NONE, getConfigPrefix(), new VisibilityValidator() {
         @Override
         public boolean isVisible()
         {
            return isActive();
         }
      });
      
      alarmList.addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            settings.put("AlarmTab.showFilter", alarmList.isFilterEnabled());
         }
      });

      // Force update of "show colors" menu item in object tabbed view 
      // when settings changes in any alarm viewer
      propertyChangeListener = new IPropertyChangeListener() {
         @Override
         public void propertyChange(PropertyChangeEvent event)
         {
            if (event.getProperty().equals("SHOW_ALARM_STATUS_COLORS")) //$NON-NLS-1$
            {
               ICommandService service = (ICommandService)PlatformUI.getWorkbench().getService(ICommandService.class);
               service.refreshElements("org.netxms.ui.eclipse.alarmviewer.commands.show_status_colors", null);
            }
         }
      };
      Activator.getDefault().getPreferenceStore().addPropertyChangeListener(propertyChangeListener);
      
      alarmList.setFilterCloseAction(new Action() {
         @Override
         public void run()
         {
            alarmList.enableFilter(false);
            ICommandService service = (ICommandService)PlatformUI.getWorkbench().getService(ICommandService.class);
            Command command = service.getCommand("org.netxms.ui.eclipse.alarmviewer.commands.show_filter_alarm_list"); //$NON-NLS-1$
            State state = command.getState("org.netxms.ui.eclipse.alarmviewer.commands.show_filter_alarm_list.state"); //$NON-NLS-1$
            state.setValue(false);
            service.refreshElements(command.getId(), null);
         }
      });
      
      alarmList.enableFilter(initShowFilter);
	}
	
	/**
	 * Get configuration prefix for alarm list.
	 * 
	 * @return
	 */
	protected String getConfigPrefix()
	{
		return "AlarmTab"; //$NON-NLS-1$
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#objectChanged(org.netxms.client.objects.AbstractObject)
	 */
	@Override
	public void objectChanged(AbstractObject object)
	{
		if ((object != null) && isActive())
			alarmList.setRootObject(object.getObjectId());
	}

	/* (non-Javadoc)
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#selected()
    */
   @Override
   public void selected()
   {
      super.selected();
      if (getObject() != null)
         alarmList.setRootObject(getObject().getObjectId());
   }

   /* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#refresh()
	 */
	@Override
	public void refresh()
	{
		alarmList.refresh();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#getSelectionProvider()
	 */
	@Override
	public ISelectionProvider getSelectionProvider()
	{
		return alarmList.getSelectionProvider();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#showForObject(org.netxms.client.objects.AbstractObject)
	 */
	@Override
	public boolean showForObject(AbstractObject object)
	{
      return object.isAlarmsVisible();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#dispose()
	 */
	@Override
	public void dispose()
	{
	   Activator.getDefault().getPreferenceStore().removePropertyChangeListener(propertyChangeListener);
		if (alarmList != null)
			alarmList.dispose();
		super.dispose();
	}
	
	/**
	 * Enable disable status color show
	 * 
	 * @param show
	 */
	public void setShowColors(boolean show)
	{
	   alarmList.setShowColors(show);
	}
	
	  /**
    * @param enabled
    */
   public void setFilterEnabled(boolean enabled)
   {
      alarmList.enableFilter(enabled);
   }
}
