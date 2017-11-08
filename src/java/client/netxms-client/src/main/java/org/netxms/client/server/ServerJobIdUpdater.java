package org.netxms.client.server;

public interface ServerJobIdUpdater
{
   /**
    * Update Server Job Id in Console Job
    * @param id server job id
    */
   public void setJobIdCallback(int id);
}
