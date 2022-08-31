/**
 * NetXMS - open source network management system
 * Copyright (C) 2020-2022 Raden Solutions
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
package org.netxms.nxmc.modules.datacollection.dialogs.helpers;

import org.eclipse.swt.events.VerifyListener;
import org.netxms.client.datacollection.BulkDciUpdateElement;

/**
 * Information about changed field for buld DCI updates
 */
public class BulkDciUpdateElementUI extends BulkDciUpdateElement
{
   private String name;
   private String[] possibleValues;
   private VerifyListener verifyListener;
   private EditModeSelector callback;
   private boolean editableDropdown = false;

   /**
    * Constructor for bulk update element 
    * 
    * @param name field name
    * @param fieldId filed id in NXCPMessage
    * @param possibleValues possible values for drop-down
    */
   public BulkDciUpdateElementUI(String name, long fieldId, String[] possibleValues)
   {
      super(fieldId);
      this.name = name;
      this.possibleValues = possibleValues;
      this.verifyListener = null;
      this.callback = null;
   }
   
   /**
    * Constructor for bulk update element 
    * 
    * @param name field name
    * @param fieldId filed id in NXCPMessage
    * @param possibleValues possible values for drop-down
    * @param callback callback that returns if field is editable (field is always editable if not provided)
    */
   public BulkDciUpdateElementUI(String name, long fieldId, String[] possibleValues, EditModeSelector callback)
   {
      super(fieldId);
      this.name = name;
      this.possibleValues = possibleValues;
      this.verifyListener = null;
      this.callback = callback;
   }
   
   /**
    * Constructor for bulk update element 
    * 
    * @param name field name
    * @param fieldId filed id in NXCPMessage
    * @param possibleValues possible values for drop-down
    * @param verifyListener field validation listener (should be used for number only fields)
    */
   public BulkDciUpdateElementUI(String name, long fieldId, String[] possibleValues, VerifyListener verifyListener)
   {
      super(fieldId);
      this.name = name;
      this.possibleValues = possibleValues;
      this.verifyListener = verifyListener;
      callback = null;
   }

   /**
    * @return the value
    */
   public String getTextValue()
   {
      if (value != null && value instanceof String)
         return (String)value;
      else
         return editableDropdown ? possibleValues[0] : "";
   }

   /**
    * @return the value
    */
   public Integer getSelectionValue()
   {
      if (value != null && value instanceof Integer)
         return (Integer)value;
      else
         return -1;
   }

   /**
    * @return the name
    */
   public String getName()
   {
      return name;
   }
   /**
    * @return the possibleValues
    */
   public String[] getPossibleValues()
   {
      return possibleValues;
   }
   
   /**
    * If editable value is text
    * 
    * @return true if editable value is test value
    */
   public boolean isText()
   {
      return possibleValues == null;
   }  

   /**
    * Verify listener
    * 
    * @return verify listener for this value
    */
   public VerifyListener getVerifyListener()
   {
      return verifyListener;
   }   
   
   /**
    * Returns true if field is editable
    * 
    * @return true if field is editable
    */
   public boolean isEditable()
   {
      if (callback != null)
         return callback.isEditable();
      return true;
   }

   /**
    * Callback interface used for choosing if field is editable or not
    */
   public static interface EditModeSelector
   {
      public boolean isEditable();
   }
   

   /** 
    * Set if drop down should be read only. By default TRUE.
    * 
    * @param isReadOnly if drop down should be read only. 
    */
   public void setEditableDropdown(boolean editableDropdown)
   {
      this.editableDropdown = editableDropdown;
   }

   /** 
    * @return if element drop down is read only
    */
   public boolean isEditableDropdown()
   {
      return editableDropdown;
   }
   
   /**
    * Set value from UI
    *
    * @param value new field value
    */
   public void setValue(Object value)
   {
      if (isText())
         this.value = ((String)value).isEmpty() ? null : value;
      else if (editableDropdown)
         this.value = possibleValues[0].equals(value) ? null : value; //do not save first element: No change
      else
         this.value = ((Integer)value) - 1;
   }
   
   /**
    * Get value to show on UI
    * 
    * @return value
    */
   public Object getValue()
   {
      if (isText() || editableDropdown)
      {
         return getTextValue();
      }
      else
      {
         return getSelectionValue() + 1;         
      }      
   }
   
   /**
    * Get value as a text to display
    * 
    * @return text to display
    */
   public String getDisplayText()
   {
      if (editableDropdown)
         return getTextValue();
      if (isText())
         return getTextValue().isEmpty() ? "No change" : getTextValue();
      return possibleValues[getSelectionValue() + 1];
   }
}
