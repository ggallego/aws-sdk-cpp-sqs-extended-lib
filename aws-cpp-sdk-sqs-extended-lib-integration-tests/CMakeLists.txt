cmake_minimum_required(VERSION 2.6)
project(aws-cpp-sdk-sqs-extended-lib-integration-tests)

####################################### gtest stuff

include(Ctest)

if (CMAKE_VERSION VERSION_LESS 3.2)
  set(UPDATE_DISCONNECTED_IF_AVAILABLE "")
else()
  set(UPDATE_DISCONNECTED_IF_AVAILABLE "UPDATE_DISCONNECTED 1")
endif()

include(DownloadProject.cmake)

download_project(PROJ                googletest
                 GIT_REPOSITORY      https://github.com/google/googletest.git
                 GIT_TAG             master
                 ${UPDATE_DISCONNECTED_IF_AVAILABLE}
)

add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR})

if (CMAKE_VERSION VERSION_LESS 2.8.11)
    include_directories("${gtest_SOURCE_DIR}/include"
                        "${gmock_SOURCE_DIR}/include")
endif()

####################################### build project

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

target_link_libraries(runSQSExtendedLibIntegrationTests aws-cpp-sdk-core aws-cpp-sdk-s3 aws-cpp-sdk-sqs aws-cpp-sdk-sqs-extended-lib aws-cpp-sdk-access-management aws-cpp-sdk-iam aws-cpp-sdk-cognito-identity gtest gtest_main)
copyDlls(runSQSExtendedLibIntegrationTests aws-cpp-sdk-core aws-cpp-sdk-s3 aws-cpp-sdk-sqs aws-cpp-sdk-sqs-extended-lib aws-cpp-sdk-access-management aws-cpp-sdk-iam aws-cpp-sdk-cognito-identity gtest gtest_main)

