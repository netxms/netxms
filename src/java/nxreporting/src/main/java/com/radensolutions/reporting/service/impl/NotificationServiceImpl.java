package com.radensolutions.reporting.service.impl;

import com.radensolutions.reporting.domain.Notification;
import com.radensolutions.reporting.service.NotificationService;
import org.hibernate.Query;
import org.hibernate.SessionFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.util.List;
import java.util.UUID;

import static java.util.Collections.checkedList;

@SuppressWarnings("unchecked")
@Service
public class NotificationServiceImpl implements NotificationService {

    @Autowired
    private SessionFactory session;

    @Override
    @Transactional
    public void create(Notification notify) {
        session.getCurrentSession().save(notify);
    }

    @Override
    @Transactional
    public List<Notification> load(UUID jobId) {
        final Query query = session.getCurrentSession().createQuery("from Notification where jobid = ?").setParameter(0, jobId);
        return checkedList(query.list(), Notification.class);
    }

    @Override
    @Transactional
    public void delete(UUID jobId) {
        final Query query = session.getCurrentSession().createQuery("from Notification where jobid = ?").setParameter(0, jobId);
        final List<Notification> list = checkedList(query.list(), Notification.class);
        for (Notification notify : list)
            session.getCurrentSession().delete(notify);
    }
}
