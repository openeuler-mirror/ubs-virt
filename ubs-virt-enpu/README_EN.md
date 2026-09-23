# ubs-virt-enpu

<p> English | <a href="README.md">简体中文</a> </p>

## Overview

ubs-virt-enpu supports hardware- and software-based dynamic allocation, multi-task parallel execution, and resource isolation, significantly improving AI hardware resource utilization and performance.

## Components

vCANN-RT: ubs-virt-enpu enables soft allocation of NPU compute capabilities, and supports control over compute capabilities and graphics memory based on the quota on these resources.

enpu-manager: the management process of ubs-virt-enpu for NPU graphics memory swap (oversubscription) services, which works with vCANN-RT to swap cold models out/in and improve graphics memory utilization.

## Instruction

vCANN-RT: [vCANN-RT Instruction](./vcann-rt/README.md)

enpu-manager: [enpu-manager Instruction](./enpu-manager/README.md)
