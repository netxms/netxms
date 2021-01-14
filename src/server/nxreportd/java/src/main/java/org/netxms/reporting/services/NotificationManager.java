package org.netxms.reporting.services;

import java.util.List;
import java.util.UUID;
import org.hibernate.Session;
import org.hibernate.Transaction;
import org.hibernate.query.Query;
import org.netxms.reporting.Server;
import org.netxms.reporting.model.Notification;

/**
 * Notification manager
 */
public class NotificationManager
{
   private Server server;

   /**
    * Create new notification manager
    *
    * @param server owning server
    */
   public NotificationManager(Server server)
   {
      this.server = server;
   }

   /**
    * Save notification object
    *
    * @param notification notification object
    */
   public void save(Notification notification)
   {
      Session session = server.getSessionFactory().getCurrentSession();
      Transaction txn = session.beginTransaction();
      try
      {
         session.save(notification);
         txn.commit();
      }
      catch(Exception e)
      {
         txn.rollback();
         throw e;
      }
   }

   /**
    * Load all notifications for given job ID.
    *
    * @param jobId job ID
    * @return notifications for given job
    */
   public List<Notification> load(UUID jobId)
   {
      Session session = server.getSessionFactory().getCurrentSession();
      Transaction txn = session.beginTransaction();
      try
      {
         final Query<Notification> query = session.createQuery("FROM Notification WHERE jobId = :jobId", Notification.class).setParameter("jobId", jobId);
         return query.getResultList();
      }
      finally
      {
         txn.commit();
      }
   }

   /**
    * Delete notifications for given job ID.
    * 
    * @param jobId job ID
    */
   public void delete(UUID jobId)
   {
      Session session = server.getSessionFactory().getCurrentSession();
      Transaction txn = session.beginTransaction();
      try
      {
         final Query<Notification> query = session.createQuery("FROM Notification WHERE jobId = :jobId", Notification.class).setParameter("jobId", jobId);
         for(Notification n : query.getResultList())
         {
            session.delete(n);
         }
         txn.commit();
      }
      catch(Exception e)
      {
         txn.rollback();
         throw e;
      }
   }
}
