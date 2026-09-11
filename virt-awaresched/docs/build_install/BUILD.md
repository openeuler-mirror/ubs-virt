# 构建指导

virt-awaresched支持两种方式编译构建：

## 方式一：基于物理机环境编译

### 拉取源码

```shell
git clone https://gitcode.com/openeuler/ubs-virt.git
# 进入项目目录
cd virt-awaresched
```

### 构建工具链准备

```shell
sudo yum install gcc-c++ gcc cmake make -y
```

### 构建

#### 构建依赖

virt-awaresched构建依赖信息已经记录在spec文件（virt-awaresched.spec）中。

具体内容如下：

```shell
Requires: patch libvirt-devel libboundscheck
```

#### 安装依赖

yum命令手动安装

```shell
sudo yum install patch libvirt-devel libboundscheck -y
```

#### 执行构建

- 执行Release构建（无调试信息），产物输出到项目顶层目录的build/目录下。

    ```shell
    bash build.sh
    ```

- 执行Debug构建（包含调试信息），产物输出到项目顶层目录的build/目录下。

    ```shell
    bash build.sh -D
    ```

### 生成rpm包

构建项目，并打包成RPM文件输出到项目顶层目录下得output/目录下。

```shell
bash build.sh package
```

产物如下：

```filepath
└── output                                                            # 打包输出文件目录
    └── virt-awaresched-1.0.0-1.aarch64.rpm                           # virt-awaresched rpm安装包
```

## 方式二：基于预编译镜像环境编译

下载预编译镜像，预编译镜像中已安装`virt-awaresched`编译所需依赖，基于预编译镜像编译`virt-awaresched`可节省环境准备时间。

```shell
docker pull swr.cn-north-4.myhuaweicloud.com/ubscore/ubs-virt:oe2403-v1
```

启动容器，示例如下

```shell
docker run -it --name=vas-build swr.cn-north-4.myhuaweicloud.com/ubscore/ubs-virt:oe2403-v1 /bin/bash
```

在容器内下载源码

```shell
git clone https://gitcode.com/openeuler/ubs-virt.git
cd virt-awaresched
```

在容器内执行构建

```shell
bash build.sh
```

编译完成之后，产物输出到项目顶层目录的`build/`目录下，与方式一的编译产物一致。

如需生成RPM包，可执行：

```shell
bash build.sh package
```

`bash build.sh package`在执行编译的基础上，会额外将编译产物打包生成RPM安装包，产物与方式一"生成rpm包"的产物一致，输出到项目顶层目录的`output/`目录下，即`virt-awaresched-1.0.0-1.aarch64.rpm`。
