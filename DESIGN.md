# NetXMS Design Guidelines

A guide for designers who want to contribute to the NetXMS network monitoring platform.

## 👋 Welcome

NetXMS is an open-source enterprise-grade network and infrastructure monitoring platform that serves IT professionals, network administrators, and system operators worldwide. Our mission is to provide a powerful, scalable, and privacy-respecting monitoring solution that can handle everything from small office networks to large enterprise infrastructures.

Network monitoring is a critical business function where seconds matter. Great design helps our users quickly identify and respond to network issues, navigate complex network topologies intuitively, configure monitoring efficiently, and make informed decisions based on clear data visualization.

## 🚢 How to Contribute Design

We're trying to keep our design process as lean as possible with a big focus on delivering value for IT professionals and network administrators.

### Our Process

1. **Create a design issue**: Use our [Design Contribution template](https://github.com/netxms/netxms/issues/new?template=design_idea.md) to propose design improvements or report UX issues.
2. **Get community feedback**: We encourage discussions around design ideas before implementation.
3. **Design and iterate**: Work in publicly available Figma files or other design tools. Share your progress early and often.
4. **Implementation review**: Issues will be picked by the team and receive feedback for iterations or be accepted for implementation.
5. **Final design review**: Pull Requests get labeled with `design: review` for final designer feedback on implementation.

### Creating a Design Issue

To contribute a design improvement:

1. Check out open [issues](https://github.com/netxms/netxms/issues) here on GitHub (we label them with `design: 💅 required`)
2. Feel free to open an issue on your own using our [Design Idea template](https://github.com/netxms/netxms/issues/new?template=design_idea.md) if you find something you would like to contribute to the project
3. Use the `design: 💡 idea` label for new suggestions or `design: 💅 required` label if reporting a UX problem
4. Include mockups, wireframes, or Figma files if available
5. Reference specific user scenarios from our target audience

## 🎯 Design Philosophy

NetXMS follows these core design principles:

- **🎯 Clarity over aesthetics**: Information clarity is more important than visual flair
- **⚡ Performance-focused**: Every UI element should load fast and respond instantly  
- **🔧 Function follows form**: Design serves the functional needs of monitoring professionals
- **🌍 Accessible to all**: Usable by people with different abilities and technical backgrounds
- **🎨 Consistent experience**: Unified experience across desktop, web, and mobile interfaces

## 🚨 High Priority Design Needs

Specific design challenges identified by the NetXMS community that require focused improvement:

### Core UX Issues Needing Attention

1. **Application workflow**
   - Better balance between information accessibility and UI cleanliness
   - Improve navigation efficiency while maintaining context

2. **Visual Design System Updates** 
   - Modernize outdated iconography throughout the application
   - Typography improvements for better readability
   - Font rendering issues in network topology maps

3. **Configuration Workflow Complexity**
   - Initial system setup is confusing for new users
   - Advanced configuration requires excessive technical expertise
   - Form design doesn't follow modern UX patterns
   - Validation and error messages need improvement

4. **Network Topology Visualization Polish**
   - Font readability issues
   - Text rendering and typography improvements needed
   - Enhanced auto-layout algorithms for large networks

## 👥 Target Audience

### Primary Users

**🔧 Administrators - "The Configurators"**
- **Role**: Configure monitoring systems, create new metrics, and set up monitoring infrastructure
- **Goals**: Efficient setup workflows, comprehensive configuration options, reliable metric creation
- **Tasks**: 
  - Setting up new monitoring targets (servers, network devices, applications)
  - Creating custom metrics and data collection policies
  - Configuring alerting rules and notification channels
  - Managing user access and permissions
- **Pain Points**: Complex configuration interfaces, unclear setup wizards, lack of validation feedback
- **Environment**: Primarily desktop/laptop, often working during planned maintenance windows
- **Technical Level**: High technical expertise, deep understanding of network and system monitoring

**🚨 Operators - "The Responders"** 
- **Role**: Use dashboards and alerting systems to identify problems and take corrective action
- **Goals**: Rapid problem identification, clear status visibility, efficient incident response
- **Tasks**:
  - Monitoring real-time dashboards for anomalies
  - Responding to alerts and notifications
  - Investigating performance issues and outages
  - Escalating problems to appropriate technical teams
- **Pain Points**: Hard-to-read dashboards, unclear network map layouts, poor information hierarchy, small fonts in high-resolution displays
- **Environment**: 24/7 operations centers (NOCs), multiple monitors, high-pressure situations
- **Technical Level**: Moderate technical skills, process-oriented, focused on quick decision-making

> **Key Design Insight**: Administrators need powerful, comprehensive tools for setup and configuration, while Operators need clear, actionable interfaces for monitoring and response. These are often different people with different skill sets and workflows.


## ✅ Do's

- Think about the specific needs of network administrators and IT professionals
- Consider the high-stress environments where NetXMS is used (NOCs, incident response)
- Focus on information hierarchy and data visualization clarity
- Test your designs with real monitoring scenarios when possible
- Share work-in-progress designs early for feedback
- Consider accessibility (screen readers, color blindness, etc.)
- Get in touch with the team by starting a discussion on [GitHub](https://github.com/netxms/netxms/discussions) or our [Forum](https://www.netxms.org/forum/index.php)
- Check out our [Contributor Guide](CONTRIBUTING.md) and understand our issue-first approach

## ❌ Don't's

- Don't assume developers have design or visual arts background
- Don't prioritize aesthetics over functionality in critical monitoring interfaces
- Don't create designs that slow down critical workflows
- Don't ignore the technical constraints of the monitoring platform
- Don't work in isolation - engage with the community early and often
- Don't feel ashamed if you don't understand something. We're here to help and share the same goals: Creating an amazing monitoring product
- Don't feel blamed. You are not your work. Our critique is never meant towards you!

## 🎓 License

All design contributions (including but not limited to UI designs, mockups, icons, graphics, and documentation) are licensed under the [MIT License](https://opensource.org/licenses/MIT). 

By contributing design work to NetXMS, you agree that your contributions will be licensed under the MIT License, which allows NetXMS to use, modify, and distribute the designs in both open-source and commercial products.

---

*Thank you for helping make network monitoring more user-friendly for IT professionals worldwide! 🙏*