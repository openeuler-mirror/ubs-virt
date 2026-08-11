# UBS Virt-awaresched User Guide

<!-- md-trans-meta sourceCommit=1e04231b9d30f54335a401ccd90b1d03de35d856 translatedAt=2026-08-10T02:48:00.821Z pushedAt=2026-08-10T08:14:19.417Z -->

## Overview

This document describes how to use Virt-awaresched (VSched) in common scenarios.

VSched consists of a user-space service vas-daemon and a frontend command-line interface (CLI) tool.

- vas-daemon depends on libvirt to monitor virtual machine (VM) events. It binds each vCPU of VMs created before and after vas-daemon startup to a core within the core binding range of the VMs, and reallocates vCPUs threads to CPU cores in the same cluster as much as possible. In addition, it periodically performs inter-cluster CPU resource defragmentation. After being started, vas-daemon collects the CPU topology information in the environment, listens on VM events and CLI commands, and periodically defragments CPU resources.

- The frontend CLI tool vasctl allows you to query vCPU binding information, manually trigger rescheduling, and modify configurations. During the execution of a command, it parses the command and verifies parameters. Upon successful verification, it sets up connections to the server and forwards the command. Then, it returns the execution result.

### Feature Description

VSched selects proper CPU cores for vCPU threads with the core binding range based on the CPU cluster topology, hyper-threading scheduling, and dynamic core binding, to maximize L3 cache sharing and reduce the cross-cluster distribution of vCPUs. This increases the cache hit rate during vCPU switchover, prevents performance deterioration caused by vCPU thread drift between CPU cores, and improves virtualization linearity. It provides the following functions:

**CPU Core Allocation and Defragmentation**

VSched selects proper CPU cores for vCPUs within the core binding range based on the CPU cluster topology, to minimize cross-cluster VMs. Upon VM destruction, it reclaims the CPU cores and performs cluster-level defragmentation.

**Static Core Binding**

VSched statically binds a vCPU thread to a CPU core so that the thread runs exclusively on this core. If the simultaneous multi-threading (SMT) is enabled, VSched does not differentiate between physical cores or hyper-threads.

**Dynamic Core Binding**

* VSched dynamically binds a vCPU thread to a preferred core.

  * When the core usage is higher than a threshold, the vCPU thread is migrated to another core with better performance within the core binding range.

  * When the core usage falls below the threshold, the vCPU thread reverts to the preferred core.

* Dynamic core binding takes hyper-threading scheduling into account, and allows vCPU threads to run on physical cores whenever possible.

* `DA_UTIL_TASKGROUP` controls how the preferred core usage is calculated.

  * When it is enabled, preferred core usage by task group is used.

  * When it is disabled, the total preferred core usage is used.

* Dependencies:

  * The kernel must support the tidal affinity feature.

  * Dynamic core binding must work together with group scheduling.

### Benefits

For the Kunpeng 950 processor, VSched effectively improves performance in both single-VM cross-cluster core binding and multi-VM scenarios.

### Key Technologies

VSched is built on the Kunpeng microarchitecture and combined with hardware Translation Lookaside Buffer Invalidate (TLBi) broadcast optimization to implement virtualization-aware scheduling and improve virtualization linearity.

### Constraints

- Host and guest CPUs are not hot-swappable.

- Special core binding relationships cannot be configured for VM vCPUs. You are advised to bind vCPUs to cores by NUMA node, leave core binding unconfigured (default), or bind vCPUs to cores in the same range (for example, cores 8 to 31).

- cgroup and libvirt may not be aware of VSched's vCPU-core binding settings, leading to inconsistent configurations.

- Static core binding: Customers need to specify whether to enable the SMT option based on actual services. If the SMT option is enabled, VSched does not differentiate between physical cores and hyper-threads during vCPU-core binding. In this case, idle physical cores may not be fully utilized in some scenarios.

- Dynamic core binding: If the CPU core usage threshold for triggering migration or the CPU core usage calculation method is inappropriate, performance may deteriorate in some scenarios. When the vCPU usage of a VM stays at 100% and the vCPU does not enter the sleep/wakeup process, the kernel scheduler may refrain from migrating the vCPU thread to a different core, even if the usage of the core where it resides exceeds 85%.

- vCPU overcommitment is not supported for VMs in the current version.

- VSched resource usage:

  - During stable long-term operation without frequent restarts, the average CPU usage is less than or equal to 1%.

  - During service operation, the maximum CPU usage is less than or equal to 100%, and the memory usage is less than or equal to 128 MB.

## Feature Installation

### Solution Deployment

| Application Scenario | Description |
| --------------------- | ----------- |
| All | Based on the Kunpeng cluster topology, the tool automatically reallocates CPU cores to vCPU threads within the core binding range of VMs to reduce cross-cluster overhead and improve linearity. |

### OS Requirements

| Item     | Version                          |
| -------- | ----------------------------- |
| Server OS | openEuler 24.03 LTS SP2 or later |

### Build Guide

If you need to perform compilation and build using source code or conduct secondary development, refer to the following document.

[Build Guide](https://gitcode.com/openeuler/ubs-virt/blob/master/virt-awaresched/docs/build_install/BUILD.md)

### Installation Guide

If you need to deploy the VSched software package, refer to the following document.

[Installation Guide](https://gitcode.com/openeuler/ubs-virt/blob/master/virt-awaresched/docs/build_install/INSTALL.md)

## Feature Usage

### Configuration Description

If you need to adjust VSched's operating parameters or scheduling policies, refer to the following configuration description.

[VSched Configuration Description](https://gitcode.com/openeuler/ubs-virt/blob/master/virt-awaresched/docs/config/CONFIG.md)

### Common Commands

If you want to get started quickly and learn about operation examples in typical service scenarios, refer to the common commands in the following document.

[Typical Scenarios](https://gitcode.com/openeuler/ubs-virt/blob/master/virt-awaresched/docs/example/EXAMPLE.md)

### CLI Usage Guide

If you need to perform O&M such as VM status query or manual rescheduling triggering, refer to the following CLI usage guide.

[VSched Reference Guide](https://gitcode.com/openeuler/ubs-virt/blob/master/virt-awaresched/docs/cli/cli_docs_reference.md)

### Log Description

| Log Path              | Owner/Group/Permission | Description                              |
| --------------------- | -------------------------- | ---------------------------------------- |
| /var/log/vas/vasd.log | root:root 644              | You are allowed to use tools such as tail to view the log. |
