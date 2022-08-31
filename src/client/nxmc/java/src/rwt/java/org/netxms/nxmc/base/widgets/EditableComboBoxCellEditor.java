/**
 * NetXMS - open source network management system
 * Copyright (C) 2022 Raden Solutions
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
package org.netxms.nxmc.base.widgets;

import java.text.MessageFormat;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import org.eclipse.core.runtime.Assert;
import org.eclipse.jface.viewers.CellEditor;
import org.eclipse.jface.viewers.ColumnViewerEditorActivationEvent;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CCombo;
import org.eclipse.swt.events.FocusAdapter;
import org.eclipse.swt.events.FocusEvent;
import org.eclipse.swt.events.KeyAdapter;
import org.eclipse.swt.events.KeyEvent;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;

/**
 * A cell editor that presents a list of items in a combo box.
 */
public class EditableComboBoxCellEditor extends CellEditor
{
   /**
    * The list is dropped down when the activation is done through the mouse
    */
   public static final int DROP_DOWN_ON_MOUSE_ACTIVATION = 1;

   /**
    * The list is dropped down when the activation is done through the keyboard
    */
   public static final int DROP_DOWN_ON_KEY_ACTIVATION = 1 << 1;

   /**
    * The list is dropped down when the activation is done without ui-interaction
    */
   public static final int DROP_DOWN_ON_PROGRAMMATIC_ACTIVATION = 1 << 2;

   /**
    * The list is dropped down when the activation is done by traversing from cell to cell
    */
   public static final int DROP_DOWN_ON_TRAVERSE_ACTIVATION = 1 << 3;

   private int activationStyle = SWT.NONE;

   /**
    * The list of items to present in the combo box.
    */
   private ArrayList<String> items = new ArrayList<String>();

   /**
    * The zero-based index of the selected item.
    */
   String selection;

   /**
    * The custom combo box control.
    */
   CCombo comboBox;

   /**
    * Default ComboBoxCellEditor style
    */
   private static final int defaultStyle = SWT.NONE;

   /**
    * Creates a new cell editor with no control and no st of choices. Initially, the cell editor has no cell validator.
    *
    * @see CellEditor#setStyle
    * @see CellEditor#create
    * @see EditableComboBoxCellEditor#setItems
    * @see CellEditor#dispose
    */
   public EditableComboBoxCellEditor()
   {
      setStyle(defaultStyle);
   }

   /**
    * Creates a new cell editor with a combo containing the given list of choices and parented under the given control. The cell
    * editor value is the zero-based index of the selected item. Initially, the cell editor has no cell validator and the first item
    * in the list is selected.
    *
    * @param parent the parent control
    * @param items the list of strings for the combo box
    */
   public EditableComboBoxCellEditor(Composite parent, String[] items)
   {
      this(parent, items, defaultStyle);
   }

   /**
    * Creates a new cell editor with a combo containing the given list of choices and parented under the given control. The cell
    * editor value is the zero-based index of the selected item. Initially, the cell editor has no cell validator and the first item
    * in the list is selected.
    *
    * @param parent the parent control
    * @param items the list of strings for the combo box
    * @param style the style bits
    */
   public EditableComboBoxCellEditor(Composite parent, String[] items, int style)
   {
      super(parent, style);
      setItems(items);
   }

   /**
    * Returns the list of choices for the combo box
    *
    * @return the list of choices for the combo box
    */
   public List<String> getItems()
   {
      return items;
   }

   /**
    * Sets the list of choices for the combo box
    *
    * @param items the list of choices for the combo box
    */
   public void setItems(String[] items)
   {
      Assert.isNotNull(items);
      Collections.addAll(this.items, items);
      populateComboBoxItems();
   }

