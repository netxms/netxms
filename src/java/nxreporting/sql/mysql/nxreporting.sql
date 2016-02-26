DROP TABLE IF EXISTS `report_notification`;
CREATE TABLE `report_notification` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `attach_format_code` int(11) DEFAULT NULL,
  `jobid` char(36) DEFAULT NULL,
  `mail` varchar(255) COLLATE utf8_unicode_ci DEFAULT NULL,
  `report_name` varchar(255) COLLATE utf8_unicode_ci DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

DROP TABLE IF EXISTS `reporting_results`;
CREATE TABLE `reporting_results` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `executionTime` datetime DEFAULT NULL,
  `jobId` char(36) DEFAULT NULL,
  `reportId` char(36) DEFAULT NULL,
  `userId` int(11) DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;
