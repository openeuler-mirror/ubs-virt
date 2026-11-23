/* Copyright (c) Huawei Technologies Co., Ltd. 2026. All rights reserved.
 * Licensed under Mulan PSL v2.
 */
#include "launch_budget.h"
#include <gtest/gtest.h>
#include "npu_manager.h"

TEST(LaunchBudgetTest, CalculationBoundsAndFallback)
{
    bool fallback = true;
    EXPECT_EQ(launch_budget_calculate(0, 0, &fallback), 0U);
    EXPECT_FALSE(fallback);
    EXPECT_EQ(launch_budget_calculate(1, 1000, nullptr), LAUNCH_BUDGET_MIN);
    EXPECT_EQ(launch_budget_calculate(UINT64_MAX, 1, nullptr), LAUNCH_BUDGET_MAX);
    EXPECT_GE(launch_budget_calculate(100000, -1, &fallback), LAUNCH_BUDGET_MIN);
    EXPECT_TRUE(fallback);
}

TEST(LaunchBudgetTest, TicketCompletionAndSmoothedFeedback)
{
    launch_budget_bind(nullptr, 0);
    launch_budget_prepare_turn(1, 100000, SCHED_POLICY_FIXED_SHARE, 1000);
    launch_budget_ticket_t ticket{};
    EXPECT_FALSE(launch_budget_try_admit_weighted(1, nullptr));
    EXPECT_TRUE(launch_budget_try_admit_weighted(100, &ticket));
    EXPECT_FALSE(launch_budget_should_yield(1));
    launch_budget_complete(&ticket);
    launch_budget_complete(&ticket);
    EXPECT_TRUE(launch_budget_should_yield(1));
    EXPECT_FALSE(launch_budget_try_admit_weighted(1, &ticket));
    launch_budget_apply_feedback(1, 50000, true);
    launch_budget_snapshot_t snapshot{};
    launch_budget_get_snapshot(&snapshot);
    EXPECT_EQ(snapshot.in_flight_launches, 0U);
    EXPECT_EQ(snapshot.alpha_per_mille, 1250U);
    launch_budget_reset();
}

TEST(LaunchBudgetTest, FallbackAndDisabledPolicy)
{
    launch_budget_bind(nullptr, 0);
    launch_budget_prepare_turn(3, 100000, SCHED_POLICY_ELASTIC, 0);
    launch_budget_apply_feedback(3, 1, true);
    launch_budget_snapshot_t snapshot{};
    launch_budget_get_snapshot(&snapshot);
    EXPECT_TRUE(snapshot.duration_fallback);
    EXPECT_EQ(snapshot.alpha_per_mille, LAUNCH_BUDGET_ALPHA_SCALE);
    launch_budget_prepare_turn(4, 100000, SCHED_POLICY_BEST_EFFORT, 1000);
    EXPECT_TRUE(launch_budget_try_admit_weighted(1, nullptr));
    EXPECT_FALSE(launch_budget_should_yield(4));
    launch_budget_get_snapshot(&snapshot);
    EXPECT_FALSE(snapshot.enabled);
    launch_budget_complete(nullptr);
    launch_budget_get_snapshot(nullptr);
    launch_budget_reset();
}

TEST(LaunchBudgetTest, SharedDrainReportsAndRefundAreIdempotent)
{
    vnpu_time_slice_sched_t sched{};
    ASSERT_EQ(pthread_mutex_init(&sched.vnpu_schedule_mutex[0], nullptr), 0);
    vnpu_launch_budget_init(&sched.launch_budget[0]);
    sched.owner.store(0);
    sched.vnpu_schedule_turn[0].store(5);
    launch_budget_bind(&sched, 0);
    launch_budget_prepare_turn(5, 1000000000, SCHED_POLICY_FIXED_SHARE, 10000000);
    auto &state = sched.launch_budget[0];
    EXPECT_TRUE(launch_budget_join_turn(state.start_ns.load()));
    EXPECT_FALSE(launch_budget_join_turn(state.start_ns.load()));
    EXPECT_TRUE(launch_budget_gate_open());
    launch_budget_ticket_t ticket{};
    EXPECT_TRUE(launch_budget_try_admit_weighted(100, &ticket));
    EXPECT_FALSE(launch_budget_should_yield(5));
    launch_budget_complete(&ticket);
    EXPECT_TRUE(launch_budget_should_yield(5));
    const auto epoch = state.epoch.load();
    launch_budget_close_turn(epoch, state.start_ns.load() + 400000000);
    EXPECT_FALSE(launch_budget_try_admit_weighted(1, &ticket));
    EXPECT_TRUE(launch_budget_report_drain(epoch, true));
    EXPECT_FALSE(launch_budget_report_drain(epoch, true));
    EXPECT_EQ(sched.vnpu_sync_expected[0].load(), 1);
    EXPECT_EQ(sched.vnpu_sync_completed[0].load(), 1);
    EXPECT_EQ(launch_budget_refund_turn(epoch, true), 600000000U);
    EXPECT_EQ(launch_budget_refund_turn(epoch, true), 0U);
    EXPECT_EQ(state.alpha_per_mille.load(), 1375U);
    launch_budget_bind(nullptr, 0);
    EXPECT_EQ(pthread_mutex_destroy(&sched.vnpu_schedule_mutex[0]), 0);
}
