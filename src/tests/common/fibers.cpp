// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <atomic>
#include <cstdlib>
#include <functional>
#include <memory>
#include <thread>
#include <unordered_map>
#include <vector>

#include <catch2/catch.hpp>
#include <math.h>
#include "common/common_types.h"
#include "common/fiber.h"
#include "common/spin_lock.h"

namespace Common {

class TestControl1 {
public:
    TestControl1() = default;

    void DoWork();

    void ExecuteThread(u32 id);

    std::unordered_map<std::thread::id, u32> ids;
    std::vector<std::shared_ptr<Common::Fiber>> thread_fibers;
    std::vector<std::shared_ptr<Common::Fiber>> work_fibers;
    std::vector<u32> items;
    std::vector<u32> results;
};

static void WorkControl1(void* control) {
    auto* test_control = static_cast<TestControl1*>(control);
    test_control->DoWork();
}

void TestControl1::DoWork() {
    std::thread::id this_id = std::this_thread::get_id();
    u32 id = ids[this_id];
    u32 value = items[id];
    for (u32 i = 0; i < id; i++) {
        value++;
    }
    results[id] = value;
    Fiber::YieldTo(work_fibers[id], thread_fibers[id]);
}

void TestControl1::ExecuteThread(u32 id) {
    std::thread::id this_id = std::this_thread::get_id();
    ids[this_id] = id;
    auto thread_fiber = Fiber::ThreadToFiber();
    thread_fibers[id] = thread_fiber;
    work_fibers[id] = std::make_shared<Fiber>(std::function<void(void*)>{WorkControl1}, this);
    items[id] = rand() % 256;
    Fiber::YieldTo(thread_fibers[id], work_fibers[id]);
    thread_fibers[id]->Exit();
}

static void ThreadStart1(u32 id, TestControl1& test_control) {
    test_control.ExecuteThread(id);
}

/** This test checks for fiber setup configuration and validates that fibers are
 *  doing all the work required.
 */
TEST_CASE("Fibers::Setup", "[common]") {
    constexpr std::size_t num_threads = 7;
    TestControl1 test_control{};
    test_control.thread_fibers.resize(num_threads);
    test_control.work_fibers.resize(num_threads);
    test_control.items.resize(num_threads, 0);
    test_control.results.resize(num_threads, 0);
    std::vector<std::thread> threads;
    for (u32 i = 0; i < num_threads; i++) {
        threads.emplace_back(ThreadStart1, i, std::ref(test_control));
    }
    for (u32 i = 0; i < num_threads; i++) {
        threads[i].join();
    }
    for (u32 i = 0; i < num_threads; i++) {
        REQUIRE(test_control.items[i] + i == test_control.results[i]);
    }
}

class TestControl2 {
public:
    TestControl2() = default;

    void DoWork1() {
        trap2 = false;
        while (trap.load())
            ;
        for (u32 i = 0; i < 12000; i++) {
            value1 += i;
        }
        Fiber::YieldTo(fiber1, fiber3);
        std::thread::id this_id = std::this_thread::get_id();
        u32 id = ids[this_id];
        assert1 = id == 1;
        value2 += 5000;
        Fiber::YieldTo(fiber1, thread_fibers[id]);
    }

    void DoWork2() {
        while (trap2.load())
            ;
        value2 = 2000;
        trap = false;
        Fiber::YieldTo(fiber2, fiber1);
        assert3 = false;
    }

    void DoWork3() {
        std::thread::id this_id = std::this_thread::get_id();
        u32 id = ids[this_id];
        assert2 = id == 0;
        value1 += 1000;
        Fiber::YieldTo(fiber3, thread_fibers[id]);
    }

    void ExecuteThread(u32 id);

    void CallFiber1() {
        std::thread::id this_id = std::this_thread::get_id();
        u32 id = ids[this_id];
        Fiber::YieldTo(thread_fibers[id], fiber1);
    }

    void CallFiber2() {
        std::thread::id this_id = std::this_thread::get_id();
        u32 id = ids[this_id];
        Fiber::YieldTo(thread_fibers[id], fiber2);
    }

    void Exit();

    bool assert1{};
    bool assert2{};
    bool assert3{true};
    u32 value1{};
    u32 value2{};
    std::atomic<bool> trap{true};
    std::atomic<bool> trap2{true};
    std::unordered_map<std::thread::id, u32> ids;
    std::vector<std::shared_ptr<Common::Fiber>> thread_fibers;
    std::shared_ptr<Common::Fiber> fiber1;
    std::shared_ptr<Common::Fiber> fiber2;
    std::shared_ptr<Common::Fiber> fiber3;
};

static void WorkControl2_1(void* control) {
    auto* test_control = static_cast<TestControl2*>(control);
    test_control->DoWork1();
}

static void WorkControl2_2(void* control) {
    auto* test_control = static_cast<TestControl2*>(control);
    test_control->DoWork2();
}

static void WorkControl2_3(void* control) {
    auto* test_control = static_cast<TestControl2*>(control);
    test_control->DoWork3();
}

void TestControl2::ExecuteThread(u32 id) {
    std::thread::id this_id = std::this_thread::get_id();
    ids[this_id] = id;
    auto thread_fiber = Fiber::ThreadToFiber();
    thread_fibers[id] = thread_fiber;
}

void TestControl2::Exit() {
    std::thread::id this_id = std::this_thread::get_id();
    u32 id = ids[this_id];
    thread_fibers[id]->Exit();
}

static void ThreadStart2_1(u32 id, TestControl2& test_control) {
    test_control.ExecuteThread(id);
    test_control.CallFiber1();
    test_control.Exit();
}

static void ThreadStart2_2(u32 id, TestControl2& test_control) {
    test_control.ExecuteThread(id);
    test_control.CallFiber2();
    test_control.Exit();
}

/** This test checks for fiber thread exchange configuration and validates that fibers are
 *  that a fiber has been succesfully transfered from one thread to another and that the TLS
 *  region of the thread is kept while changing fibers.
 */
TEST_CASE("Fibers::InterExchange", "[common]") {
    TestControl2 test_control{};
    test_control.thread_fibers.resize(2);
    test_control.fiber1 =
        std::make_shared<Fiber>(std::function<void(void*)>{WorkControl2_1}, &test_control);
    test_control.fiber2 =
        std::make_shared<Fiber>(std::function<void(void*)>{WorkControl2_2}, &test_control);
    test_control.fiber3 =
        std::make_shared<Fiber>(std::function<void(void*)>{WorkControl2_3}, &test_control);
    std::thread thread1(ThreadStart2_1, 0, std::ref(test_control));
    std::thread thread2(ThreadStart2_2, 1, std::ref(test_control));
    thread1.join();
    thread2.join();
    REQUIRE(test_control.assert1);
    REQUIRE(test_control.assert2);
    REQUIRE(test_control.assert3);
    REQUIRE(test_control.value2 == 7000);
    u32 cal_value = 0;
    for (u32 i = 0; i < 12000; i++) {
        cal_value += i;
    }
    cal_value += 1000;
    REQUIRE(test_control.value1 == cal_value);
}

class TestControl3 {
public:
    TestControl3() = default;

