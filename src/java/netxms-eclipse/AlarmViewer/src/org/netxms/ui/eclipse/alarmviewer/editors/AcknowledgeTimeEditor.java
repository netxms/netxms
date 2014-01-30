package org.netxms.ui.eclipse.alarmviewer.editors;

import java.util.LinkedList;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.preference.FieldEditor;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.List;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Widget;
import org.netxms.ui.eclipse.alarmviewer.Activator;
import org.netxms.ui.eclipse.alarmviewer.dialogs.AcknowledgeCustomTimeDialog;
import org.netxms.ui.eclipse.alarmviewer.widgets.helpers.AlarmAcknowledgeTimeFunctions;

class ListActions
{
   public final static int ADD = 0;
   public final static int DELETE = 1;

   public int action;
   public int element;
   public int value;
}

public class AcknowledgeTimeEditor extends FieldEditor
{
   /**
    * The list widget; <code>null</code> if none (before creation or after disposal).
    */
   private List list;

   /**
    * The button box containing the Add, Remove, Up, and Down buttons; <code>null</code> if none (before creation or after
    * disposal).
    */
   private Composite buttonBox;

   /**
    * The Add button.
    */
   private Button addButton;

   /**
    * The Remove button.
    */
   private Button removeButton;

   /**
    * The selection listener.
    */
   private SelectionListener selectionListener;

   /**
    * Creates a new path field editor
    */
   protected AcknowledgeTimeEditor()
   {
   }

   /**
    * Creates list of actions to be applied at the save time
    */
   LinkedList<ListActions> actions = new LinkedList<ListActions>();

   /**
    * Creates a path field editor.
    * 
    * @param name the name of the preference this field editor works on
    * @param labelText the label text of the field editor
    * @param dirChooserLabelText the label text displayed for the directory chooser
    * @param parent the parent of the field editor's control
    */
   public AcknowledgeTimeEditor(String name, String labelText, Composite parent)
   {
      init(name, labelText);
      createControl(parent);
   }

   /**
    * Notifies that the Add button has been pressed.
    */
   private void addPressed()
   {
      setPresentsDefaultValue(false);
      String input = null;
      int time = 0;
      AcknowledgeCustomTimeDialog dlg = new AcknowledgeCustomTimeDialog(getShell());
      if (dlg.open() == Window.OK)
      {
         time = dlg.getTime();
         if (time > 0)
            input = AlarmAcknowledgeTimeFunctions.timeToString(time);
      }

      if (input != null)
      {
         int index = list.getItemCount();
         list.add(input, index);

         ListActions act = new ListActions();
         act.action = ListActions.ADD;
         act.element = index;
         act.value = time;
         actions.add(act);
      }
   }

   /*
    * (non-Javadoc) Method declared on FieldEditor.
    */
   protected void adjustForNumColumns(int numColumns)
   {
      Control control = getLabelControl();
      ((GridData)control.getLayoutData()).horizontalSpan = numColumns;
      ((GridData)list.getLayoutData()).horizontalSpan = numColumns - 1;
   }

   /**
    * Creates the Add, Remove, Up, and Down button in the given button box.
    * 
    * @param box the box for the buttons
    */
   private void createButtons(Composite box)
   {
      addButton = createPushButton(box, "ListEditor.add");//$NON-NLS-1$
      removeButton = createPushButton(box, "ListEditor.remove");//$NON-NLS-1$
   }

   /**
    * Helper method to create a push button.
    * 
    * @param parent the parent control
    * @param key the resource name used to supply the button's label text
    * @return Button
    */
   private Button createPushButton(Composite parent, String key)
   {
      Button button = new Button(parent, SWT.PUSH);
      button.setText(JFaceResources.getString(key));
      button.setFont(parent.getFont());
      GridData data = new GridData(GridData.FILL_HORIZONTAL);
      int widthHint = convertHorizontalDLUsToPixels(button, IDialogConstants.BUTTON_WIDTH);
      data.widthHint = Math.max(widthHint, button.computeSize(SWT.DEFAULT, SWT.DEFAULT, true).x);
      button.setLayoutData(data);
      button.addSelectionListener(getSelectionListener());
      return button;
   }

   /**
    * Creates a selection listener.
    */
   public void createSelectionListener()
   {
      selectionListener = new SelectionAdapter() {
         public void widgetSelected(SelectionEvent event)
         {
            Widget widget = event.widget;
            if (widget == addButton)
            {
               addPressed();
            }
            else if (widget == removeButton)
            {
               removePressed();
            }
            else if (widget == list)
            {
               selectionChanged();
            }
         }
      };
   }

