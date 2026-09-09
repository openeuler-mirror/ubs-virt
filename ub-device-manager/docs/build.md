RPM 构建指导
范围与产物
软件包名称为 ub-device-manager，版本同时定义在 build_rpm.sh 和 ub_device_manager.spec 中。SPEC 文件位于子项目根目录：

操作系统要求：OpenEuler 24.03 LTS SP4

ub-device-manager/
├── build_rpm.sh
├── requirements.txt
├── ub_device_manager.spec
└── ub_device_manager/

构建完成后，RPM包会收集ubs-virt/ub-device-manager下的 output/ 目录。

前置操作
开始前安装以下工具：

dnf install -y rpm-build tar python3-pip

使用 Python 官方公共 PyPI 源：

python3.11 -m pip config set global.index-url https://pypi.org/simple
python3.11 -m pip config list

准备源码和依赖
拉取代码仓源码，然后进入 ub-device-manager 目录：

cd ubs-virt/ub-device-manager

根据 requirements.txt 下载并安装 Python 依赖：

python3.11 -m pip install -r requirements.txt

开始构建
在 ub-device-manager 目录中执行：

bash build_rpm.sh

构建成功后，可在以下位置找到二进制 RPM：

output/ub-device-manager-*.rpm

build_rpm.sh 会将 requirements.txt 放入 RPM，但不会将 pip 下载的依赖 打入 RPM。如果将 RPM 安装到另一台机器，目标机器也需要按照 requirements.txt 安装 Python 依赖，并单独安装 UBSE 运行环境。