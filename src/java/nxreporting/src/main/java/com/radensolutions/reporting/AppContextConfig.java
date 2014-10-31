package com.radensolutions.reporting;

import com.radensolutions.reporting.dao.DaoContextConfig;
import com.radensolutions.reporting.service.ServerSettings;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.ApplicationContext;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.ComponentScan;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.mail.javamail.JavaMailSender;
import org.springframework.mail.javamail.JavaMailSenderImpl;
import org.springframework.scheduling.quartz.SchedulerFactoryBean;

import javax.annotation.PostConstruct;
import java.io.IOException;
import java.io.InputStream;
import java.util.Properties;

@Configuration
@ComponentScan("com.radensolutions.reporting")
@Import(DaoContextConfig.class)
public class AppContextConfig {
    private static final Logger log = LoggerFactory.getLogger(AppContextConfig.class);
    @Autowired
    private ApplicationContext applicationContext;
    @Autowired
    private ServerSettings settings;

    @PostConstruct
    public void init() {
        log.debug("AppContextConfig initialized.");
    }

    @Bean
    public JavaMailSender javaMailSender() {
        JavaMailSenderImpl sender = new JavaMailSenderImpl();
        sender.setHost(settings.getSmtpServer());
        return sender;
    }

    @Bean
    public SchedulerFactoryBean quartzScheduler() {
        SchedulerFactoryBean schedulerFactory = new SchedulerFactoryBean();

        final Properties mergedProperties = new Properties();
        mergedProperties.putAll(loadProperties("org/quartz/quartz.properties"));

        ServerSettings.DataSourceConfig dataSourceConfig = settings.getDataSourceConfig(ServerSettings.DC_ID_SCHEDULER);
        Properties properties = new Properties();
        properties.put("org.quartz.jobStore.class", "org.quartz.impl.jdbcjobstore.JobStoreTX");
        properties.put("org.quartz.jobStore.driverDelegateClass", dataSourceConfig.getQuartzDriverDelegate());
        properties.put("org.quartz.jobStore.dataSource", "myDS");
        properties.put("org.quartz.dataSource.myDS.driver", dataSourceConfig.getDriver());
        properties.put("org.quartz.dataSource.myDS.URL", dataSourceConfig.getUrl());
        properties.put("org.quartz.dataSource.myDS.user", dataSourceConfig.getUsername());
        properties.put("org.quartz.dataSource.myDS.password", dataSourceConfig.getPassword());
        properties.put("org.quartz.dataSource.myDS.maxConnections", "20");
        properties.put("org.quartz.scheduler.skipUpdateCheck", "true");

        mergedProperties.putAll(properties);

        schedulerFactory.setQuartzProperties(mergedProperties);

        AutowiringSpringBeanJobFactory jobFactory = new AutowiringSpringBeanJobFactory();
        jobFactory.setApplicationContext(applicationContext);
        schedulerFactory.setJobFactory(jobFactory);

        return schedulerFactory;
    }

    private Properties loadProperties(String name) {
        ClassLoader classLoader = getClass().getClassLoader();
        Properties properties = new Properties();
        InputStream stream = null;
        try {
            stream = classLoader.getResourceAsStream(name);
            properties.load(stream);
        } catch (Exception e) {
            log.error("Unable to load " + name, e);
        } finally {
            if (stream != null) {
                try {
                    stream.close();
                } catch (IOException e) {/* ignore */
                }
            }
        }
        return properties;
    }
}