   @Override
   protected Control createControl(Composite parent)
   {

      comboBox = new CCombo(parent, getStyle());
      comboBox.setFont(parent.getFont());

      populateComboBoxItems();

      comboBox.addKeyListener(new KeyAdapter() {
         // hook key pressed - see PR 14201
         @Override
         public void keyPressed(KeyEvent e)
         {
            keyReleaseOccured(e);
         }
      });

      comboBox.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetDefaultSelected(SelectionEvent event)
         {
            applyEditorValueAndDeactivate();
         }

         @Override
         public void widgetSelected(SelectionEvent event)
         {
            selection = comboBox.getText();
         }
      });

      comboBox.addTraverseListener(e -> {
         if (e.detail == SWT.TRAVERSE_ESCAPE || e.detail == SWT.TRAVERSE_RETURN)
         {
            e.doit = false;
         }
      });
      // RAP [if] Use CANCEL_KEYS instead of doit = false
      String[] cancelKeys = new String[] { "ESC", "ENTER" //$NON-NLS-1$ //$NON-NLS-2$
      };
      comboBox.setData(RWT.CANCEL_KEYS, cancelKeys);
      // ENDRAP

      comboBox.addFocusListener(new FocusAdapter() {
         @Override
         public void focusLost(FocusEvent e)
         {
            EditableComboBoxCellEditor.this.focusLost();
         }
      });
      return comboBox;
   }

   /**
    * The <code>ComboBoxCellEditor</code> implementation of this <code>CellEditor</code> framework method returns the zero-based
    * index of the current selection.
    *
    * @return the zero-based index of the current selection wrapped as an <code>Integer</code>
    */
   @Override
   protected Object doGetValue()
   {
      return selection;
   }

   @Override
   protected void doSetFocus()
   {
      comboBox.setFocus();
   }

   /**
    * The <code>ComboBoxCellEditor</code> implementation of this <code>CellEditor</code> framework method sets the minimum width of
    * the cell. The minimum width is 10 characters if <code>comboBox</code> is not <code>null</code> or <code>disposed</code> else
    * it is 60 pixels to make sure the arrow button and some text is visible. The list of CCombo will be wide enough to show its
    * longest item.
    */
   @Override
   public LayoutData getLayoutData()
   {
      LayoutData layoutData = super.getLayoutData();
      if ((comboBox == null) || comboBox.isDisposed())
      {
         layoutData.minimumWidth = 60;
      }
      else
      {
         // make the comboBox 10 characters wide
         GC gc = new GC(comboBox);
         layoutData.minimumWidth = (gc.getFontMetrics().getAverageCharWidth() * 10) + 10;
         gc.dispose();
      }
      return layoutData;
   }

   /**
    * The <code>ComboBoxCellEditor</code> implementation of this <code>CellEditor</code> framework method accepts a zero-based index
    * of a selection.
    *
    * @param value the zero-based index of the selection wrapped as an <code>Integer</code>
    */
   @Override
   protected void doSetValue(Object value)
   {
      Assert.isTrue(comboBox != null && (value instanceof String));
      selection = ((String)value);
      int i = 0;
      for(; i < items.size(); i++)
      {
         if (items.get(i).equals(selection))
            break;
      }
      if (i == items.size())
      {
         items.add(selection);
         comboBox.add(selection, i);
      }
      comboBox.select(i);
   }

   /**
    * Updates the list of choices for the combo box for the current control.
    */
   private void populateComboBoxItems()
   {
      if (comboBox != null && items != null)
      {
         comboBox.removeAll();
         for(int i = 0; i < items.size(); i++)
         {
            comboBox.add(items.get(i), i);
         }

         setValueValid(true);
         selection = items.get(0);
      }
   }

   /**
    * Applies the currently selected value and deactivates the cell editor
    */
   void applyEditorValueAndDeactivate()
   {
      // must set the selection before getting value
      selection = comboBox.getText();
      Object newValue = doGetValue();
      markDirty();
      boolean isValid = isCorrect(newValue);
      setValueValid(isValid);

      if (!isValid)
      {
         setErrorMessage(MessageFormat.format(getErrorMessage(), new Object[] { comboBox.getText() }));
      }

      fireApplyEditorValue();
      deactivate();
   }

   /**
    * @see org.eclipse.jface.viewers.CellEditor#focusLost()
    */
   @Override
   protected void focusLost()
   {
      if (isActivated())
      {
         applyEditorValueAndDeactivate();
      }
   }

   /**
    * @see org.eclipse.jface.viewers.CellEditor#keyReleaseOccured(org.eclipse.swt.events.KeyEvent)
    */
   @Override
   protected void keyReleaseOccured(KeyEvent keyEvent)
   {
      if (keyEvent.character == '\u001b')
      { // Escape character
         fireCancelEditor();
         // RAP [if] In case of single editable column next cell editor is not visible
         // } else if (keyEvent.character == '\t') { // tab key
         // applyEditorValueAndDeactivate();
         // RAP [if] With CANCEL_KEYS in place we need to apply the editor value on key listener as
         // defaultSelection event is not fired if ENTER is canceled
      }
      else if (keyEvent.character == '\r')
      { // enter key
         applyEditorValueAndDeactivate();
      }
      // ENDRAP
   }

   /*
    * (non-Javadoc)
    *
    * @see org.eclipse.jface.viewers.CellEditor#activate(org.eclipse.jface.viewers.ColumnViewerEditorActivationEvent)
    */
   public void activate(ColumnViewerEditorActivationEvent activationEvent)
   {
      super.activate(activationEvent);
      if (activationStyle != SWT.NONE)
      {
         boolean dropDown = false;
         if ((activationEvent.eventType == ColumnViewerEditorActivationEvent.MOUSE_CLICK_SELECTION || activationEvent.eventType == ColumnViewerEditorActivationEvent.MOUSE_DOUBLE_CLICK_SELECTION) &&
               (activationStyle & DROP_DOWN_ON_MOUSE_ACTIVATION) != 0)
         {
            dropDown = true;
         }
         else if (activationEvent.eventType == ColumnViewerEditorActivationEvent.KEY_PRESSED && (activationStyle & DROP_DOWN_ON_KEY_ACTIVATION) != 0)
         {
            dropDown = true;
         }
         else if (activationEvent.eventType == ColumnViewerEditorActivationEvent.PROGRAMMATIC && (activationStyle & DROP_DOWN_ON_PROGRAMMATIC_ACTIVATION) != 0)
         {
            dropDown = true;
         }
         else if (activationEvent.eventType == ColumnViewerEditorActivationEvent.TRAVERSAL && (activationStyle & DROP_DOWN_ON_TRAVERSE_ACTIVATION) != 0)
         {
            dropDown = true;
         }

         if (dropDown)
         {
            getControl().getDisplay().asyncExec(new Runnable() {

               public void run()
               {
                  ((CCombo)getControl()).setListVisible(true);
               }

            });

         }
      }
   }

   /**
    * This method allows to control how the combo reacts when activated
    *
    * @param activationStyle the style used
    */
   public void setActivationStyle(int activationStyle)
   {
      this.activationStyle = activationStyle;
   }
}
