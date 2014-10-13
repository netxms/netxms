package com.radensolutions.reporting.service;

import com.radensolutions.reporting.domain.Notification;

import java.util.List;
import java.util.UUID;

public interface NotificationService {
    void create(Notification notify);

    List<Notification> load(UUID jobId);

    void delete(UUID jobId);
}
