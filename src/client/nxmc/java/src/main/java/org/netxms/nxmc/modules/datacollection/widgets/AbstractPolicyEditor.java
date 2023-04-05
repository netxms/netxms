/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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
package org.netxms.nxmc.modules.datacollection.widgets;

import java.util.HashSet;
import java.util.Set;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.AgentPolicy;
import org.netxms.nxmc.modules.datacollection.views.PolicyEditorView;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.PolicyModifyListener;

/**
 * Abstract agent policy editor.
 */
public abstract class AbstractPolicyEditor extends Composite 
{
   protected AgentPolicy policy;
   protected PolicyEditorView view;

   private Set<PolicyModifyListener> listeners = new HashSet<PolicyModifyListener>();
   private boolean enableModifyListeners = true;

   /**
    * Create abstract policy editor
    * 
    * @param parent parent composite
    * @param style control style
    * @param policy policy object
    */
   public AbstractPolicyEditor(Composite parent, int style, AgentPolicy policy, PolicyEditorView view)
   {
      super(parent, style);
      this.policy = policy;
      this.view = view;
   }

   /**
    * @param modifyListener
    */
   public void addModifyListener(PolicyModifyListener modifyListener)
   {
      listeners.add(modifyListener);
   }

   /**
    * @param modifyListener
    */
   public void removeModifyListener(PolicyModifyListener modifyListener)
   {
      listeners.remove(modifyListener);
   }

   /**
    * Execute all registered modify listeners
    */
   protected void fireModifyListeners()
   {
      if (enableModifyListeners)
         for(PolicyModifyListener l : listeners)
            l.modifyParser();
   }
   
   /**
    * Set new policy object to edit
    * 
    * @param policy new policy object to edit
    */
   public void setPolicy(AgentPolicy policy)
   {
      this.policy = policy;   
      enableModifyListeners = false;
      updateControlFromPolicy();
      enableModifyListeners = true;
   }

   /**
    * Update editor controls from policy
    */
   public abstract void updateControlFromPolicy();

   /**
    * Update policy object with editor content.
    */
   public abstract void updatePolicyFromControl();

   /**
    * Fill local pull down menu with additional actions
    * 
    * @param manager menu manager
    */
   public void fillLocalPullDown(IMenuManager manager)
   {
   }

   /**
    * Fill local tool bar with additional actions
    * 
    * @param manager menu manager
    */
   public void fillLocalToolBar(IToolBarManager manager)
   {
   }

   /**
    * Check if save is allowed by editor. If save is not allowed editor should return string with reason.
    *
    * @return null to allow save operation or string describing the reason for not allowing save operation
    */
   public String isSaveAllowed()
   {
      return null;
   }

   /**
    * Callback that will be called on policy save
    */
   public void onSave()
   {      
   }

   /**
    * Callback that will be called on save discard
    */
   public void onDiscard()
   {
   }
}
