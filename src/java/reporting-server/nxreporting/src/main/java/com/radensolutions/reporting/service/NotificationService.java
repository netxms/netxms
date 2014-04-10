package com.radensolutions.reporting.service;

import java.util.List;
import java.util.UUID;

import com.radensolutions.reporting.domain.Notification;

public interface NotificationService
{
	void create(Notification notify);
	
	List<Notification> load(UUID jobId);
	
	void delete(UUID jobId);
}
