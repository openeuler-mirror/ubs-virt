# ubs-virt

## 介绍

Ubs-virt 包含多个虚拟化功能组件，分别提供NPU算力软切分，虚拟机线性度调优，NPU直通虚拟机大模型推理性能调优的功能。

## 涉及组件

1. ubs-virt-enpu: ubs-virt提供NPU算力切分的服务，支持配置指定的算力和显存资源。
2. virt-awaresched: virt-awaresched提供对虚拟化调度调优的服务，减少vCPU无意义迁移的性能开销，提升虚机的线性度。
3. virt-optimizer: virt-optimizer提供NPU直通虚拟机大模型推理性能瓶颈分析，性能调优建议的功能。

## 使用说明

1. ubs-virt-enpu: [ubs-virt-enpu使用说明](./ubs-virt-enpu/README.md)
2. virt-awaresched：[virt-awaresched使用说明](./virt-awaresched/README.md)
3. virt-optimizer: [virt-optimizer使用说明](./virt-optimizer/README.md)
