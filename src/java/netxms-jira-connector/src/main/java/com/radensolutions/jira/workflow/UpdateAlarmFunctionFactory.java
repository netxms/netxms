package com.radensolutions.jira.workflow;

import com.atlassian.core.util.map.EasyMap;
import com.atlassian.jira.plugin.workflow.AbstractWorkflowPluginFactory;
import com.atlassian.jira.plugin.workflow.WorkflowPluginFunctionFactory;
import com.opensymphony.workflow.loader.AbstractDescriptor;
import com.opensymphony.workflow.loader.FunctionDescriptor;

import java.util.Map;

public class UpdateAlarmFunctionFactory extends AbstractWorkflowPluginFactory implements WorkflowPluginFunctionFactory {

    public static final String KEY = "selectedAction";

    @Override
    protected void getVelocityParamsForInput(Map<String, Object> velocityParams) {
    }

    @Override
    protected void getVelocityParamsForEdit(Map<String, Object> velocityParams, AbstractDescriptor abstractDescriptor) {
        getVelocityParamsForInput(velocityParams);
        getVelocityParamsForView(velocityParams, abstractDescriptor);
    }

    @Override
    protected void getVelocityParamsForView(Map<String, Object> velocityParams, AbstractDescriptor abstractDescriptor) {
        FunctionDescriptor descriptor = (FunctionDescriptor) abstractDescriptor;
        String value = (String) descriptor.getArgs().get(KEY);
        if (value == null) {
            value = "";
        }
        velocityParams.put(KEY, value);
    }

    @Override
    public Map<String, ?> getDescriptorParams(Map<String, Object> conditionParams) {
        String value = extractSingleParam(conditionParams, KEY);
        return EasyMap.build(KEY, value);
    }
}
