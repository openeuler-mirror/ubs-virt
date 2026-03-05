/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026-2026. All rights reserved.
 * ubs-virt-enpu is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

#include <gtest/gtest.h>
#include <mockcpp/mockcpp.hpp>
#include <sys/file.h>
#include <runtime/rt.h>
#include "securec.h"
#include "runtime_stub.h"
#include "log.h"
#include "npu_manager.h"
#include "mem_limiter.h"

class KernelTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        std::cout<<"Kernel test start"<<std::endl;
    }

    static void TearDownTestCase()
    {
        std::cout<<"Kernel test end"<<std::endl;
    }

    void SetUp()
    {
        (void)sprintf_s(g_log_config.log_dir, sizeof(g_log_config.log_dir), "%s", "../__build/log/enpu/");
        fd_ = open(stub_lock_path(), O_CREAT | O_RDONLY, 0755); // ut中的memctl.lock文件,设置为755权限
        MOCKER(lock_path).stubs().will(invoke(stub_lock_path));
        MOCKER(enpu_load_config).stubs().will(invoke(stub_enpu_load_config));
        enpu_global_init();
        MOCKER(load_rt_libraries).stubs().will(invoke(stub_load_rt_libraries));
    }

    void TearDown()
    {
        GlobalMockObject::verify();
        GlobalMockObject::reset();
        close(fd_);
        fd_ = -1;
    }
private:
    int fd_ = -1;
};

TEST_F(KernelTest, rtKernelLaunchTest)
{
    void *stubFunc = nullptr;
    uint32_t blockDim = 0;
    void *args = nullptr;
    uint32_t argsSize = 0;
    rtSmDesc_t *smDesc = nullptr;
    rtStream_t stm = nullptr;
    rtError_t ret = rtKernelLaunch(stubFunc, blockDim, args, argsSize, smDesc, stm);
    EXPECT_EQ(ret, RT_ERROR_NONE);
}

TEST_F(KernelTest, rtKernelLaunchWithHandleTest)
{
    void *hdl = nullptr;
    uint64_t tilingKey = 0;
    uint32_t blockDim = 0;
    rtArgsEx_t *argsInfo = nullptr;
    rtSmDesc_t *smDesc = nullptr;
    rtStream_t stm = nullptr;
    void *kernelInfo = nullptr;
    rtError_t ret = rtKernelLaunchWithHandle(hdl, tilingKey, blockDim, argsInfo, smDesc, stm, kernelInfo);
    EXPECT_EQ(ret, RT_ERROR_NONE);
}

TEST_F(KernelTest, rtKernelLaunchWithHandleV2Test)
{
    void *hdl = nullptr;
    uint64_t tilingKey = 0;
    uint32_t blockDim = 0;
    rtArgsEx_t *argsInfo = nullptr;
    rtSmDesc_t *smDesc = nullptr;
    rtStream_t stm = nullptr;
    rtTaskCfgInfo_t *cfgInfo = nullptr;
    rtError_t ret = rtKernelLaunchWithHandleV2(hdl, tilingKey, blockDim, argsInfo, smDesc, stm, cfgInfo);
    EXPECT_EQ(ret, RT_ERROR_NONE);
}

TEST_F(KernelTest, rtKernelLaunchWithFlagTest)
{
    void *stubFunc = nullptr;
    uint32_t blockDim = 0;
    rtArgsEx_t *argsInfo = nullptr;
    rtSmDesc_t *smDesc = nullptr;
    rtStream_t stm = nullptr;
    uint32_t flags = 0;
    rtError_t ret = rtKernelLaunchWithFlag(stubFunc, blockDim, argsInfo, smDesc, stm, flags);
    EXPECT_EQ(ret, RT_ERROR_NONE);
}

TEST_F(KernelTest, rtKernelLaunchWithFlagV2Test)
{
    void *stubFunc = nullptr;
    uint32_t blockDim = 0;
    rtArgsEx_t *argsInfo = nullptr;
    rtSmDesc_t *smDesc = nullptr;
    rtStream_t stm = nullptr;
    uint32_t flags = 0;
    rtTaskCfgInfo_t *cfgInfo = nullptr;
    rtError_t ret = rtKernelLaunchWithFlagV2(stubFunc, blockDim, argsInfo, smDesc, stm, flags, cfgInfo);
    EXPECT_EQ(ret, RT_ERROR_NONE);
}

TEST_F(KernelTest, rtKernelLaunchExTest)
{
    void *args = nullptr;
    uint32_t argsSize = 0;
    uint32_t flags = 0;
    rtStream_t stm = nullptr;
    rtError_t ret = rtKernelLaunchEx(args, argsSize, flags, stm);
    EXPECT_EQ(ret, RT_ERROR_NONE);
}

TEST_F(KernelTest, rtKernelLaunchFwkTest)
{
    char_t *opName = nullptr;
    void *args = nullptr;
    uint32_t argsSize = 0;
    uint32_t flags = 0;
    rtStream_t stm = nullptr;
    rtError_t ret = rtKernelLaunchFwk(opName, args, argsSize, flags, stm);
    EXPECT_EQ(ret, RT_ERROR_NONE);
}

