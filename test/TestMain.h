#pragma once

namespace test {


struct TestMain {
    TestMain() = delete;

    static void setup() {
        _setupLandEventTest();
        _setupPaginationFormTest();
        _setupChooseLandAdvancedUtilGUITest();
    }

    static void _setupLandEventTest();
    static void _setupPaginationFormTest();
    static void _setupChooseLandAdvancedUtilGUITest();
};


} // namespace test