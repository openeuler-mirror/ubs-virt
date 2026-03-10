# 部署指导

---

## 拷贝软件包到环境

使用文件传输软件, 将`virt-awaresched-*.*.*-1.aarch64.rpm`软件包上传至服务器。

---

## 安装rpm包

- 首次安装

    ```shell
    rpm -ivh virt-awaresched-*.rpm
    ```

- 覆盖安装

    ```shell
    rpm -ivh --force virt-awaresched-*.rpm
    ```

---

## 确认服务部署状态

```shell
systemctl status vas-daemon
```

出现如下返回信息，说明服务已正常启动。

![服务正常启动](images/服务正常启动.png "服务正常启动示例")

如果服务启动失败, 请查看服务日志(默认路径: `/var/log/vas/vas.log`), 确认启动失败原因.

---

## 修改启动参数

- 详细请参考[配置说明](../config/CONFIG.md)
- 重新加载配置

    ```shell
    systemctl daemon-reload
    ```

- 重启服务

    ```shell
    systemctl restart vas-daemon
    ```

---
