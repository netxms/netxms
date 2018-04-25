package org.netxms.ui.eclipse.tools;

public class SplitedString
{
   private String result;
   private boolean fit;
   
   SplitedString(String result, boolean fit)
   {
      this.setResult(result);
      this.setFit(fit);
   }

   /**
    * @return the result
    */
   public String getResult()
   {
      return result;
   }

   /**
    * @param result the result to set
    */
   public void setResult(String result)
   {
      this.result = result;
   }

   /**
    * @return the fit
    */
   public boolean fits()
   {
      return fit;
   }

   /**
    * @param fit the fit to set
    */
   public void setFit(boolean fit)
   {
      this.fit = fit;
   }

}
