# Test target for mx-usb-unmounter
# Tests are built and run manually: cmake -DBUILD_TESTS=ON -B build && cmake --build build && ctest --test-dir build

find_package(Qt6 REQUIRED COMPONENTS Test)

add_executable(test_mx_usb_unmounter
    test_main.cpp
)

target_link_libraries(test_mx_usb_unmounter PRIVATE
    Qt6::Test
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::DBus
)

add_test(NAME test_mx_usb_unmounter COMMAND test_mx_usb_unmounter)
set_tests_properties(test_mx_usb_unmounter PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)
