# ubs-virt-ovs

#### 介绍
`ubs-virt-ovs`是ubs-virt提供网络的能力的服务，支持配置指定urma设备的带宽

#### 源码下载
可以使用如下两种方式下载`ubs-virt-ovs`源码。

```shell
# 方法一
$ git clone <ubs-virt-ovs-repo-url>
$ git submodule update --init --recursive
# 方法二
$ git clone <ubs-virt-ovs-repo-url> --recurse-submodules
```

#### 源码目录结构
`ubs-virt-ovs`源码的主要目录结构如下所示

```shell
.
├── build   // 存放项目中编译构建使用的脚本文件
├── sdk       // 存放对外暴露的sdk的代码
├── src       // 存放项目的功能实现源码，仅该目录参与构建出包
└── test      // 存放项目的ut和dtfuzz等
```

#### 编译
`ubs-virt-ovs`在代码仓中提供了统一的编译构建脚本（即`build.sh`），可以直接执行该脚本编译构建。默认无需任何配置项，直接执行即可。
编译产物存在于build/output目录下

```shell
$ ./build/build.sh
[INFO] Build type: RelwithDebinfo
[INFO] Project root: /tmp/xxx
[INFO] Spec file: /tmp/xxx/ubs-virt-ovs/build/ubs-virt-ovs.spec
[INFO] Source tar created: /tmp/xxx/ubs-virt-ovs/build/rpmbuild/SOURCES/ubs-virt-ovs-1.0.0.tar.gz
...
[INFO] Outpud RPMs:
total 716K
-rw-------. 1 root root  50K ubs-virt-ovs-1.0.0-1.rel.aarch64.rpm
-rw-------. 1 root root 640K ubs-virt-ovs-debuginfo-1.0.0-1.rel.aarch64.rpm
-rw-------. 1 root root  23K ubs-virt-ovs-debugsource-1.0.0-1.rel.aarch64.rpm
```
#### 部署
将编译产物上传到环境上，rpm安装即可
```shell
$ rpm -ivh ubs-virt-ovs-*
```