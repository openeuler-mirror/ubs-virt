# ub-device-manager 配置文件说明

## 1. 配置文件位置

RPM 安装后的配置文件为：

```text
/etc/ub_device_manager/ub_device_manager.toml
```

## 2. 默认配置

配置文件使用 TOML 格式，默认内容如下所示：

```toml
title = "UB Device Manager"

[log]
level = "INFO"
max_file_size = "10 MB"
max_file_count = 10
stdout = false

[vm]
max_xml_size = "1 MB"

[h2n]
mode = "server_type"
wait_timeout = 300
poll_interval = 2
probe_timeout = 3
healthy_stable_duration = 10
sysfs_root = "/sys/bus/ub/devices"
```

## 3. 配置项说明

| 配置项 | 默认值 | 说明                                                                            |
| --- | --- |-------------------------------------------------------------------------------|
| `title` | `"UB Device Manager"` | 配置文件标题。                                                                        |
| `log.level` | `"INFO"` | 日志级别，支持 `DEBUG`、`INFO`、`ERROR`。                                                |
| `log.max_file_size` | `"10 MB"` | 单个日志文件的轮转阈值，日志文件内容超过该值会进行打包。                                                   |
| `log.max_file_count` | `10` | 轮转日志的最大保留数量。                                                                   |
| `log.stdout` | `false` | 是否同时输出日志到标准输出。 |
| `vm.max_xml_size` | `"1 MB"` | 创建虚拟机时允许输入的基础 XML 最大字节数，超过限制时拒绝该 XML。                                         |
| `h2n.mode` | `"server_type"` | H2N 检测策略，三种模式的适用条件见下文。 |
| `h2n.wait_timeout` | `300` | 等待首次健康的最长时间，单位秒，取值范围 1～3600。 |
| `h2n.poll_interval` | `2` | 相邻探测间隔，单位秒，取值范围 0.1～60。 |
| `h2n.probe_timeout` | `3` | 单次探测及 `server_type` 产品类型查询超时，单位秒，取值范围 0.1～60。 |
| `h2n.healthy_stable_duration` | `10` | 首次健康后要求持续健康的时长，单位秒，取值范围 0～300，且不得大于 `wait_timeout`。 |
| `h2n.sysfs_root` | `"/sys/bus/ub/devices"` | 读取设备 `guid` 和 `fault_mgr_state` 的根目录；测试时可指向模拟目录。 |

> [!NOTE] 说明
> 日志文件固定为 `/var/log/ub_device_manager/ub_device_manager.log`。

H2N 策略仅在本次创建选中 NIC VFE 时生效；普通虚机和仅 NPU 直通虚机不检测。需要检测时，先验证 SMBIOS Type 131，再在配置 VF MAC 前检查关联 PFE 链路：

| `h2n.mode` | 适用条件及行为 |
| --- | --- |
| `server_type`（默认） | 查询本机 UBSE 产品类型；`SERVER` 执行检测，`POD_16_1825` 和 `POD_32_1825` 跳过。查询失败、超时或返回未知值时创建失败。适用于仅服务器形态需要检测的部署。 |
| `all` | 不查询产品类型，选中 NIC VFE 时均检测。适用于所有相关部署都需要检测，或测试环境强制验证检测流程。 |
| `disabled` | 跳过检测，但仍会配置 NIC/VF MAC。仅在确认无需 H2N 链路检查时使用。 |

检测要求每个关联 PFE 的 `fault_mgr_state` 为 `PF/IDLE` 或 `PPF/IDLE`，并保持连续健康达到 `healthy_stable_duration`；未通过则不配置 VF MAC、不启动虚机。H2N 配置项在服务启动时校验，取值无效会阻止启动。

## 4. 修改配置并生效

修改配置文件后，等待正在执行的操作结束，再重启服务使修改生效：

```bash
systemctl restart ub-device-manager
systemctl status ub-device-manager --no-pager
```

安装与启动步骤见[安装部署指导](./install.md)。