    void DoWork1() {
        value1 += 1;
        Fiber::YieldTo(fiber1, fiber2);
        std::thread::id this_id = std::this_thread::get_id();
        u32 id = ids[this_id];
        value3 += 1;
        Fiber::YieldTo(fiber1, thread_fibers[id]);
    }

    void DoWork2() {
        value2 += 1;
        std::thread::id this_id = std::this_thread::get_id();
        u32 id = ids[this_id];
        Fiber::YieldTo(fiber2, thread_fibers[id]);
    }

    void ExecuteThread(u32 id);

    void CallFiber1() {
        std::thread::id this_id = std::this_thread::get_id();
        u32 id = ids[this_id];
        Fiber::YieldTo(thread_fibers[id], fiber1);
    }

    void Exit();

    u32 value1{};
    u32 value2{};
    u32 value3{};
    std::unordered_map<std::thread::id, u32> ids;
    std::vector<std::shared_ptr<Common::Fiber>> thread_fibers;
    std::shared_ptr<Common::Fiber> fiber1;
    std::shared_ptr<Common::Fiber> fiber2;
};

static void WorkControl3_1(void* control) {
    auto* test_control = static_cast<TestControl3*>(control);
    test_control->DoWork1();
}

static void WorkControl3_2(void* control) {
    auto* test_control = static_cast<TestControl3*>(control);
    test_control->DoWork2();
}

void TestControl3::ExecuteThread(u32 id) {
    std::thread::id this_id = std::this_thread::get_id();
    ids[this_id] = id;
    auto thread_fiber = Fiber::ThreadToFiber();
    thread_fibers[id] = thread_fiber;
}

void TestControl3::Exit() {
    std::thread::id this_id = std::this_thread::get_id();
    u32 id = ids[this_id];
    thread_fibers[id]->Exit();
}

static void ThreadStart3(u32 id, TestControl3& test_control) {
    test_control.ExecuteThread(id);
    test_control.CallFiber1();
    test_control.Exit();
}

/** This test checks for one two threads racing for starting the same fiber.
 *  It checks execution occured in an ordered manner and by no time there were
 *  two contexts at the same time.
 */
TEST_CASE("Fibers::StartRace", "[common]") {
    TestControl3 test_control{};
    test_control.thread_fibers.resize(2);
    test_control.fiber1 =
        std::make_shared<Fiber>(std::function<void(void*)>{WorkControl3_1}, &test_control);
    test_control.fiber2 =
        std::make_shared<Fiber>(std::function<void(void*)>{WorkControl3_2}, &test_control);
    std::thread thread1(ThreadStart3, 0, std::ref(test_control));
    std::thread thread2(ThreadStart3, 1, std::ref(test_control));
    thread1.join();
    thread2.join();
    REQUIRE(test_control.value1 == 1);
    REQUIRE(test_control.value2 == 1);
    REQUIRE(test_control.value3 == 1);
}

class TestControl4;

static void WorkControl4(void* control);

class TestControl4 {
public:
    TestControl4() {
        fiber1 = std::make_shared<Fiber>(std::function<void(void*)>{WorkControl4}, this);
        goal_reached = false;
        rewinded = false;
    }

    void Execute() {
        thread_fiber = Fiber::ThreadToFiber();
        Fiber::YieldTo(thread_fiber, fiber1);
        thread_fiber->Exit();
    }

    void DoWork() {
        fiber1->SetRewindPoint(std::function<void(void*)>{WorkControl4}, this);
        if (rewinded) {
            goal_reached = true;
            Fiber::YieldTo(fiber1, thread_fiber);
        }
        rewinded = true;
        fiber1->Rewind();
    }

    std::shared_ptr<Common::Fiber> fiber1;
    std::shared_ptr<Common::Fiber> thread_fiber;
    bool goal_reached;
    bool rewinded;
};

static void WorkControl4(void* control) {
    auto* test_control = static_cast<TestControl4*>(control);
    test_control->DoWork();
}

TEST_CASE("Fibers::Rewind", "[common]") {
    TestControl4 test_control{};
    test_control.Execute();
    REQUIRE(test_control.goal_reached);
    REQUIRE(test_control.rewinded);
}

} // namespace Common
