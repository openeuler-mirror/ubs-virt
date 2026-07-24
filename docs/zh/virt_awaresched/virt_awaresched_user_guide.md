# 概述

本文档提供了Virt-awaresched（VSched）在常规场景下的用户指南。

VSched包含vas-daemon用户态服务和前端CLI工具两部分。

- 虚拟化感知调度服务进程（vas-daemon）依赖libvirt监控虚拟机事件，对已创建和服务启动后创建的范围绑核虚拟机进行vCPU1:1绑核调度，尽可能将vCPU线程重分配到同一cluster的CPU上，并定时进行cluster间CPU碎片资源整理。vas-daemon服务启动后，采集环境CPU拓扑信息，侦听虚拟机事件和CLI指令，定时整理CPU碎片资源。
- 前端CLI工具（vasctl）支持使用指令查询虚拟机vCPU绑定信息，手动触发重调度，修改配置。vasctl执行时，进行指令解析并完成参数校验，通过校验后，与服务端建链并转发指令，返回执行结果。

## 特性描述

VSched基于CPU cluster拓扑、超线程调度、动态绑核，为范围绑核的虚拟机vCPU线程选择合适的CPU，尽可能地共享L3 cache，减少vcpu分布跨cluster的情况，提高虚拟机vcpu切换时cache命中率，防止vCPU线程在CPU间无意义的漂移导致的性能下降，提高虚拟化线性度。主要包含以下功能：

- CPU分配与碎片整理：VSched基于CPU cluster拓扑在虚拟机绑核范围内为vCPU选择一个合适的CPU，以尽可能地减少虚拟机跨cluster，在虚拟机销毁时回收CPU，并进行基于cluster的碎片整理。
- 静态绑核模式：VSched为vCPU线程选择一个CPU，进行绑核，静态绑核后vCPU线程只能在该核运行，如果设置了SMT参数，VSched不区分CPU是物理核还是超线程。
- 动态绑核模式：VSched为vCPU线程选择一个优选核后，进行动态绑核，当优选核CPU使用率高于阈值时，vCPU线程会迁移到范围绑核内其它性能更优的核上，当优选核CPU使用率低于阈值时，vCPU线程又会回到优选核，而且它考虑了超线程调度，会让vCPU线程尽可能运行在物理核上，DA_UTIL_TASKGROUP控制优选核CPU使用率计算方式，开启时表示基于任务组（taskgroup）的优选核CPU利用率，关闭时表示基于优选核CPU总利用率。动态绑核需要内核支持潮汐亲和特性，并与组调度配合使用。

## 价值优势

针对鲲鹏950处理器，VSched在单虚拟机跨Cluster绑核场景与多虚拟机场景中，均有效提升了性能。

## 关键技术

VSched基于鲲鹏微架构，与硬件TLBi广播优化相结合，实现虚拟化感知调度，提升虚拟化线性度。

## 约束限制

- 不支持主机（host）和客户机（guest）CPU热插拔。
- 不支持虚拟机vCPU配置特殊的绑核关系，一般推荐按NUMA粒度绑核，或默认不设置绑核，或vCPU绑核到同一范围：如8~31。
- VSched设置绑核可能会与cgroup资源、libvirt存在不一致，如VSched为vCPU绑核，cgroup、libvirt可能不感知。
- 静态绑核模式：客户需要根据实际业务指定是否设置SMT选项，开启SMT选项后vCPU选核绑定时不区分该核是空闲物理核还是超线程，在某些场景下可能不能充分利用空闲物理核。
- 动态绑核模式：动态绑核CPU使用率迁移阈值或动态绑核CPU使用率计算方式不合理，在某些场景可能导致性能有所下降。如果虚拟机vCPU使用率一直处于100%而不进入睡眠唤醒流程，此时若对vCPU线程所在核加压到85%以上，因内核调度可能导致vcpu线程不会迁移到其它核。
- 此版本暂不支持虚拟机vCPU超分场景。
- VSched资源占用率：
  - 非频繁重启的长稳运行时CPU平均占用率 <= 1%。
  - 业务运行时，CPU最大占用率<=100%；内存占用率 <= 128M。

# 特性安装

## 部署方案

| 应用场景   | 说明                                                         |
| ---------- | ------------------------------------------------------------ |
| 无特殊场景 | 基于鲲鹏cluster拓扑，对于范围绑核的虚拟机，工具将自动为虚拟机vCPU线程重新分配CPU，以减少跨cluster开销，提升线性度。 |

## 操作系统要求

| 项目     | 版本                          |
| -------- | ----------------------------- |
| 服务器OS | openEuler 24.03 LTS SP2及以上 |

## 构建指导

如果您需要基于源码进行编译构建或进行二次开发，请参考以下文档：

[构建文档](https://gitcode.com/openeuler/ubs-virt/blob/master/virt-awaresched/docs/build_install/BUILD.md)

## 安装指导

如果您需要部署VSched软件包，请参考以下安装文档：

[安装文档](https://gitcode.com/openeuler/ubs-virt/blob/master/virt-awaresched/docs/build_install/INSTALL.md)

# 特性使用

## 配置说明

如果您需要调整VSched的运行参数或调度策略，请参考以下配置说明：

[VSched配置说明](https://gitcode.com/openeuler/ubs-virt/blob/master/virt-awaresched/docs/config/CONFIG.md)

## 常用命令

如果您希望快速上手，了解典型业务场景下的操作示例，请参考以下常用命令：

[典型场景](https://gitcode.com/openeuler/ubs-virt/blob/master/virt-awaresched/docs/example/EXAMPLE.md)

## CLI使用指南

如果您需要进行虚拟机状态查询、手动触发重调度等运维操作，请参考以下CLI使用指南：

[VSched参考指南](https://gitcode.com/openeuler/ubs-virt/blob/master/virt-awaresched/docs/cli/cli_docs_reference.md)

## 日志说明

| 日志路径              | 权限属组      | 说明                     |
| --------------------- | ------------- | ------------------------ |
| /var/log/vas/vasd.log | root:root 644 | 允许使用tail等工具查看。 |
