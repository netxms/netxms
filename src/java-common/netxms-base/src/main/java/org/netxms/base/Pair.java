package org.netxms.base;

/**
 * Class pair
 *
 * @param <U> first element class
 * @param <V> second element class
 */
public class Pair<U, V> 
{   
   /**
    * The first element of this <code>Pair</code>
    */
   private U first;
   
   /**
    * The second element of this <code>Pair</code>
    */
   private V second;
   
   /**
    * Constructs a new <code>Pair</code> with the given values.
    * 
    * @param first  the first element
    * @param second the second element
    */
   public Pair(U first, V second) 
   {
       this.first = first;
       this.second = second;
   }

   /**
    * @return the first
    */
   public U getFirst()
   {
      return first;
   }

   /**
    * @param first the first to set
    */
   public void setFirst(U first)
   {
      this.first = first;
   }

   /**
    * @return the second
    */
   public V getSecond()
   {
      return second;
   }

   /**
    * @param second the second to set
    */
   public void setSecond(V second)
   {
      this.second = second;
   }
}