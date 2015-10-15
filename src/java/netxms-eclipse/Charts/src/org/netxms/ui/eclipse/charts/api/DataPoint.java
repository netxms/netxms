/**
 * 
 */
package org.netxms.ui.eclipse.charts.api;

import java.util.Date;
import org.swtchart.ISeries;

/**
 * Data point information
 */
public class DataPoint
{
   public Date timestamp;
   public double value;
   public ISeries series;
   
   public DataPoint(Date timestamp, double value, ISeries series)
   {
      this.timestamp = timestamp;
      this.value = value;
      this.series = series;
   }
}
