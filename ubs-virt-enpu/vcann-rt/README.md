# vCANN-RT

## 介绍

`vCANN-RT(virtual CANN Runtime)`是ubs-virt-enpu提供NPU算力软切分的方案。本方案需要依赖Linux系统Preload Hook的能力，通过预加载软切分动态库，拦截部分runtime API的调用，根据算力和显存资源配额信息，进行算力控制或显存控制。

## 环境准备

### 软件版本

##### Atlas A2 推理系列产品

**表 1 软件版本**

| 软件                | 版本                                                                        |
|:---------------------|:-----------------------------------------------------------------------------|
| CANN                | 8.5.0                                                                     |
| HDK                 | 25.5.0及以上版本                                                            |
| （可选）Kubernetes  | 1.17.x~1.34.x，推荐使用1.19.x及以上版本。<br>（直接使用Docker部署则不需要）|
| （可选）MindCluster | 26.0.0（直接使用Docker部署则不需要）|

#### Atlas A3 推理系列产品

**表 2 软件版本**

| 软件                | 版本                                                                        |
|:---------------------|:-----------------------------------------------------------------------------|
| CANN                | 8.5.0                                                                     |
| HDK                 | 26.0.0及以上版本                                                            |
| （可选）Kubernetes  | 1.17.x~1.34.x，推荐使用1.19.x及以上版本。<br>（直接使用Docker部署则不需要）|
| （可选）MindCluster | 26.0.0（直接使用Docker部署则不需要）|

### 主机侧环境配置

主机侧通过`npu-smi`工具开启容器共享模式。若配合MindCluster使用，要求整节点开启容器共享模式。

```shell
npu-smi set -t device-share -i ${id} -c ${chip_id} -d ${value}
```

**表 3 参数说明**

|参数|参数选项|说明|
|:---|:---|:---|
|id|设备id|通过`npu-smi info -l`命令查询获取的NPU ID即为设备id。|
|chip_id|芯片id|通过`npu-smi info -m`命令查询获取的Chip ID即为芯片id。|
|value|<ul>默认值：禁用<li>禁用(0)</li><li>使能(1)</li></ul>|容器共享模式使能状态。|
    
  示例：
  开启设备0所有芯片的容器共享模式。

  ```shell
  npu-smi set -t device-share -i 0 -d 1
  ```

主机侧可设置容器共享持久化使能状态。

```shell
npu-smi set -t device-share-cfg-recover -d ${value}
```

**表 4 参数说明**

|参数|参数选项|说明|
|:---|:---|:---|
|value|<ul>默认值：禁用<li>禁用(0)</li><li>使能(1)</li></ul>|容器共享持久化使能状态。|

（可选）针对A3 推理系列产品，如采用下文提到的docker方式部署，则在主机侧需要通过`npu-smi`工具开启支持单Die通入容器的模式。

```shell
npu-smi set -t multi-die-policy -d ${value}
```
**表 5 参数说明**

|参数|参数选项|说明|
|:---|:---|:---|
|value|<ul>默认值：双Die联合模式<li>双Die联合模式(0)</li><li>单Die独立模式(1)</li></ul>|A3的device通入容器的策略模式|

### 虚拟化环境检测工具准备

  用户需要保证业务容器`/usr/bin`目录内包含`systemd-detect-virt`命令行工具，该工具用于检测系统的运行环境是否为虚拟化环境和虚拟化方式。若容器内未安装systemd-detect-virt工具，软切分动态库在调用dcmi接口时会出现故障。

## 源码获取

在编译vCANN-RT源码前，用户需要先设置CANN的环境变量。

可以使用如下方式获取`vCANN-RT`源码。

```shell
git clone <ubs-virt-enpu-vcann-rt-url>
```

### 源码目录结构

`vCANN-RT`源码的主要目录结构如下所示：

```shell
.
├── scripts   // 存放项目中编译构建使用的脚本文件
├── src       // 存放项目的功能实现源码，仅该目录参与构建出包
└── test      // 存放项目的ut用例
```

## 编译

在编译vCANN-RT源码前，用户需要先设置CANN的环境变量。

`vCANN-RT`在代码仓中提供了统一的编译构建脚本（即`make_build.sh`文件），可以直接执行该脚本文件进行编译构建。默认无需任何配置项，直接执行即可。

```shell
bash make_build.sh 8.5.0
```

编译完成之后，会在`build`目录下面产生相应的编译产物（如果当前编译环境安装的cann版本不是8.5.0，则不需要增加编译参数）。

## 部署

用户可以选择将软切分动态库`libvruntime.so`和检测工具`enpu-monitor`分别拷贝到`/opt/enpu/vcann-rt/lib`和`/opt/enpu/vcann-rt/tools`目录。（如果选择不拷贝，则需要在后面的流程中修改相应的路径。）

