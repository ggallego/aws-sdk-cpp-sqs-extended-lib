cmake_minimum_required(VERSION 2.6)
project(aws-cpp-sdk-sqs-extended-lib-integration-tests)

file(GLOB AWS_SQS_EXTENDED_LIB_SRC
  "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

file(GLOB AWS_SQS_EXTENDED_LIB_INTEGRATION_TESTS_SRC
  ${AWS_SQS_EXTENDED_LIB_SRC}
)

find_package(aws-sdk-cpp)

if(MSVC AND BUILD_SHARED_LIBS)
    add_definitions(-DGTEST_LINKED_AS_SHARED_LIBRARY=1)
endif()

enable_testing()

if(PLATFORM_ANDROID AND BUILD_SHARED_LIBS)
  add_library(runSQSExtendedLibIntegrationTests ${LIBTYPE} ${AWS_SQS_EXTENDED_LIB_INTEGRATION_TESTS_SRC})
else()
  add_executable(runSQSExtendedLibIntegrationTests ${AWS_SQS_EXTENDED_LIB_INTEGRATION_TESTS_SRC})
endif()

target_link_libraries(runSQSExtendedLibIntegrationTests aws-cpp-sdk-core aws-cpp-sdk-s3 aws-cpp-sdk-sqs aws-cpp-sdk-sqs-extended-lib aws-cpp-sdk-access-management aws-cpp-sdk-iam aws-cpp-sdk-cognito-identity testing-resources)
copyDlls(runSQSExtendedLibIntegrationTests aws-cpp-sdk-core aws-cpp-sdk-s3 aws-cpp-sdk-sqs aws-cpp-sdk-sqs-extended-lib aws-cpp-sdk-access-management aws-cpp-sdk-iam aws-cpp-sdk-cognito-identity testing-resources)
