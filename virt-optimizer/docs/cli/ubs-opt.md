# 描述
### 启动optimizer采集的服务：
```
ubs-opt start_ebpf
```
说明：执行该命令后，ubs-optimizer会根据配置项，周期性采集虚拟机的性能数据，输出到/var/ubs-opt/data/data.json

### 停止optimizer的服务
```
ubs-opt stop_ebpf
```
说明：执行该命令后，ubs-optimizer将停止采集虚拟机性能数据

# 示例
虚拟机内安装ubs-optimizer的组件完成后，启动ubs-optimizer服务执行以下命令：
```
ubs-opt start_ebpf
```
查看/var/ubs-opt/data/data.json：
![img.png](../images/image-20253108.png)

采集完成后，ubs-optimizer服务执行以下命令：
```
ubs-opt stop_ebpf
```