### 启动服务

vCANN-RT支持两种方式启动服务：

#### 方式一：基于kubernetes编排系统部署

  用户通过k8s yaml文件，在申请容器时声明所需的vNPU算力资源百分比例和显存资源数量。k8s会将容器调度到资源充足的节点，并将算力和显存资源配额等信息以配置文件挂载到容器内。同时，k8s会将vCANN-RT的包挂载到容器，容器内即可使能vCANN-RT软切分能力。

1. 配置preload文件。

    创建ld.so.preload文件，并将libvruntime.so的路径配置到ld.so.preload文件中，用于K8s挂载vCANN-RT到容器。

    ```shell
    vi ld.so.preload
      # 例如libvruntime.so 的路径为/opt/enpu/vcann-rt/lib
    /opt/enpu/vcann-rt/lib/libvruntime.so
    ```

    ld.so.preload文件的路径用户可自定义，文档后续内容中使用${preload_path}表示。

2. <span id="step2">使用yaml启动容器。</span>

    yaml文件配置可参考如下格式：

    ```shell
    vnpu-base.yaml

    apiVersion: mindxdl.gitee.com/v1
    kind: AscendJob
    metadata:
        # 容器名，需与yaml文件名一致
        name: vnpu-base 
        # namespace
        namespace: vnpu 
        labels:
            framework: pytorch
            # 该标签为任务是否使用交换机亲和性调度，配置项如下：
              # null/不配置：不使用
              # large-model-schema：大模型任务
              # normal-schema：普通任务
            tor-affinity: "null"
            fault-scheduling: "force"
            # 算力aicore配额，单位：%
            huawei.com/scheduler.softShareDev.aicoreQuota: "20" 
            # 显存HBM配额，单位：MB
            huawei.com/scheduler.softShareDev.hbmQuota: "65536" 
            # 调度策略，默认弹性模式(elastic)，可选fixed-share，elastic，best-effort
            huawei.com/scheduler.softShareDev.policy: "elastic" 
        annotations:
            # 集群调度组件的调度策略
            huawei.com/schedule_policy: "chip1-softShareDev"
    spec:
        # work when enableGangScheduling is true
        schedulerName: volcano 
        runPolicy:
            # work when enableGangScheduling is true
            schedulingPolicy: 
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
                            host-arch: huawei-arm
                            accelerator-type: module-910b-8
                        containers:
                        # do not modify
                        - name: ascend 
                          # 镜像名称
                          image: ${image_name} 
                          imagePullPolicy: IfNotPresent
                          env:
                            # IP address of the physical node, which is used to identify the node where the pod is running
                            - name: XDL_IP 
                              valueFrom:
                                fieldRef:
                                    fieldPath: status.hostIP
                          command:
                            - /bin/bash
                            - -c
                          args: [ "sleep 3000000;" ]
                          ports: 
                            # default value containerPort: 2222 name: ascendjob-port if not set
                            # determined by user
                            - containerPort: 2222
                              # do not modify 
                              name: ascendjob-port 
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
                            # 共享内存
                          - name: dshm 
                            mountPath: /dev/shm
                          - name: localtime
                            mountPath: /etc/localtime
                            # 配置文件夹路径
                          - name: share-device-config-dir
                            mountPath: /etc/enpu/vcann-rt/
                            # 软切分动态库路径
                          - name: libpreload 
                            mountPath: /opt/enpu/vcann-rt/lib/libvruntime.so
                            # preload配置文件路径
                          - name: preload 
                            mountPath: ${preload_path}/ld.so.preload
                        volumes: 
                        - name: sbin
                          hostPath: 
                            path: /usr/local/sbin/
                        - name: ascend-driver
                          hostPath:
                            path: /usr/local/Ascend/driver
                          # 共享内存  
                        - name: dshm 
                          hostPath:
                            path: /dev/shm/
                        - name: localtime
                          hostPath:
                            path: /etc/localtime
                          # 配置文件夹路径
                        - name: share-device-config-dir 
                          hostPath:
                            # {配置文件夹路径}/${namespace}.${container_name}/
                            path: /etc/enpu/vcann-rt/vnpu.vnpu-base/ 
                            type: DirectoryOrCreate
                          # 软切分动态库路径
                        - name: libpreload 
                          hostPath:
                            # preload配置文件路径
                            path: /opt/enpu/vcann-rt/lib/libvruntime.so
                        - name: preload 
                          hostPath:
                            path: ${preload_path}/ld.so.preload
    ```

    可根据实际业务进行参数配置：
    - metadata:
        - name: 容器名，与yaml文件名一致。
        - namespace: 命名空间。
        - labels:
            - huawei.com/scheduler.softShareDev.aicoreQuota: 算力aicore配额，单位：%
            - huawei.com/scheduler.softShareDev.hbmQuota: 显存HBM配额，单位：MB
            - huawei.com/scheduler.softShareDev.policy: 调度策略，默认为弹性模式。
              - 固定配额模式（fixed_share）
              - 弹性模式（elastic）
              - 争抢模式（best-effort）
    - containers:
        - image: 镜像名。
    - volumeMounts:
        - name: preload # preload配置文件路径。
        - mountPath: ${preload_path}/ld.so.preload # [步骤2](#step2)中创建的ld.so.preload文件路径。
    - volumes:
        - name: share-device-config-dir # 配置文件夹路径。
            - hostPath:
                - path: /etc/enpu/vcann-rt/${namespace}.${container_name}/
        - name: preload # preload配置文件路径。
            - hostPath:
                - path: ${preload_path}/ld.so.preload # [步骤2](#step2)中创建的ld.so.preload文件路径。
    
    使用yaml文件拉起容器：

    ```shell
    kubectl apply -f ${container_name.yaml}
    # 查看容器
    kubectl get pods -n ${namespace}
    ```

    如果容器启动失败，可参考方式二中的解决方法。
