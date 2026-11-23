# ub-device-manager

## 项目介绍

ub-device-manager 是 ubs-virt 项目中的 UB 设备管理组件，面向 NPU 直通虚拟机场景，提供设备查询、绑定与解绑，以及虚拟机创建与删除能力，并支持配套 Hi1825 智能网卡的 UB 直通。

用户提供基础虚拟机 XML，并指定 NPU 设备 ID 或所需数量。ub-device-manager 通过 UBS Engine（UBSE）完成设备资源分配与绑定，自动补充 UB 直通配置，再通过 libvirt 定义并启动虚拟机。

## 主要功能

| 功能 | 说明                                                                                                   |
| --- |------------------------------------------------------------------------------------------------------|
| NPU 设备管理 | 查询 NPU 设备信息和绑定状态，按设备 ID 或数量选择资源，完成绑定与解绑。                                                             |
| 虚拟机管理 | 基于基础 XML 创建普通虚拟机或 NPU 直通虚拟机，并支持虚拟机删除与相关设备解绑。                                                         |
| 内存与 NPU 亲和选择 | 创建 NPU 直通虚拟机时，可按内存所在 NUMA 或 Socket 选择 NPU，支持三种亲和性策略，详见 [CLI 使用指导](./docs/cli_userguide.md#可选内存亲和策略)。 |
| 配套智能网卡直通 | 根据 NPU 与智能网卡的关联关系，为虚拟机配置配套的 Hi1825 智能网卡 VFE。                                                         |
| 多种调用方式 | 提供 `ubdmctl` 命令行工具、Python SDK 和基于本地 Unix Domain Socket 的 HTTP API。                                   |

## H2N 检测配置

RPM 安装后在 `/etc/ub_device_manager/ub_device_manager.toml` 的 `[h2n]` 中配置。H2N 检测只针对本次创建选中了 NIC VFE 的直通虚机；普通虚机或仅 NPU 直通虚机，无论使用哪种模式都不会执行 H2N 检测。需要检测时，在配置 VF MAC 前等待关联 PFE 链路持续健康。

| `h2n.mode` | 适用条件 |
| --- | --- |
| `server_type`（默认） | 仅当本机 UBSE 产品类型为 `SERVER` 时检测；`POD_16_1825`、`POD_32_1825` 跳过。产品类型查询失败、超时或返回未知值时，创建失败，不会按 POD 跳过。适用于只要求服务器形态检查 H2N 的部署。 |
| `all` | 选中 NIC VFE 时一律检测，不查询产品类型。适用于所有相关部署都要求检查，或需要强制验证检测流程的测试环境。 |
| `disabled` | 跳过 H2N 检测，但不跳过后续 NIC/VF MAC 配置。仅在确认无需此项链路检查时使用。 |

其他参数的默认值是：`wait_timeout = 300`（等待首次健康的最长秒数）、`poll_interval = 2`（探测间隔）、`probe_timeout = 3`（单次探测及产品类型查询的超时）、`healthy_stable_duration = 10`（连续健康时长），单位均为秒；`sysfs_root = "/sys/bus/ub/devices"` 指定探测目录。修改配置后需重启服务。完整配置及取值范围见[配置文件说明](./docs/configuration.md)。

## 开始使用

首次使用请阅读[快速开始](./docs/quick_start.md)，按照文档完成构建、安装、环境准备和第一台 NPU 直通虚拟机的创建。

## 文档导航

| 文档 | 内容 |
| --- | --- |
| [快速开始](./docs/quick_start.md) | 从源码构建到创建、验证和删除第一台 NPU 直通虚拟机的完整流程。 |
| [构建指导](./docs/build.md) | 构建环境、依赖、RPM 构建方法和产物获取。 |
| [安装部署指导](./docs/install.md) | 依赖安装、访问权限配置、服务启动，以及升级和卸载。 |
| [配置文件说明](./docs/configuration.md) | 配置文件位置、默认配置、配置项含义和生效方法。 |
| [普通虚拟机创建指导](./docs/vm_basics.md) | 基础 XML、镜像、网络、普通虚拟机创建，以及大页和 NUMA 配置。 |
| [NPU 直通虚拟机 XML 配置指南](./docs/npu_passthrough_xml.md) | UB 直通设备与 SMBIOS Type 131 的 XML 配置，以及手动创建、验证和清理步骤。 |
| [CLI 使用指导](./docs/cli_userguide.md) | `ubdmctl` 命令、参数和使用示例。 |
| [HTTP API 使用指导](./docs/http_userguide.md) | HTTP 接口的调用方式、参数和示例。 |
| [Python SDK 使用指导](./docs/sdk_userguide.md) | Python SDK 的使用方法和调用示例。 |
| [设计说明](./docs/design.md) | 整体架构、HTTP/SDK/CLI 实现、UBSE 调用、XML 转换及创建与删除流程。 |
| [测试指导](./docs/test.md) | 测试环境准备和测试执行方法。 |
