# UBS Optimizer (UB Service Optimizer)

`UBS Optimizer`（UB Service Optimizer，简称 UBS Optimizer）是昇腾虚拟化场景下针对虚拟机性能优化的调优工具。使用该工具可实现昇腾虚拟化场景下推理性能瓶颈识别和调优下发。

---

## 📌 快速开始

### 环境要求

- 推荐操作系统：openEuler 24.03 LTS SP4
- 支持的 CPU 架构：x86_64、aarch64

### 软件依赖要求

| 软件包             | 版本信息           |
|:-------------------|:-------------------|
| git                | 2.33.0 及以上版本  |
| cmake              | 3.22.0 及以上版本  |
| make               | 4.3 及以上版本     |
| gcc                | 10.3.1 及以上版本  |
| g++                | 10.3.1 及以上版本  |
| rapidjson-devel    | 1.1.0 及以上版本   |
| bpftool            | 7.2.0 及以上版本   |
| libbpf             | 0.8.1 及以上版本   |
| libbpf-devel       | 0.8.1 及以上版本   |
| clang              | 12.0.1 及以上版本  |
| tar                | 1.34 及以上版本    |
| rpm-build          | 4.18.2 及以上版本  |

### 安装部署

#### 方式一：通过 yum 源安装

推荐使用 openEuler 24.03 LTS SP4。安装依赖软件：

```bash
yum install git cmake make gcc g++ rapidjson-devel bpftool libbpf libbpf-devel clang tar rpm-build
```

安装 openEuler 软件源中的 `ubs-virt-optimizer` 软件包：

```bash
yum install ubs-virt-optimizer
```

> **SSL 证书校验异常：**如果 yum 提示 SSL 证书校验失败，应优先检查系统时间、CA 证书和网络代理配置。在暂时无法修复证书问题且网络环境可信的情况下，可将 `sslverify=false` 作为临时备选方案，仅对当前命令关闭 SSL 证书校验：
>
> ```bash
> yum --setopt=sslverify=false install ubs-virt-optimizer
> ```
>
> 关闭 SSL 证书校验会降低软件包下载过程的安全性，不建议长期或全局配置。详细说明请参阅[部署说明](./docs/build_install/部署说明.md)。

#### 方式二：从源码构建安装

推荐使用 openEuler 24.03 LTS SP4。获取源码：

```bash
git clone https://gitcode.com/openeuler/ubs-virt.git
cd ubs-virt/virt-optimizer
```

安装依赖软件：

```bash
yum install git cmake make gcc g++ rapidjson-devel bpftool libbpf libbpf-devel clang tar rpm-build
```

构建项目：

```bash
bash build.sh
```

构建产物位于 `ubs-virt/virt-optimizer/output/release/` 目录，RPM 文件名格式为 `ubs-optimizer-<version>-<release>-<arch>.rpm`，其中 `<arch>` 为 `x86_64` 或 `aarch64`。

---

## 🧩 核心功能

- **采集内核事件性能数据**
- **识别昇腾推理场景下虚拟机性能瓶颈**
- **基于瓶颈提供调优建议和调优项说明**

---

## 📂 项目结构概览

```text
virt-optimizer/
├── ebpf/                       # ubs-optimizer 核心服务
│   ├── src/
│   │   ├── client/             # 采集模块
│   │   ├── common/             # 通用模块
│   │   ├── optimizer/          # 优化器相关模块
│   │   └── server/             # 数据处理模块
│   └── tests/                  # 测试用例
├── docs/                       # 文档
├── build.sh                    # 构建脚本
├── build_rpm.sh                # RPM 构建子脚本
└── README.md                   # 项目说明文件
```

---

## 🧪 开发者测试

项目包含完整的单元测试（UT）：

1. 在 `virt-optimizer/ebpf/tests/` 目录下新增或修改单元测试代码。
2. 进入测试脚本所在目录：

   ```bash
   cd ubs-virt/virt-optimizer/ebpf
   ```

3. 执行单元测试脚本：

   ```bash
   bash run_ut.sh
   ```

4. 脚本将以 Debug 模式构建并运行所有单元测试。openEuler 24.03 LTS SP4 默认软件源不提供 `lcov` 软件包，因此覆盖率报告为可选项：环境中已有 `lcov` 和 `genhtml` 命令时，报告生成在 `virt-optimizer/build/debug/tests/gcover_report/`；命令不存在时，脚本会跳过报告生成，不影响单元测试执行。

详细说明请参阅[单元测试开发指南](./docs/test/单元测试开发指南.md)。

---

## 🛠️ 工具使用指导

使用 virt-optimizer 时，需要结合昇腾服务器虚拟化场景，将工具合理部署到虚拟机和宿主机上。可参考以下示例完成部署和调优分析：

- **场景示例**：[最佳实践：如何使用 ubs-optimizer](./docs/example/最佳实践：如何使用ubs-optimizer.md)

---

## 📚 文档索引

所有技术文档位于 [`docs/`](./docs/) 目录，包括：

- **构建指导**：[构建指导.md](./docs/build_install/构建指导.md)
- **部署说明**：[部署说明.md](./docs/build_install/部署说明.md)
- **配置说明**：[配置说明.md](./docs/config/配置说明.md)
- **场景示例**：[最佳实践：如何使用 ubs-optimizer](./docs/example/最佳实践：如何使用ubs-optimizer.md)
- **测试指南**：[单元测试开发指南.md](./docs/test/单元测试开发指南.md)
- **性能优化方法**：[性能优化方法.md](./docs/optimize_operations/性能优化方法.md)
- **安全声明**：[安全声明.md](./docs/security/安全声明.md)

---

> 项目主页：[https://gitcode.com/openeuler/ubs-virt/tree/master](https://gitcode.com/openeuler/ubs-virt/tree/master)