3. 拉起vCANN-RT软切分服务。

    进入容器：

    ```shell
    kubectl exec -it ${pod_name} -n ${namespace} bash
    ```

    进入容器后，可通过环境变量`ENPU_LOG_LEVEL`配置日志级别。日志级别由高到底分别是FATAL(0), ERROR(1), WARN(2), INFO(3), DEBUG(4)。默认日志级别为INFO。

    示例：

    ```shell
    export ENPU_LOG_LEVEL=3
    ```

    启动推理任务时，会自动拉起vCANN-RT算力控制和显存控制服务。

    在容器内可查询配置文件获取vNPU资源配额等信息：

    ```shell
    cat /etc/enpu/vcann-rt/npu_info.config
    ```

    在容器内可通过监测工具查询vNPU资源配额和内存使用情况等信息：

    ```shell
    ./opt/enpu/vcann-rt/tools/enpu-monitor
    ```

#### 方式二：docker方式部署（不依赖kubernetes组件）

  当不依赖k8s组件使用vCANN-RT时，需要用户在启动容器时自行挂载软切分相关动态库、文件和设备。例如软件分动态库、配置文件、共享内存设备和物理NPU设备。具体步骤如下：

1. 创建vNPU配置文件和共享内存设备。

    针对每个容器，需要在主机侧创建一个配置文件，并映射到容器的npu_info.config文件中（由于不同容器的配置内容不同，需要确保每个容器的配置文件在主机侧独立存储并明确区分，比如通过文件名或者路径区分）。配置文件的格式和字段示例如下：

    ```bash
      physical-npu-id=0
      virtual-npu-id=0
      aicore-quota=20
      memory-quota=1024
      shm-id=xxx
      scheduling-policy=1
    ```

    **表 6 配置项说明**

    |参数|参数选项|说明|
    |:---|:---|:---|
    |physical-npu-id|物理NPU id|`physical-npu-id=0`表示使用第0张物理NPU。|
    |virtual-npu-id|vNPU id|需要从0开始配置，并且同一个物理NPU下的vNPU不允许重复。|
    |aicore-quota|AI Core资源配额，单位为%|表示算力使用的时间比例。当前每个time slice默认为100ms, 通过软件硬编码，不支持动态配置。假如申请了20%的算力资源，那么该容器有20ms的NPU使用权。|
    |memory-quota|HBM资源配额，单位为MB|表示显存资源使用容量。当前容器内所有进程使用的HBM总量不能超过HBM资源配额。|
    |shm-id|共享内存文件名称|该文件名称采用物理NPU对应的VDie ID, 可以保证全局唯一。<br>通过`npu-smi info -t board -i ${id} -c ${chip_id}`命令查询VDie ID。<br>查询完成之后，可以通过`-`符号拼接成文件名称，例如：`shm-id=11111111-22222222-33333333-44444444-55555555` 用户需要确保宿主机中存在此共享内存文件。|
    |scheduling-policy|<ul>默认配置为2。<li>1: fixed-share mode</li><li>2: elastic mode</li><li>3: best-effort mode</li></ul>|调度策略。|

    此外，需要设置配置文件具有合适的权限，建议为644。

2. 预加载配置文件（若使用指定容器镜像，可以跳过此步骤）。

    在主机侧创建预加载动态库文件`ld.so.preload`, 文件内容为libvruntime.so的路径，例如：`/opt/enpu/vcann-rt/lib/libvruntime.so`

