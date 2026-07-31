# UBS Optimizer (UB Service Optimizer)

UB Service Optimizer (`UBS Optimizer`) is designed to optimize virtual machine performance in Ascend-based virtualization scenarios. It identifies inference performance bottlenecks and delivers optimization policies.

---

## 📌 Quick Start

### Environment Requirements

- Recommended operating system: openEuler 24.03 LTS SP4
- Supported CPU architectures: x86_64 and aarch64

### Software Dependencies

| Package            | Version                   |
|:-------------------|:--------------------------|
| git                | 2.33.0 or later           |
| cmake              | 3.22.0 or later           |
| make               | 4.3 or later              |
| gcc                | 10.3.1 or later           |
| g++                | 10.3.1 or later           |
| rapidjson-devel    | 1.1.0 or later            |
| bpftool            | 7.2.0 or later            |
| libbpf             | 0.8.1 or later            |
| libbpf-devel       | 0.8.1 or later            |
| clang              | 12.0.1 or later           |
| tar                | 1.34 or later             |
| rpm-build          | 4.18.2 or later           |

### Installation and Deployment

#### Method 1: Install from a yum Repository

openEuler 24.03 LTS SP4 is recommended. Install the dependencies:

```bash
yum install git cmake make gcc g++ rapidjson-devel bpftool libbpf libbpf-devel clang tar rpm-build
```

Install the `ubs-virt-optimizer` package from the openEuler repository:

```bash
yum install ubs-virt-optimizer
```

> **SSL certificate verification errors:** If yum reports an SSL certificate verification failure, first check the system time, CA certificates, and network proxy configuration. If the certificate issue cannot be fixed immediately and the network is trusted, `sslverify=false` can be used as a temporary fallback for the current command:
>
> ```bash
> yum --setopt=sslverify=false install ubs-virt-optimizer
> ```
>
> Disabling SSL certificate verification reduces the security of package downloads and is not recommended as a persistent or global setting. For details, see the [Deployment Guide](./docs/build_install/部署说明.md).

#### Method 2: Build and Install from Source

openEuler 24.03 LTS SP4 is recommended. Obtain the source code:

```bash
git clone https://gitcode.com/openeuler/ubs-virt.git
cd ubs-virt/virt-optimizer
```

Install the dependencies:

```bash
yum install git cmake make gcc g++ rapidjson-devel bpftool libbpf libbpf-devel clang tar rpm-build
```

Build the project:

```bash
bash build.sh
```

The build output is stored in `ubs-virt/virt-optimizer/output/release/`. The RPM file name follows the format `ubs-optimizer-<version>-<release>-<arch>.rpm`, where `<arch>` is `x86_64` or `aarch64`.

---

## 🧩 Core Functions

- **Collects kernel event performance data**
- **Identifies virtual machine performance bottlenecks in Ascend-based inference scenarios**
- **Provides optimization suggestions with corresponding descriptions based on bottlenecks**

---

## 📂 Project Structure

```text
virt-optimizer/
├── ebpf/                       # ubs-optimizer core service
│   ├── src/
│   │   ├── client/             # Collection module
│   │   ├── common/             # Common modules
│   │   ├── optimizer/          # Optimizer-related modules
│   │   └── server/             # Data processing module
│   └── tests/                  # Test cases
├── docs/                       # Documentation
├── build.sh                    # Build script
├── build_rpm.sh                # RPM build sub-script
└── README.md                   # Project description
```

---

## 🧪 Developer Testing

The project contains a complete unit test suite:

1. Add or update unit tests in `virt-optimizer/ebpf/tests/`.
2. Enter the directory containing the test script:

   ```bash
   cd ubs-virt/virt-optimizer/ebpf
   ```

3. Run the unit tests:

   ```bash
   bash run_ut.sh
   ```

4. The script builds the project in Debug mode and runs all unit tests. The default openEuler 24.03 LTS SP4 repositories do not provide the `lcov` package, so coverage reporting is optional. If both `lcov` and `genhtml` are already available, the report is generated in `virt-optimizer/build/debug/tests/gcover_report/`; otherwise, report generation is skipped without affecting the unit tests.

For details, see the [Unit Test Development Guide](./docs/test/单元测试开发指南.md).

---

## 🛠️ Usage Guide

Deploy virt-optimizer to the virtual machine and host according to the Ascend server virtualization scenario. The following example describes the deployment and optimization workflow:

- **Scenario example**: [Best Practices for Using ubs-optimizer](./docs/example/最佳实践：如何使用ubs-optimizer.md)

---

## 📚 Documentation Index

All technical documents are located in the [`docs/`](./docs/) directory:

- **Build guide**: [Build Guide](./docs/build_install/构建指导.md)
- **Deployment guide**: [Deployment Guide](./docs/build_install/部署说明.md)
- **Configuration guide**: [Configuration Guide](./docs/config/配置说明.md)
- **Scenario example**: [Best Practices for Using ubs-optimizer](./docs/example/最佳实践：如何使用ubs-optimizer.md)
- **Test guide**: [Unit Test Development Guide](./docs/test/单元测试开发指南.md)
- **Performance optimization methods**: [Performance Optimization Methods](./docs/optimize_operations/性能优化方法.md)
- **Security statement**: [Security Statement](./docs/security/安全声明.md)

---

> Project homepage: [https://gitcode.com/openeuler/ubs-virt/tree/master](https://gitcode.com/openeuler/ubs-virt/tree/master)
