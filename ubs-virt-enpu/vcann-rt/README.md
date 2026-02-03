# vCANN-RT

## 介绍

`vCANN-RT(virtual CANN Runtime)`是ubs-virt-enpu提供NPU算力软切分的方案。本方案需要依赖Linux系统Preload Hook的能力，预加载软切分动态库，通过拦截runtime API的调用，根据算力和显存资源配额信息，进行算力控制或显存控制。

## 源码下载

可以使用如下方式下载`vCANN-RT`源码。

```shell
$ git clone <ubs-virt-enpu-vcann-rt-url>
```


## 源码目录结构

`vCANN-RT`源码的主要目录结构如下所示：

```shell
.
├── scripts   // 存放项目中编译构建使用的脚本文件
├── src       // 存放项目的功能实现源码，仅该目录参与构建出包
└── test      // 存放项目的ut和dtfuzz等
```

## 编译

`vCANN-RT`在代码仓中提供了统一的编译构建脚本（即`make_build.sh`文件），可以直接执行该脚本文件进行编译构建。默认无需任何配置项，直接执行即可。

```shell
$ bash make_build.sh
```

编译产物存在于rpmbuild目录下。

## 部署

安装

    将编译产物上传到环境上，使用rpm安装。

    ```shell
    $ rpm -ivh --nodeps {vruntime.rpm}
    ```

    更换vCANN-RT版本前，需要卸载现有版本，重新安装：

    ```shell
    $ rpm -ev {vruntime.rpm}
    $ rpm -ivh --nodeps {vruntime.rpm}
    ```

vCANN-RT支持两种方式启动服务：

- 方式一：基于kubernetes编排系统部署

    用户通过k8s yaml文件，在申请容器时声明所需的vNPU算力资源百分比例和显存资源数量。k8s会将容器调度到资源充足的节点，并将算力和显存资源配额等信息以配置文件挂载到容器内。同时，k8s会将vCANN-RT的包挂载到容器，容器内即可使能vCANN-RT软切分能力。

1. 准备已部署业务模型的容器镜像

    在`/usr/bin`目录安装systemd-detect-virt工具，用于检测系统的运行环境是否为虚拟化环境和虚拟化方式。若容器内未安装systemd-detect-virt工具，调用dcmi接口时会出现故障。

2. 配置preload文件

    创建ld.so.preload文件，并将编译获得的vCANN-RT的libvruntime.so文件路径配置到ld.so.preload文件中，用于k8s挂载cCANN-RT到容器。

    ```shell
    $ vi ld.so.preload
    # libvruntime.so 的默认路径为/opt/enpu/vcann-rt/lib
    $ /opt/enpu/vcann-rt/lib/libvruntime.so 
    ```

    ld.so.preload文件的路径用户可自定义，下面用{preload_path}占位。

