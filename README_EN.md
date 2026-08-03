# ubs-virt

## Overview

Ubs-virt includes multiple virtualization feature components, each offering capabilities like soft partitioning of NPU computing power, tuning VM linearity, and optimizing NPU passthrough for large model inference performance in VMs.

## Components

1. ubs-virt-enpu: ubs-virt enables NPU compute capability allocation, supporting configurable allocation of compute capability and graphics memory resources.
2. virt-awaresched: virt-awaresched optimizes virtualization scheduling, reducing the performance overhead of unnecessary vCPU migration and improving the linearity of VMs.
3. virt-optimizer: virt-optimizer provides NPU passthrough VM large model inference performance bottleneck analysis and performance optimization suggestions.

## Instruction

1. ubs-virt-enpu: [ubs-virt-enpu Instruction](./ubs-virt-enpu/README.md)
2. virt-awaresched: [virt-awaresched Instruction](./virt-awaresched/README.md)
3. virt-optimizer: [virt-optimizer Instruction](./virt-optimizer/README.md)