   /*
    * (non-Javadoc) Method declared on FieldEditor.
    */
   protected void doFillIntoGrid(Composite parent, int numColumns)
   {
      Control control = getLabelControl(parent);
      GridData gd = new GridData();
      gd.horizontalSpan = numColumns;
      control.setLayoutData(gd);

      list = getListControl(parent);
      gd = new GridData(GridData.FILL_HORIZONTAL);
      gd.verticalAlignment = GridData.FILL;
      gd.horizontalSpan = numColumns - 1;
      gd.grabExcessHorizontalSpace = true;
      list.setLayoutData(gd);

      buttonBox = getButtonBoxControl(parent);
      gd = new GridData();
      gd.verticalAlignment = GridData.BEGINNING;
      buttonBox.setLayoutData(gd);
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.preference.FieldEditor#doLoadDefault()
    */
   @Override
   protected void doLoadDefault()
   {
      if (list != null)
      {
         list.removeAll();
         String[] array = AlarmAcknowledgeTimeFunctions.getValues();
         for(int i = 0; i < array.length; i++)
         {
            list.add(array[i]);
         }
      }
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.preference.FieldEditor#doLoad()
    */
   @Override
   protected void doLoad()
   {
      if (list != null)
      {
         list.removeAll();
         String[] array = AlarmAcknowledgeTimeFunctions.getValues();
         if (array != null)
         {
            for(int i = 0; i < array.length; i++)
            {
               list.add(array[i]);
            }
         }
      }
   }

   /*
    * (non-Javadoc) Method declared on FieldEditor.
    */
   protected void doStore()
   {
      IDialogSettings settings = Activator.getDefault().getDialogSettings();
      int menuSize;
      try
      {
         menuSize = settings.getInt("AlarmList.ackMenuSize"); //$NON-NLS-1$
      }
      catch(NumberFormatException e)
      {
         menuSize = 0;
      }
      ListActions act = new ListActions();

      while(!actions.isEmpty())
      {
         act = actions.removeFirst();
         switch(act.action)
         {
            case ListActions.ADD:
               settings.put("AlarmList.ackMenuEntry" + act.element, act.value); //$NON-NLS-1$
               settings.put("AlarmList.ackMenuSize", ++menuSize); //$NON-NLS-1$
               break;
            case ListActions.DELETE:
               settings.put("AlarmList.ackMenuSize", --menuSize); //$NON-NLS-1$
               for(int i = act.element; i < menuSize; i++)
               {
                  settings.put("AlarmList.ackMenuEntry" + i, settings.getInt("AlarmList.ackMenuEntry" + (i + 1))); //$NON-NLS-1$ //$NON-NLS-2$
               }
               break;
         }
      }
      // apply to list all changes
   }

   /**
    * Returns this field editor's button box containing the Add, Remove, Up, and Down button.
    * 
    * @param parent the parent control
    * @return the button box
    */
   public Composite getButtonBoxControl(Composite parent)
   {
      if (buttonBox == null)
      {
         buttonBox = new Composite(parent, SWT.NULL);
         GridLayout layout = new GridLayout();
         layout.marginWidth = 0;
         buttonBox.setLayout(layout);
         createButtons(buttonBox);
         buttonBox.addDisposeListener(new DisposeListener() {
            public void widgetDisposed(DisposeEvent event)
            {
               addButton = null;
               removeButton = null;
               buttonBox = null;
            }
         });

      }
      else
      {
         checkParent(buttonBox, parent);
      }

      selectionChanged();
      return buttonBox;
   }

   /**
    * Returns this field editor's list control.
    * 
    * @param parent the parent control
    * @return the list control
    */
   public List getListControl(Composite parent)
   {
      if (list == null)
      {
         list = new List(parent, SWT.BORDER | SWT.SINGLE | SWT.V_SCROLL | SWT.H_SCROLL);
         list.setFont(parent.getFont());
         list.addSelectionListener(getSelectionListener());
         list.addDisposeListener(new DisposeListener() {
            public void widgetDisposed(DisposeEvent event)
            {
               list = null;
            }
         });
      }
      else
      {
         checkParent(list, parent);
      }
      return list;
   }

   /*
    * (non-Javadoc) Method declared on FieldEditor.
    */
   public int getNumberOfControls()
   {
      return 2;
   }

   /**
    * Returns this field editor's selection listener. The listener is created if nessessary.
    * 
    * @return the selection listener
    */
   private SelectionListener getSelectionListener()
   {
      if (selectionListener == null)
      {
         createSelectionListener();
      }
      return selectionListener;
   }

   /**
    * Returns this field editor's shell.
    * <p>
    * This method is internal to the framework; subclassers should not call this method.
    * </p>
    * 
    * @return the shell
    */
   protected Shell getShell()
   {
      if (addButton == null)
      {
         return null;
      }
      return addButton.getShell();
   }

   /**
    * Notifies that the Remove button has been pressed.
    */
   private void removePressed()
   {
      setPresentsDefaultValue(false);
      int index = list.getSelectionIndex();

      ListActions act = new ListActions();
      act.action = ListActions.DELETE;
      act.element = index;
      actions.add(act);

      if (index >= 0)
      {
         list.remove(index);
         selectionChanged();
      }
   }

   /**
    * Invoked when the selection in the list has changed.
    * 
    * <p>
    * The default implementation of this method utilizes the selection index and the size of the list to toggle the enablement of
    * the up, down and remove buttons.
    * </p>
    * 
    * <p>
    * Sublcasses may override.
    * </p>
    * 
    * @since 3.5
    */
   protected void selectionChanged()
   {

      int index = list.getSelectionIndex();
      removeButton.setEnabled(index >= 0);
   }

   /*
    * (non-Javadoc) Method declared on FieldEditor.
    */
   public void setFocus()
   {
      if (list != null)
      {
         list.setFocus();
      }
   }

   /*
    * @see FieldEditor.setEnabled(boolean,Composite).
    */
   public void setEnabled(boolean enabled, Composite parent)
   {
      super.setEnabled(enabled, parent);
      getListControl(parent).setEnabled(enabled);
      addButton.setEnabled(enabled);
      removeButton.setEnabled(enabled);
   }

   /**
    * Return the Add button.
    * 
    * @return the button
    * @since 3.5
    */
   protected Button getAddButton()
   {
      return addButton;
   }

   /**
    * Return the Remove button.
    * 
    * @return the button
    * @since 3.5
    */
   protected Button getRemoveButton()
   {
      return removeButton;
   }

   /**
    * Return the List.
    * 
    * @return the list
    * @since 3.5
    */
   protected List getList()
   {
      return list;
   }
}
