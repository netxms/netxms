/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objecttools.views;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.InputField;
import org.netxms.client.TextOutputAdapter;
import org.netxms.client.TextOutputListener;
import org.netxms.client.constants.InputFieldType;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.nxmc.Memento;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.views.ViewNotRestoredException;
import org.netxms.nxmc.base.widgets.TextConsole;
import org.netxms.nxmc.base.widgets.TextConsole.IOConsoleOutputStream;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.ObjectContext;
import org.netxms.nxmc.modules.objects.dialogs.InputFieldEntryDialog;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Abstract view to display command execution results
 */
public abstract class AbstractCommandResultView extends ObjectToolResultView
{
   private I18n i18n = LocalizationHelper.getI18n(AbstractCommandResultView.class);

   protected Map<String, String> inputValues;
   protected List<String> maskedFields;
   protected TextConsole console;
   protected long streamId = 0;
   protected boolean askInputValues = false;
   protected boolean viewRestored = false;

   private IOConsoleOutputStream out;
   private TextOutputListener outputListener;

	private Action actionClear;
	private Action actionScrollLock;
	private Action actionCopy;
	private Action actionSelectAll;
   protected Action actionRestart;


	/**
	 * Constructor
	 * 
	 * @param image
	 * @param id
	 * @param hasFilter
	 * @param nodeId
	 * @param toolId
	 */
   public AbstractCommandResultView(ObjectContext node, ObjectTool tool, final Map<String, String> inputValues, final List<String> maskedFields) 
   {
      super(node, tool, ResourceManager.getImageDescriptor("icons/object-tools/terminal.png"), false);
      this.inputValues = inputValues;
      this.maskedFields = maskedFields;

      outputListener = new TextOutputAdapter() {
         @Override
         public void messageReceived(String text)
         {
            try
            {
               if (out != null)
                  out.write(text.replace("\r", ""));
            }
            catch(IOException e)
            {
            }
         }

         @Override
         public void setStreamId(long streamId)
         {
            AbstractCommandResultView.this.streamId = streamId;
         }
      };
   }

   /**
    * Clone constructor
    */
   protected AbstractCommandResultView()
   {
      super();
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#cloneView()
    */
   @Override
   public View cloneView()
   {
      AbstractCommandResultView view = (AbstractCommandResultView)super.cloneView();
      view.inputValues = inputValues;
      view.maskedFields = maskedFields;

      view.outputListener = new TextOutputAdapter() {
         @Override
         public void messageReceived(String text)
         {
            try
            {
               if (view.out != null)
                  view.out.write(text.replace("\r", ""));
            }
            catch(IOException e)
            {
            }
         }

         @Override
         public void setStreamId(long streamId)
         {
            view.streamId = streamId;
         }
      };
      return view;
   } 
   
   
   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#postClone(org.netxms.nxmc.base.views.View)
    */
   @Override
   protected void postClone(View view)
   {
      super.postClone(view);
      AbstractCommandResultView origin = (AbstractCommandResultView)view;
      console.setText(origin.console.getText());
      actionRestart.setEnabled(true);
   }


   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
	{
		console = new TextConsole(parent, SWT.NONE);
		console.addSelectionChangedListener(new ISelectionChangedListener() {
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
				actionCopy.setEnabled(console.canCopy());
			}
		});

		createActions();
		createPopupMenu();
	}   
   
	/**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#postContentCreate()
    */
   @Override
   protected void postContentCreate()
   {
      super.postContentCreate();
      if (!viewRestored)
         execute();
      else
         actionRestart.setEnabled(true);
   }

   /**
	 * Create actions
	 */
	protected void createActions()
	{
		actionClear = new Action(i18n.tr("C&lear console"), SharedIcons.CLEAR_LOG) {
			@Override
			public void run()
			{
				console.clear();
			}
		};

		actionScrollLock = new Action(i18n.tr("&Scroll lock"), Action.AS_CHECK_BOX) {
			@Override
			public void run()
			{
			   console.setAutoScroll(!actionScrollLock.isChecked());
			}
		};
		actionScrollLock.setImageDescriptor(ResourceManager.getImageDescriptor("icons/scroll-lock.png")); 
		actionScrollLock.setChecked(false);

		actionCopy = new Action(i18n.tr("&Copy")) {
			@Override
			public void run()
			{
			   console.copy();
			}
		};
		actionCopy.setEnabled(false);

		actionSelectAll = new Action(i18n.tr("Select &all")) {
			@Override
			public void run()
			{
			   console.selectAll();
			}
		};

      actionRestart = new Action(i18n.tr("&Restart"), SharedIcons.RESTART) {
         @Override
         public void run()
         {
            reExecute();
         }
      };
      actionRestart.setEnabled(false);
	}
	
	/**
	 * Re execute tool 
	 */
	private void reExecute() 
	{	   
	   if ((tool.getFlags() & ObjectTool.ASK_CONFIRMATION) != 0)
	   {
	      new Job(String.format(i18n.tr("Expand confirmation message for node %s"), object.object.getObjectName()), this) {
            
            @Override
            protected void run(IProgressMonitor monitor) throws Exception
            {
               List<String> textToExpand = new ArrayList<String>();
               textToExpand.add(tool.getConfirmationText());
               final List<String> expandedText = session.substituteMacros(object, textToExpand, inputValues);
               runInUIThread(new Runnable() {                  
                  @Override
                  public void run()
                  {
                     if (MessageDialogHelper.openQuestion(Registry.getMainWindowShell(), i18n.tr("Confirm Tool Execution"), expandedText.get(0)))
                     {
                        AbstractCommandResultView.this.execute();
                     }                     
                  }
               });
            }
            
            @Override
            protected String getErrorMessage()
            {
               return String.format(i18n.tr("Failed to expand confirmation message for node %s"), object.object.getObjectName());
            }
         }.start();
      }
	   else
	   {
	      execute();
	   }      
	}
	
