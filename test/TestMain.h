#pragma once

namespace test {


struct TestMain {
    TestMain() = delete;

    static void setup() {
        _setupLandEventTest();
        _setupPaginationFormTest();
    }

    static void _setupLandEventTest();
    static void _setupPaginationFormTest();
};


} // namespace test