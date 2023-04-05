package org.netxms.ui.eclipse.datacollection.widgets;

import java.util.HashSet;
import java.util.Set;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.AgentPolicy;
import org.netxms.ui.eclipse.datacollection.views.PolicyEditorView;
import org.netxms.ui.eclipse.datacollection.widgets.helpers.PolicyModifyListener;

public abstract class AbstractPolicyEditor extends Composite
{
   protected PolicyEditorView viewPart;
   protected AgentPolicy policy;

   private Set<PolicyModifyListener> listeners = new HashSet<PolicyModifyListener>();
   private boolean enableModifyListeners = true;

   /**
    * Create abstract policy editor
    * 
    * @param parent parent composite
    * @param style control style
    * @param policy policy object
    */
   public AbstractPolicyEditor(Composite parent, int style, AgentPolicy policy, PolicyEditorView viewPart)
   {
      super(parent, style);
      this.policy = policy;
      this.viewPart = viewPart;
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
    * Update policy object from editor content
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