	/**
	 * Execute action
	 */
	protected abstract void execute();

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolbar(org.eclipse.jface.action.ToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionRestart);
      manager.add(new Separator());
      manager.add(actionClear);
      manager.add(actionScrollLock);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(org.eclipse.jface.action.MenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionRestart);
      manager.add(new Separator());
      manager.add(actionClear);
      manager.add(actionScrollLock);
      manager.add(new Separator());
      manager.add(actionSelectAll);
      manager.add(actionCopy);
   }

	/**
	 * Create pop-up menu
	 */
	private void createPopupMenu()
	{
		// Create menu manager
		MenuManager menuMgr = new MenuManager();
		menuMgr.setRemoveAllWhenShown(true);
		menuMgr.addMenuListener(new IMenuListener() {
			public void menuAboutToShow(IMenuManager mgr)
			{
				fillContextMenu(mgr);
			}
		});

		// Create menu
		Menu menu = menuMgr.createContextMenu(console);
		console.setMenu(menu);
	}

	/**
	 * Fill context menu
	 * 
	 * @param mgr Menu manager
	 */
	protected void fillContextMenu(final IMenuManager manager)
	{
      manager.add(actionRestart);
      manager.add(new Separator());
		manager.add(actionClear);
		manager.add(actionScrollLock);
		manager.add(new Separator());
		manager.add(actionSelectAll);
		manager.add(actionCopy);
	}

	/**
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		console.setFocus();
	}	

   /**
    * @see org.netxms.nxmc.base.views.View#dispose()
    */
   @Override
   public void dispose()
   {
      closeOutputStream();
      super.dispose();
   }

   /**
    * Get output listener
    *
    * @return output listener
    */
   protected TextOutputListener getOutputListener()
   {
      return outputListener;
   }

   /**
    * Open new output stream (will close existing one if any)
    */
   protected void createOutputStream()
   {
      closeOutputStream();
      out = console.newOutputStream();
   }

   /**
    * Close output stream
    */
   protected void closeOutputStream()
   {
      if (out == null)
         return;

      try
      {
         out.close();
      }
      catch(IOException e)
      {
      }
      out = null;
   }

   /**
    * Write text to output stream.
    *
    * @param text text to write
    */
   protected void writeToOutputStream(String text)
   {
      if (out == null)
         return;

      try
      {
         out.write(text);
      }
      catch(IOException e)
      {
      }
   }
   
   /**
    * Restore user input values if required. 
    * Is used to restore masked fields after console restart
    * 
    * @return true if fields was successfully restored
    */
   protected boolean restoreUserInputFields()
   {
      if (!askInputValues)
         return true;      
      
      final InputField[] fields = tool.getInputFields();
      if (fields.length > 0)
      {
         Arrays.sort(fields, new Comparator<InputField>() {
            @Override
            public int compare(InputField f1, InputField f2)
            {
               return f1.getSequence() - f2.getSequence();
            }
         });
         InputFieldEntryDialog dlg = new InputFieldEntryDialog(getWindow().getShell(), tool.getDisplayName(), fields, inputValues);
         if (dlg.open() != Window.OK)
            return false; //canceled
         inputValues = dlg.getValues();
         maskedFields.clear();
         for (int i = 0; i < fields.length; i++)
         {
            if (fields[i].getType() == InputFieldType.PASSWORD)
            {
               maskedFields.add(fields[i].getName());
            }
         }
      }
      askInputValues = false;
      return true;
   }
   
   /**
    * @see org.netxms.nxmc.base.views.Perspective#saveState(org.netxms.nxmc.Memento)
    */
   @Override
   public void saveState(Memento memento)
   {      
      super.saveState(memento);
      memento.set("inputValues.keys", inputValues.keySet());
      List<String> values = new ArrayList<String>();
      for (Entry<String, String> v : inputValues.entrySet())
      {
         if (maskedFields.contains(v.getKey()))
         {
            values.add("");
         }
         else
         {
            values.add(v.getValue());
         }
      }
      memento.set("inputValues.values", values);
      if (maskedFields != null)
         memento.set("maskedFields", maskedFields);
   }
   
   /**
    * @throws ViewNotRestoredException 
    * @see org.netxms.nxmc.base.views.Perspective#restoreState(org.netxms.nxmc.Memento)
    */
   @Override
   public void restoreState(Memento memento) throws ViewNotRestoredException
   {
      super.restoreState(memento);
      List<String> keys = memento.getAsStringList("inputValues.keys");
      List<String> values = memento.getAsStringList("inputValues.values");
      inputValues = new HashMap<String, String>();
      for (int i = 0; i < keys.size() && i < values.size(); i++)
      {
         inputValues.put(keys.get(i), values.get(i));
      }
      
      maskedFields = memento.getAsStringList("maskedFields");
      askInputValues = maskedFields.size() > 0;
      viewRestored = true;

      outputListener = new TextOutputAdapter() {
         @Override
         public void messageReceived(String text)
         {
            try
            {
               if (out != null)
                  out.write(text.replace("\r", ""));
            }
            catch(IOException e)
            {
            }
         }

         @Override
         public void setStreamId(long streamId)
         {
            AbstractCommandResultView.this.streamId = streamId;
         }
      };
   }
}