3. 启动业务容器。

    用户在启动容器时，需要将软切分相关动态库、文件和设备挂载到容器，容器启动命令可参考(假如使用第0张NPU卡)：

    ```bash
      docker run -it --name=container_name \
      --device=/dev/davinci0:/dev/davinci0 \
      --device=/dev/davinci_manager \
      --device=/dev/hisi_hdc:/dev/hisi_hdc \
      -v /usr/local/sbin:/usr/local/sbin \
      -v /usr/local/Ascend/driver:/usr/local/Ascend/driver \
      -v /dev/shm:/dev/shm \
      -v /opt/enpu/vcann-rt/lib/libvruntime.so:/opt/enpu/vcann-rt/lib/libvruntime.so \
      -v /opt/enpu/vcann-rt/tools/enpu-monitor:/opt/enpu/vcann-rt/tools/enpu-monitor \
      -v /xxx/npu_info.config:/etc/enpu/vcann-rt/npu_info.config \
      -v /xxx/ld.so.preload:/etc/ld.so.preload \
      image_name /bin/bash
    ```

    如果遇到容器启动报错`libboundscheck.so: cannot open shared object file: No such file or directory`，则说明动态库libvruntime.so依赖的安全函数库在容器中无法找到。解决办法：

      1. 取消容器启动命令中的/etc/ld.so.preload的映射，并重新启动容器。

      2. 参照文末FAQ中方法安装安全函数库，并确保动态库libvruntime.so能够正常链接。

      3. 执行`export LD_PRELOAD=/opt/enpu/vcann-rt/lib/libvruntime.so`命令。

    **表 7 参数说明**

    |文件/工具|路径|
    |:---|:---|
    |软切分动态库|主机侧和容器内均为建议路径：`/opt/enpu/vcann-rt/lib/libvruntime.so`|
    |物理NPU设备|主机侧和容器内均为建议路径：`/opt/enpu/vcann-rt/tools/enpu-monitor`|
    |共享内存设备|主机侧和容器内均为固定路径：`/dev/davinci0`|
    |共享内存设备|主机侧和容器内均为固定路径：`/dev/shm`|
    |配置文件|<ul><li>主机侧可存放在自定义路径。</li><li>容器内为固定路径：`/etc/enpu/vcann-rt/npu_info.config`</li></ul>|
    |预加载动态库文件|主机侧可存放在自定义路径，容器内为固定路径：`/etc/ld.so.preload`|

    若用户使用指定镜像，则启动命令可以简化为：

    ```bash
      docker run -it --name=container_name \
      --device=/dev/davinci0:/dev/davinci0 \
      --device=/dev/davinci_manager \
      --device=/dev/hisi_hdc:/dev/hisi_hdc \
      -v /usr/local/sbin:/usr/local/sbin \
      -v /usr/local/Ascend/driver:/usr/local/Ascend/driver \
      -v /dev/shm:/dev/shm \
      -v /xxx/npu_info.config:/etc/enpu/vcann-rt/npu_info.config \
      image_name /bin/bash
    ```

4. 启动业务，使用vCANN-RT服务

    - 拉起容器之后，启动训练推理任务前，可以通过环境变量配置日志级别。例如：

      ```bash
      export ENPU_LOG_LEVEL=3
      ```

    - 推理任务启动时，会自动拉起vCANN-RT服务进行算力控制和显存控制，如果日志回显内容为`"Successfully to initialize all module."`, 则表示vCANN-RT服务启动成功。

    - 在容器内可通过监测工具查询vNPU资源配额和内存使用情况等信息。

      ```bash
      ./opt/enpu/vcann-rt/tools/enpu-monitor
      ```
      
## 约束

- 由于vCANN-RT解决方案使用了共享内存，因此用户需要确保在可信用户范围内使用。
- 单个物理NPU卡支持的最大容器数量为100个，单个vNPU支持的最大进程数为128。
- 当用户的配置文件错误时，只有在容器业务拉起之后才会报错。例如，若用户配置aicore-quota=120，此时算力资源配额已经超出100%，在容器启动时不会报错，只有当容器内的使用vNPU的进程启动后，才会返回报错，此时需要用户在主机侧修改配置文件，并重新拉起容器。
- 若用户通过源码自行编译软切分动态库，则需要保证主机侧的CANN版本和容器镜像中的CANN版本保持一致。

## FAQ

1. hook拦截runtime API提示`can't find function`:

    当前vCANN-RT方案适配CANN软件版本为商发版本8.5.0，部分runtime API在CANN其他版本未支持。

2. 容器启动或者业务运行时报错`GLIBC_xxx not found`:
    
    由于GLIBC兼容性问题，容器使用的动态库要求的GLIBC版本和容器本身的版本不兼容，建议进行兼容适配。

3. 容器启动或者业务运行时报错`libboundscheck.so: cannot open shared object file: No such file or directory`：

    由于软切分动态库在运行时依赖安全函数库，因此用户需要确保容器中存在安全函数库，可以参考 [https://gitcode.com/openeuler/libboundscheck](https://gitcode.com/openeuler/libboundscheck) 完成构建部署。