TEST_F(KernelTest, rtCpuKernelLaunchTest)
{
    void *soName = nullptr;
    void *kernelName = nullptr;
    uint32_t blockDim = 0;
    void *args = nullptr;
    uint32_t argsSize = 0;
    rtSmDesc_t *smDesc = nullptr;
    rtStream_t stm = nullptr;
    rtError_t ret = rtCpuKernelLaunch(soName, kernelName, blockDim, args, argsSize, smDesc, stm);
    EXPECT_EQ(ret, RT_ERROR_NONE);
}

TEST_F(KernelTest, rtCpuKernelLaunchWithFlagTest)
{
    void *soName = nullptr;
    void *kernelName = nullptr;
    uint32_t blockDim = 0;
    rtArgsEx_t *argsInfo = nullptr;
    rtSmDesc_t *smDesc = nullptr;
    rtStream_t stm = nullptr;
    uint32_t flags = 0;
    rtError_t ret = rtCpuKernelLaunchWithFlag(soName, kernelName, blockDim, argsInfo, smDesc, stm, flags);
    EXPECT_EQ(ret, RT_ERROR_NONE);
}

TEST_F(KernelTest, rtAicpuKernelLaunchWithFlagTest)
{
    rtKernelLaunchNames_t *launchNames = nullptr;
    uint32_t blockDim = 0;
    rtArgsEx_t *argsInfo = nullptr;
    rtSmDesc_t *smDesc = nullptr;
    rtStream_t stm = nullptr;
    uint32_t flags = 0;
    rtError_t ret = rtAicpuKernelLaunchWithFlag(launchNames, blockDim, argsInfo, smDesc, stm, flags);
    EXPECT_EQ(ret, RT_ERROR_NONE);
}

TEST_F(KernelTest, rtAicpuKernelLaunchExWithArgsTest)
{
    uint32_t kernelType = 0;
    char_t *opName = nullptr;
    uint32_t blockDim = 0;
    rtArgsEx_t *argsInfo = nullptr;
    rtSmDesc_t *smDesc = nullptr;
    rtStream_t stm = nullptr;
    uint32_t flags = 0;
    rtError_t ret = rtAicpuKernelLaunchExWithArgs(kernelType, opName, blockDim, argsInfo, smDesc, stm, flags);
    EXPECT_EQ(ret, RT_ERROR_NONE);
}

TEST_F(KernelTest, rtLaunchKernelByFuncHandleTest)
{
    rtFuncHandle funcHandle = nullptr;
    uint32_t blockDim = 0;
    rtLaunchArgsHandle argsHandle = nullptr;
    rtStream_t stm = nullptr;
    rtError_t ret = rtLaunchKernelByFuncHandle(funcHandle, blockDim, argsHandle, stm);
    EXPECT_EQ(ret, RT_ERROR_NONE);
}

TEST_F(KernelTest, rtLaunchKernelByFuncHandleV2Test)
{
    rtFuncHandle funcHandle = nullptr;
    uint32_t blockDim = 0;
    rtLaunchArgsHandle argsHandle = nullptr;
    rtStream_t stm = nullptr;
    rtTaskCfgInfo_t *cfgInfo = nullptr;
    rtError_t ret = rtLaunchKernelByFuncHandleV2(funcHandle, blockDim, argsHandle, stm, cfgInfo);
    EXPECT_EQ(ret, RT_ERROR_NONE);
}

TEST_F(KernelTest, rtLaunchKernelByFuncHandleV3Test)
{
    rtFuncHandle funcHandle = nullptr;
    uint32_t blockDim = 0;
    rtArgsEx_t *const argsInfo = nullptr;
    rtStream_t stm = nullptr;
    rtTaskCfgInfo_t *cfgInfo = nullptr;
    rtError_t ret = rtLaunchKernelByFuncHandleV3(funcHandle, blockDim, argsInfo, stm, cfgInfo);
    EXPECT_EQ(ret, RT_ERROR_NONE);
}

TEST_F(KernelTest, rtVectorCoreKernelLaunchWithHandleTest)
{
    void *hdl = nullptr;
    uint64_t tilingKey = 0;
    uint32_t blockDim = 0;
    rtArgsEx_t *argsInfo = nullptr;
    rtSmDesc_t *smDesc = nullptr;
    rtStream_t stm = nullptr;
    rtTaskCfgInfo_t *cfgInfo = nullptr;
    rtError_t ret = rtVectorCoreKernelLaunchWithHandle(hdl, tilingKey, blockDim, argsInfo, smDesc, stm, cfgInfo);
    EXPECT_EQ(ret, RT_ERROR_NONE);
}

TEST_F(KernelTest, rtVectorCoreKernelLaunchTest)
{
    void *stubFunc = nullptr;
    uint32_t blockDim = 0;
    rtArgsEx_t *argsInfo = nullptr;
    rtSmDesc_t *smDesc = nullptr;
    rtStream_t stm = nullptr;
    uint32_t flags = 0;
    rtTaskCfgInfo_t *cfgInfo = nullptr;
    rtError_t ret = rtVectorCoreKernelLaunch(stubFunc, blockDim, argsInfo, smDesc, stm, flags, cfgInfo);
    EXPECT_EQ(ret, RT_ERROR_NONE);
}
