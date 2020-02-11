package org.netxms.ui.eclipse.datacollection.widgets;

import java.util.HashSet;
import java.util.Set;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IViewPart;
import org.netxms.client.objects.AgentPolicy;
import org.netxms.ui.eclipse.datacollection.widgets.helpers.PolicyModifyListener;

public abstract class AbstractPolicyEditor extends Composite
{
   private AgentPolicy policy;  
   private Set<PolicyModifyListener> listeners = new HashSet<PolicyModifyListener>();
   private IViewPart viewPart;

   /**
    * Create abstract policy editor
    * 
    * @param parent parent composite
    * @param style control style
    * @param policy policy object
    */
   public AbstractPolicyEditor(Composite parent, int style, AgentPolicy policy, IViewPart viewPart)
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
      for(PolicyModifyListener l : listeners)
         l.modifyParser();
   }
   
   /**
    * Get policy object currently being edited
    * 
    * @return policy object currently being edited
    */
   protected AgentPolicy getPolicy()
   {
      return policy;
   }

   /**
    * Set new policy object to edit
    * 
    * @param policy new policy object to edit
    */
   public void setPolicy(AgentPolicy policy)
   {
      this.policy = policy;   
      updateControlFromPolicy();
   }

   /**
    * Update editor controls from policy
    */
   protected abstract void updateControlFromPolicy();

   /**
    * Get policy object updated from editor content
    * 
    * @return policy object updated from editor content
    */
   public abstract AgentPolicy updatePolicyFromControl();

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
    * @return the viewPart
    */
   public IViewPart getViewPart()
   {
      return viewPart;
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
