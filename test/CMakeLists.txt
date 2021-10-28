# Create a test runner
add_executable(ImNBTTestRunner "src/main.cpp")

# Link the test runner to the library
target_link_libraries(ImNBTTestRunner ImNBT)
