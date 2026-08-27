# 版本说明

## 修改记录

|文档版本|发布日期|修改说明|
|---|---|---|
|01|2026-06-30|第一次正式发布|
|02|2026-06-30|第二次正式发布|

## 版本配套说明

### 产品版本信息

|产品名称|版本|
|--|--|
|UBS Virt-awaresched|master|
|UBS Virt-optimizer|master|

### 软件版本配套说明

**UBS Virt-awaresched**

|软件名称|软件版本|
|--|--|
|libboundscheck|1.1.1|
|qemu|8.2.0|
|libvirt|9.1.0|
|OS| openEuler 24.03 LTS SP2及以上版本|
|CMake|3.22及以上版本|
|GCC|9.3及以上版本|
**UBS Virt-optimizer**

|软件名称|软件版本|
|:-------------------|:-------------------|
| git                | 2.33.0 及以上版本  |
| cmake              | 3.22.0 及以上版本  |
| make               | 4.3 及以上版本     |
| gcc                | 10.3.1 及以上版本  |
| g++                | 10.3.1 及以上版本  |
| rapidjson-devel    | 1.1.0 及以上版本   |
| bpftool            | 7.2.0 及以上版本   |
| libbpf             | 0.8.1 及以上版本   |
| libbpf-devel       | 0.8.1 及以上版本   |
| clang              | 12.0.1 及以上版本  |
| tar                | 1.34 及以上版本    |
| rpm-build          | 4.18.2 及以上版本  |
|OS| openEuler 24.03 LTS SP4|

### 硬件版本配套说明

**UBS Virt-awaresched**

|部件名称|硬件要求|
|--|--|
|架构|AArch64|

**UBS Virt-optimizer**

|部件名称|硬件要求|
|--|--|
|架构|AArch64、x86_64|

## UBS Virt-awaresched
 
### 更新说明

无

### 已解决问题

无

### 遗留问题

无

## UBS Virt-optimizer
 
### 更新说明

无

### 已解决问题

无

### 遗留问题

无

## 版本配套文档

**UBS Virt-awaresched**

|文档名称|内容简介|
|----|---|
|《[UBS VirtAwareSched CLI参考指南](../docs/zh/virt_awaresched/virt_awaresched_cli_userguide.md)》|本文档描述了UBS VirtAwareSched CLI命令行工具的使用方法，包含命令格式、参数说明及CPU调度管理等核心功能。|
|《[UBS VirtAwareSched配置说明](../docs/zh/virt_awaresched/virt_awaresched_configuration_instructions.md)》|本文档包含smt、sched-policy等启动参数，说明类型、范围及默认值。|
|《[UBS VirtAwareSched安装指导书](../docs/zh/virt_awaresched/virt_awaresched_installation.md)》|本文档包括环境要求、离线安装、服务状态确认、启动参数修改及动态绑核模式准备。|
|《[UBS VirtAwareSched用户指南](../docs/zh/virt_awaresched/virt_awaresched_user_guide.md)》|本文档提供了Virt-awaresched在常规场景下的用户指南，包括概述、特性安装、特性使用。|

**UBS Virt-optimizer**

|文档名称|内容简介|
|----|---|
|《[UBS virt-optimizer命令行使用指南](../docs/zh/virt_optimizer/virt_optimizer_cli_userguide.md)》|本文档描述了UBS virt-optimizer的命令行使用指南，包含优化器启动及采集服务的启动与停止操作步骤。|
|《[UBS virt-optimizer配置说明](../docs/zh/virt_optimizer/virt_optimizer_configuration_instructions.md)》|本文档描述了UBS virt-optimizer的配置说明，包含配置原则、文件路径、示例及注意事项。|
|《[UBS virt-optimizer安装指南](../docs/zh/virt_optimizer/virt_optimizer_installation.md)》|本文档描述了UBS virt-optimizer的安装指南，包括部署说明、环境要求、软件安装、进程管理及卸载等内容。|
|《[UBS virt-optimizer安全声明](../docs/zh/virt_optimizer/virt_optimizer_security_description.md)》|本文档描述了离线性能调优工具的安全声明，强调仅限开发使用，需及时关闭以规避安全风险。|
|《[UBS virt-optimizer用户指南](../docs/zh/virt_optimizer/virt_optimizer_userguide.md)》|本文档介绍了ubs-optimizer的核心功能、适用场景、代码示例及常见问题。|
