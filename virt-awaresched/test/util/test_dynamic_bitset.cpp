/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
 *
 * VSched is licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *      http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */
#include "test_dynamic_bitset.h"

#include <set>
#include <string>
#include <vector>

#include "dynamic_bitset.h"

using namespace vas::common;

namespace vas::ut::util {
    void TestDynamicBitset::SetUp()
    {
        testing::Test::SetUp();
    }

    void TestDynamicBitset::TearDown()
    {
        testing::Test::TearDown();
    }

    TEST_F(TestDynamicBitset, testDynamicBitsetSet) {
        DynamicBitset bitSet(5, false);
        Bitset::DynamicBitsetSet(bitSet, 1, 3);

        EXPECT_FALSE(bitSet[0]);
        EXPECT_TRUE(bitSet[1]);
        EXPECT_TRUE(bitSet[2]);
        EXPECT_TRUE(bitSet[3]);
        EXPECT_FALSE(bitSet[4]);
    }

    TEST_F(TestDynamicBitset, testDynamicBitsetClear) {
        DynamicBitset bitSet(5, true);
        Bitset::DynamicBitsetClear(bitSet, 1, 3);

        EXPECT_TRUE(bitSet[0]);
        EXPECT_FALSE(bitSet[1]);
        EXPECT_FALSE(bitSet[2]);
        EXPECT_FALSE(bitSet[3]);
        EXPECT_TRUE(bitSet[4]);
    }

    TEST_F(TestDynamicBitset, testDynamicBitsetNot) {
        DynamicBitset src(3);
        src[0] = true;
        src[1] = false;
        src[2] = true;
        DynamicBitset dst = Bitset::DynamicBitsetNot(src);

        EXPECT_FALSE(dst[0]);
        EXPECT_TRUE(dst[1]);
        EXPECT_FALSE(dst[2]);
    }

    TEST_F(TestDynamicBitset, testDynamicBitsetOr) {
        DynamicBitset src1(3);
        src1[0] = true;
        src1[1] = false;
        src1[2] = true;
        DynamicBitset src2(3);
        src2[0] = false;
        src2[1] = true;
        src2[2] = false;
        Bitset::DynamicBitsetOr(src1, src2);

        EXPECT_TRUE(src1[0]);
        EXPECT_TRUE(src1[1]);
        EXPECT_TRUE(src1[2]);
    }

    TEST_F(TestDynamicBitset, testDynamicBitsetAnd) {
        DynamicBitset src1(3);
        src1[0] = true;
        src1[1] = false;
        src1[2] = true;
        DynamicBitset src2(3);
        src2[0] = false;
        src2[1] = true;
        src2[2] = true;
        Bitset::DynamicBitsetAnd(src1, src2);

        EXPECT_FALSE(src1[0]);
        EXPECT_FALSE(src1[1]);
        EXPECT_TRUE(src1[2]);
    }

    TEST_F(TestDynamicBitset, testIsDynamicBitsetCross) {
        DynamicBitset first(3);
        first[0] = true;
        first[1] = false;
        first[2] = true;
        DynamicBitset second(3);
        second[0] = false;
        second[1] = true;
        second[2] = true;
        EXPECT_TRUE(Bitset::IsDynamicBitsetCross(first, second));
    }

    TEST_F(TestDynamicBitset, testGenDynamicBitSetByArea) {
        DynamicBitset bitSet = Bitset::GenDynamicBitSetByArea(5, 1, 3);
        EXPECT_EQ(bitSet.size(), 5);
        EXPECT_FALSE(bitSet[0]);
        EXPECT_TRUE(bitSet[1]);
        EXPECT_TRUE(bitSet[2]);
        EXPECT_TRUE(bitSet[3]);
        EXPECT_FALSE(bitSet[4]);
    }

    TEST_F(TestDynamicBitset, testFindFirstIdlePos) {
        DynamicBitset bitSet(5);
        bitSet[0] = true;
        bitSet[1] = false;
        bitSet[2] = false;
        bitSet[3] = true;
        bitSet[4] = false;
        EXPECT_EQ(Bitset::FindFirstIdlePos(bitSet, 0, 2), 1);
    }

    TEST_F(TestDynamicBitset, testGetDynamicBitsetAreaSet) {
        DynamicBitset bitSet(5);
        bitSet[0] = true;
        bitSet[2] = true;
        bitSet[4] = true;
        std::set<uint16_t> areaSet = Bitset::GetDynamicBitsetAreaSet(bitSet);
        EXPECT_EQ(areaSet.size(), 3);
        EXPECT_EQ(areaSet.count(0), 1);
        EXPECT_EQ(areaSet.count(2), 1);
        EXPECT_EQ(areaSet.count(4), 1);
    }

    TEST_F(TestDynamicBitset, testGenDynamicBitsetByCpuSet) {
        std::set<uint16_t> cpuSet = {0, 2, 4};
        DynamicBitset bitSet = Bitset::GenDynamicBitsetByCpuSet(5, cpuSet);
        EXPECT_EQ(bitSet.size(), 5);
        EXPECT_TRUE(bitSet[0]);
        EXPECT_FALSE(bitSet[1]);
        EXPECT_TRUE(bitSet[2]);
        EXPECT_FALSE(bitSet[3]);
        EXPECT_TRUE(bitSet[4]);
    }

    TEST_F(TestDynamicBitset, testCpuMaskToDynamicBitset) {
        unsigned char cpuMask[] = {0b1001};
        DynamicBitset bitSet;
        Bitset::CpuMaskToDynamicBitset(cpuMask, 1, bitSet);
        EXPECT_EQ(bitSet.size(), 8);
        EXPECT_TRUE(bitSet[0]);
        EXPECT_TRUE(bitSet[3]);
    }

    TEST_F(TestDynamicBitset, testDynamicBitsetToStr) {
        DynamicBitset bitSet(5);
        bitSet[0] = true;
        bitSet[2] = true;
        bitSet[4] = true;
        std::string str = Bitset::DynamicBitsetToStr(bitSet);
        EXPECT_EQ(str, "10101");
    }

    TEST_F(TestDynamicBitset, testGetCpuSetFromDynamicBitset) {
        DynamicBitset bitSet(5);
        bitSet[0] = true;
        bitSet[2] = true;
        bitSet[4] = true;
        std::string cpuSet = Bitset::GetCpuSetFromDynamicBitset(bitSet);
        EXPECT_EQ(cpuSet, "0 2 4");
    }

    TEST_F(TestDynamicBitset, testDynamicBitsetSetArea) {
        DynamicBitset bitSet(5, false);
        Bitset::DynamicBitsetSetArea(bitSet, 1, 3, true);
        EXPECT_FALSE(bitSet[0]);
        EXPECT_TRUE(bitSet[1]);
        EXPECT_TRUE(bitSet[2]);
        EXPECT_TRUE(bitSet[3]);
        EXPECT_FALSE(bitSet[4]);
    }
} // namespace vas::ut::common