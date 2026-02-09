# vas 配置说明

> vas-daemon服务启动参数, 编辑`/usr/lib/systemd/system/vas-daemon.service`文件中, 启动命令行

| key                  | 说明                                                 | 参数类型      | 合法范围                                                    | 默认值      |
|:---------------------|:---------------------------------------------------|:----------|:--------------------------------------------------------|:---------|
| smt                  | cpu配分配粒度                                           | bool      | true: 按照超线程分配; false: 按照物理核分配                           | TRUE     |
| sched-policy         | 调度策略                                               | string 枚举 | dynamicAffinity: vcpu动态绑核, 依赖内核开启动态绑核特性; affinity: 静态绑核 | affinity |
| dynamic-util-thresh  | 动态亲和策略的cpu使用率阈值, cpu负载超过阈值, cpu动态绑核                | int       | (0, 100)                                                | 85       |
| skip-cluster-cpumask | 为vcpu分配cpu时将忽略skip-cluster-cpumask对应的cpu所在的Cluster | string    | 支持范围指定, 例如: 0-10; 支持分段指定: 0-1,2,3-4                     | ""       |
| range-affinity       | 重调度的虚拟机, 是否包含非numa范围绑核的虚拟机                         | bool      | true: 允许非numa范围绑核的虚拟机参与调度; false: 仅允许numa范围绑核的虚拟机参与调度   | TRUE     |
