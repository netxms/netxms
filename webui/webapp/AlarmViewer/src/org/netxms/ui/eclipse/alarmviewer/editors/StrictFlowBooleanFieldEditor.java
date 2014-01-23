package org.netxms.ui.eclipse.alarmviewer.editors;

import org.eclipse.jface.preference.FieldEditor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

public class StrictFlowBooleanFieldEditor extends FieldEditor 
{   
   /**
    * The previously selected, or "before", value.
    */
   private boolean wasSelected;

   /**
    * The checkbox control, or <code>null</code> if none.
    */
   private Button checkBox = null;
   
   /**
    * The current session object
    */
   private NXCSession session = null;
   
   /**
    * Indicates if field is changed
    */
   private boolean changed = false;

   /**
    * Creates a new boolean field editor
    */
   protected StrictFlowBooleanFieldEditor() {
      session = (NXCSession)ConsoleSharedData.getSession();
   }

   /**
    * Creates a boolean field editor in the default style.
    * 
    * @param name
    *            the name of the preference this field editor works on
    * @param label
    *            the label text of the field editor
    * @param parent
    *            the parent of the field editor's control
    */
   public StrictFlowBooleanFieldEditor(String name, String label, Composite parent) {
      init(name, label);
      createControl(parent);
      session = (NXCSession)ConsoleSharedData.getSession();
   }

   /*
    * (non-Javadoc) Method declared on FieldEditor.
    */
   protected void adjustForNumColumns(int numColumns) {
      ((GridData) checkBox.getLayoutData()).horizontalSpan = numColumns;
   }

   /*
    * (non-Javadoc) Method declared on FieldEditor.
    */
   protected void doFillIntoGrid(Composite parent, int numColumns) {
      String text = getLabelText();
      checkBox = getChangeControl(parent);
      GridData gd = new GridData();
      gd.horizontalSpan = numColumns;
      checkBox.setLayoutData(gd);
      if (text != null) 
      {
         checkBox.setText(text);
      }
   }

   /*
    * (non-Javadoc) Method declared on FieldEditor. Loads the value from the
    * preference store and sets it to the check box.
    */
   protected void doLoad() {
      if (checkBox != null) {
         boolean value = session.getAlarmStatusFlowStrict() != 0;
         checkBox.setSelection(value);
         wasSelected = value;
      }
   }

   /*
    * (non-Javadoc) Method declared on FieldEditor. Loads the default value
    * from the preference store and sets it to the check box.
    */
   protected void doLoadDefault() {
      if (checkBox != null) {
         boolean value = session.getAlarmStatusFlowStrict() != 0;
         checkBox.setSelection(value);
         wasSelected = value;
      }
   }

   /*
    * (non-Javadoc) Method declared on FieldEditor.
    */
   protected void doStore() {
      if (changed)
      {
         try
         {
            session.setAlarmFlowState(checkBox.getSelection() ? 1 : 0);
         }
         catch(Exception e)
         {
            e.printStackTrace();
         }
      }
   }

   /**
    * Returns this field editor's current value.
    * 
    * @return the value
    */
   public boolean getBooleanValue() {
      return checkBox.getSelection();
   }

   /**
    * Returns the change button for this field editor.
    * 
    * @param parent
    *            The Composite to create the receiver in.
    * 
    * @return the change button
    */
   protected Button getChangeControl(Composite parent) {
      if (checkBox == null) {
         checkBox = new Button(parent, SWT.CHECK | SWT.LEFT);
         checkBox.setFont(parent.getFont());
         checkBox.addSelectionListener(new SelectionAdapter() {
            public void widgetSelected(SelectionEvent e) {
               boolean isSelected = checkBox.getSelection();
               valueChanged(wasSelected, isSelected);
               wasSelected = isSelected;
            }
         });
         checkBox.addDisposeListener(new DisposeListener() {
            public void widgetDisposed(DisposeEvent event) {
               checkBox = null;
            }
         });
      } else {
         checkParent(checkBox, parent);
      }
      return checkBox;
   }

   /*
    * (non-Javadoc) Method declared on FieldEditor.
    */
   public int getNumberOfControls() {
      return 1;
   }

   /*
    * (non-Javadoc) Method declared on FieldEditor.
    */
   public void setFocus() {
      if (checkBox != null) {
         checkBox.setFocus();
      }
   }

   /*
    * (non-Javadoc) Method declared on FieldEditor.
    */
   public void setLabelText(String text) {
      super.setLabelText(text);
      Label label = getLabelControl();
      if (label == null && checkBox != null) {
         checkBox.setText(text);
      }
   }

   /**
    * Informs this field editor's listener, if it has one, about a change to
    * the value (<code>VALUE</code> property) provided that the old and new
    * values are different.
    * 
    * @param oldValue
    *            the old value
    * @param newValue
    *            the new value
    */
   protected void valueChanged(boolean oldValue, boolean newValue) {
      setPresentsDefaultValue(false);
      if (oldValue != newValue) {
         changed = !changed;
      }
   }

   /*
    * @see FieldEditor.setEnabled
    */
   public void setEnabled(boolean enabled, Composite parent) {
      getChangeControl(parent).setEnabled(enabled);
   }

}
