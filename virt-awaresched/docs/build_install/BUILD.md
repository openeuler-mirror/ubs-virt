# 构建指导

## 拉取源码

```shell
git clone https://gitee.com/openeuler/ubs-virt.git
# 进入项目目录
cd virt-awaresched
```

## 构建

### 构建依赖

virt-awaresched构建依赖信息已经记录在spec文件(virt-awaresched.spec)中

具体内容如下：

```shell
Requires: libvirt libsecurec libcap
```

### 安装依赖

**方式一：yum手动安装**

```shell
yum install -y libsecurec
```

### 执行构建

```shell
# 执行 Release 构建（没有调试信息），产物输出到项目顶层目录的build/目录下
bash build.sh

# 执行 Debug 构建（包含调试信息），产物输出到项目顶层目录的build/目录下
bash build.sh -D
```

## 形成rpm包

```shell
# 构建项目，并打包成 rpm 文件输出到项目顶层目录的 output/ 下。
bash build.sh package
```

产物如下：

```
└── output                                                            # 打包输出文件目录
    └── virt-awaresched-1.0.0-1.aarch64.rpm                           # virt-awaresched rpm安装包
```