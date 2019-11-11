package org.netxms.ui.eclipse.datacollection.widgets;

import java.util.HashSet;
import java.util.Set;
import org.eclipse.jface.text.IFindReplaceTarget;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.texteditor.FindReplaceAction;
import org.netxms.client.objects.AgentPolicy;
import org.netxms.ui.eclipse.datacollection.widgets.helpers.PolicyModifyListener;

public abstract class AbstractPolicyEditor extends Composite implements IFindReplaceTarget
{
   private AgentPolicy policy;  
   private Set<PolicyModifyListener> listeners = new HashSet<PolicyModifyListener>();
   private FindReplaceAction actionFindReplace = null;

   /**
    * Create abstract policy editor
    * 
    * @param parent parent composite
    * @param style control style
    * @param policy policy object
    */
   public AbstractPolicyEditor(Composite parent, int style, AgentPolicy policy)
   {
      super(parent, style);
      this.policy = policy;
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
    * Create actions for editor
    */
   protected void createActions()
   {
   }
   
   /**
    * Check if Find/Replace action is required for this editor
    * 
    * @return true if Find/Replace action is required for this editor
    */
   public abstract boolean isFindAndReplaceRequired();

   /**
    * Set Find/Replace action for this editor
    * 
    * @param actionFindReplace
    */
   public void setFindAndReplaceAction(FindReplaceAction actionFindReplace)
   {
      this.actionFindReplace = actionFindReplace;
   }

   /**
    * @return the actionFindReplace
    */
   protected FindReplaceAction getFindAndReplaceAction()
   {
      return actionFindReplace;
   }
   
   /**
    * Call update() on Find/Replace action 
    */
   protected void updateFindAndReplaceAction()
   {
      if (actionFindReplace != null)
         actionFindReplace.update();
   }
}
