## 1 介绍

UBS VirtAwareSched CLI（vasctl）是提供给用户的命令行工具，以命令、操作指令、操作类型、选项和参数的形式向UBS Virt-awaresched发送请求，并回显操作或查询结果。

> 须知
>
> vasctl和vas_daemon仅支持root用户。



## 2 命令入口简介

### 2.1 命令功能

vasctl为VirtAwareSched工具的命令行界面，用于向vas_daemon进程发送用户命令。

### 2.2 命令格式及参数说明

**命令格式**

```shell
vasctl [-h | --help] COMMAND TYPE [-h | --help][OPTIONS]
```

**参数与参数说明**

| 参数    | 参数说明                                                     |
| ------- | ------------------------------------------------------------ |
| COMMAND | 指定操作的指令动作。set config：配置设置query affinity：查询亲和性opt reassign：优化重新分配opt recover：恢复虚拟机亲和性配置 |
| TYPE    | 指定操作的数据或对象。sched-policy：调度策略配置scope：范围  |

若已安装CLI工具，可通过-h/--help参数查看当前CLI工具支持项。

- 全量查询

  ```shell
  vasctl [-h/--help]
  ```

  vasctl打印信息如下所示：

  ```shell
  Usage: set config[OPTIONS]
  OPTIONS:
      -sp  ,--sched-policy                      Set scheduling policy, with valid values including dynamic (dynamic affinity) and bind (static core binding).
  
  Usage: query affinity[OPTIONS]
  OPTIONS:
      -s   ,--scope                             Query virtual machine vCPU affinity: 'all' to retrieve affinity information for all virtual machines, 'uuid' to fetch 
                                                affinity details for a specific virtual machine.
  
  Usage: opt reassign[OPTIONS]
  OPTIONS:
      -s   ,--scope                             Realign virtual machine vCPU: 'all' to reschedule all virtual machines, 'uuid' to reconfigure a specific virtual 
                                                machine's CPU affinity.
  
  Usage: opt recover[OPTIONS]
  OPTIONS:
       The command does not support any option arguments.
  ```

- 单命令查询

  ```shell
  vasctl COMMAND TYPE -h
  ```

  > 说明
  >
  > 单命令查询会返回该命令的相关参数信息。



## 3 CPU调度模块管理

### 3.1 设置调度策略

**命令格式**

```shell
vasctl set config
```

**参数说明**

| 参数               | 参数选项                 | 说明                                                         |
| ------------------ | ------------------------ | ------------------------------------------------------------ |
| -sp/--sched-policy | dynamicAffinity/affinity | 设置调度策略。<br />dynamicAffinity：启用动态CPU亲和性。<br />affinity：启用静态CPU核心绑定。 |

**使用说明**

```shell
vasctl set config -sp dynamicAffinity
```

### 3.2 查询虚拟机vCPU亲和性

**命令格式**

```
vasctl query affinity
```

**参数说明**

| 参数       | 参数选项 | 说明                                                         |
| ---------- | -------- | ------------------------------------------------------------ |
| -s/--scope | all/uuid | 获取虚拟机vCPU亲和性信息。<br />all：获取所有虚拟机的亲和性信息。<br />uuid：获取特定虚拟机的详细亲和性信息。 |

**使用示例**

```shell
vasctl query affinity -s all
```

返回值：

```json
{
	"a1d11347-8738-45fb-8944-e3a058f46401": {
		"uuid": a1d11347-8738-45fb-8944-e3a058f46401,
		"tgid": 266956,
		"ioThreadIds": [1,
		3],
		"domainAffinityMap": {
			"1": {
				"numaId": 1,
				"groups": [{
					"id": "a1d11347-8738-45fb-8944-e3a058f46401_1_10_0",
					"clusterId": 10,
					"layerId": 0,
					"start": 0,
					"nrCpus": 1,
					"usedBitmap": 10000000,
					"entityPids": [266990]
				}],
				"vcpuAffinityInfoMap": {
					"3": {
						"vcpu": 3,
						"pid": 266990,
						"cpuIdx": 0
					}
				}
			},
			"0": {
				"numaId": 0,
				"groups": [{
					"id": "a1d11347-8738-45fb-8944-e3a058f46401_0_0_0",
					"clusterId": 0,
					"layerId": 0,
					"start": 0,
					"nrCpus": 3,
					"usedBitmap": 11100000,
					"entityPids": [266987,
					266988,
					266989]
				}],
				"vcpuAffinityInfoMap": {
					"0": {
						"vcpu": 0,
						"pid": 266987,
						"cpuIdx": 0
					},
					"1": {
						"vcpu": 1,
						"pid": 266988,
						"cpuIdx": 1
					},
					"2": {
						"vcpu": 2,
						"pid": 266989,
						"cpuIdx": 2
					}
				}
			}
		}
	}
}
```

### 3.3 虚拟机重新调度

**命令格式**

```shell
vasctl opt reassign
```

**参数说明**

| 参数       | 参数选项 | 使用说明                                                     |
| ---------- | -------- | ------------------------------------------------------------ |
| -s/--scope | all/uuid | 重新调度虚拟机。<br />all：重新调度所有虚拟机。<br />uuid：重新调度指定虚拟机。 |

**使用示例**

```shell
vasctl opt reassign -s all
```

### 3.4 手动恢复虚拟机亲和性配置

**命令格式**

```shell
vasctl opt recover
```

**参数说明**

无

**使用示例**

```shell
vasctl opt recover
```

示例返回：

```shell
Start recover vm affinity setting.
Success to recover vm affinity setting.
```

### 3.5 通用返回信息

| 指令执行结果   | 返回信息                     |
| -------------- | ---------------------------- |
| 指令执行成功   | Success to execute command.  |
| 指令执行失败   | Failed to execute command.   |
| 指令序列化失败 | Failed to serialize command. |