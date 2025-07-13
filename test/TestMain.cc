#include "TestMain.h"


namespace test {

extern void SetupEventListener();
extern void TestPaginationForm();

void RunTestMain() {
    SetupEventListener();
    TestPaginationForm();
}


} // namespace test