3. 使用yaml启动容器

    yaml文件配置可参考如下格式：

    ```shell
    vnpu-base.yaml

    apiVersion: mindxdl.gitee.com/v1
    kind: AscendJob
    metadata:
        name: vnpu-base # 容器名，与yaml文件名一致
        namespace: vnpu # namespace
        labels:
            framework: pytorch
            tor-affinity: "null" # 该标签为任务是否使用交换机亲和性调度，null/不配置：不适用，large-model-schema：大模型任务，normal-schema：普通任务
            fault-scheduling: "force"
            scheduler-share-aicore-quota: "20" # 算力aicore配额，单位：%
            scheduler-share-hbm-quota: "65536" # 显存HBM配额，单位：MB
            scheduler-share-scheduling-policy: "2" # 调度策略，默认弹性模式(elastic mode)
    spec:
        schedulerName: volcano # work when enableGangScheduling is true
        runPolicy:
            schedulingPolicy: # work when enableGangScheduling is true
                minAvailable: 1
                queue: default
        successPolicy: AllWorkers
        replicaSpecs:
            Master:
                replocas: 1
                restartPolicy: Never
                template:
                    spec:
                        nodeSelector:
                            host-arch: huawei-srm
                            accelerator-type: module-910b-8
                        containers:
                        - name: ascend # do not modify
                          image: {image_name} # 镜像名
                          imagePullPolicy: IfNotPresent
                          env:
                            - name: XDL_IP # IP address of the physical node, which is used to identify the node where the pod is running
                              valueFrom:
                                fieldRef:
                                    fieldPath: status.hostIP
                          command:
                            - /bin/bash
                            - -c
                          args: [ "sleep 3000000;" ]
                          ports: # default value containerPort: 2222 name: ascendjob-port if not set
                            - containerPort: 2222 # determined by user
                              name: ascendjob-port # do not modify
                          resources:
                            limits:
                                huawei.com/Ascend910: 1
                            requests:
                                huawei.com/Ascend910: 1
                          volumeMounts:
                          - name: sbin
                            mountPath: /usr/local/sbin/
                          - name: ascend-driver
                            mountPath: /usr/local/Ascend/driver
                          - name: dshm # 共享内存
                            mountPath: /dev/shm
                          - name: localtime
                            mountPath: /etc/localtime
                          - name: share-device-config-dir # 配置文件夹路径
                            mountPath: /etc/enpu/vcann-rt/
                          - name: libpreload # 软切分动态库路径
                            mountPath: /opt/enpu/vcann-rt/lib/libvruntime.so
                          - name: preload # preload配置文件路径
                            mountPath: {preload_path}/ld.so.preload
                        volumes: 
                        - name: sbin
                          hostPath: 
                            path: /usr/local/sbin/
                        - name: ascend_driver
                          hostPath:
                            path: /usr/local/Ascend/driver
                        - name: dshm # 共享内存
                          hostPath:
                            path: /dev/shm
                        - name: localtime
                          hostPath:
                            path: /etc/localtime
                        - name: share-device-config-dir # 配置文件夹路径
                          hostPath:
                            path: /etc/enpu/vcann-rt/vnpu.vnpu-base/ # {配置文件夹路径}/{namespace}.{container_name}/
                            type: DirectoryOrCreate
                        - name: libpreload # 软切分动态库路径
                          hostPath:
                            path: /opt/enpu/vcann-rt/lib/libvruntime.so
                        - name: preload # preload配置文件路径
                          hostPath:
                            path: {preload_path}/ld.so.preload
    ```

    可根据实际业务进行参数配置：
    - metadata:
        - name: 容器名，与yaml文件名一致
        - namespace: 命名空间
        - labels:
            - scheduler-share-aicore-quota: 算力aicore配额，单位：%
            - scheduler-share-hbm-quota: 显存HBM配额，单位：MB
            - scheduler-share-scheduling-policy: 调度策略，默认弹性模式，1：固定配额模式（fixed_share）, 2: 弹性模式（elastic）, 3: 争抢模式（best-effort）
    - containers:
        - image: 镜像名
    - volumeMounts:
        - name: preload # preload配置文件路径
        - mountPath: {preload_path}/ld.so.preload # 步骤2中创建的ld.so.preload文件路径
    - volumes:
        - name: share-device-config-dir # 配置文件夹路径
            - hostPath:
                - path: /etc/enpu/vcann-rt/{namespace}.{container_name}/
        - name: preload # preload配置文件路径
            - hostPath:
                - path: {preload_path}/ld.so.preload # 步骤2中创建的ld.so.preload文件路径

    
    使用yaml拉起容器

    ```shell
    $ kubectl apply -f {container_name.yaml}
    # 查看容器
    $ kubectl get pods -n {namespace}
    ```

4. 拉起vCANN-RT软切分服务

    进入容器：

    ```shell
    $ kubectl exec -it {pod_name} -n {namespace} bash
    ```

    进入容器后，可通过环境变量`ENPU_LOG_LEVEL`配置日志级别。日志级别由高到底分别是FATAL(0), ERROR(1), WARN(2), INFO(3), DEBUG(4)。默认日志级别为INFO。

    示例：

    ```shell
    $ export ENPU_LOG_LEVEL=3
    ```

    启动训推任务时，会自动拉起vCANN-RT算力控制和显存控制服务。

    在容器内可查询配置文件获取vNPU资源配额等信息：`cat /etc/enpu/npu_info.config`


- 方式二：一体机部署


## 约束
1. 软件约束
    - CANN 8.3.RC1, 安装对应的HDK驱动
    - kubernetes 1.28及以上
    - mindcluster xxx版本，部署链接：

2. 主机侧环境配置

    主机侧通过npu-smi工具开启容器共享模式：

    ```shell
    $ npu-smi set -t device-share -i {id} -c {chip_id} -d {value}
    ```

    参数说明：

    - id : 设备id, 通过`npu-smi info -l`命令查出的NPU ID即为设备id.
    - chip_id : 芯片id, 通过`npu-smi info -m`命令查出的Chip ID即为芯片id.
    - value : 容器共享模式使能状态：分为禁用(0)、使能(1)。默认禁用禁用。
    
    示例：开启设备0的容器共享模式：`npu-smi set -t device-share -i 0 -d 1`

## FAQ
1. hook拦截runtime API提示`can't find function`

    当前vCANN-RT方案适配CANN软件版本为商发版本8.3.RC1，部分runtime API在CANN其他版本未支持

