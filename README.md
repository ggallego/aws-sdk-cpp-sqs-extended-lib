# aws-sdk-cpp-sqs-extended-lib

This is an unofficial CPP port of [Amazon SQS Extended Client Library for Java](https://github.com/ggallego/amazon-sqs-java-extended-client-lib) built using [aws-sdk-cpp](https://github.com/aws/aws-sdk-cpp) cmake structure.

## Requirements:
* [aws-sdk-cpp](https://github.com/aws/aws-sdk-cpp) already built.

#### 'Quick build' aws-sdk-cpp on EC2 c4.2xlarge (Ubuntu Server 14.04 LTS (HVM) - ami-fce3c696) 
```
$ sudo apt-get update && sudo apt-get upgrade

$ sudo apt-get install -y software-properties-common
$ sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
$ sudo apt-get update
$ sudo apt-get install g++-4.9
$ sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-4.9 60 --slave /usr/bin/g++ g++ /usr/bin/g++-4.9

$ sudo apt-get install git
$ git clone https://github.com/aws/aws-sdk-cpp
$ cd aws-sdk-cpp

$ sudo apt-get install cmake
$ cmake .

$ sudo apt-get install libcurl4-openssl-dev
$ make
```

## How to Build this lib:
```
$ git clone https://github.com/ggallego/aws-sdk-cpp-sqs-extended-lib
$ cd aws-sdk-cpp-sqs-extended-lib
$ cmake -Daws-sdk-cpp_DIR=/home/ubuntu/aws-sdk-cpp .
$ make
```

## How to Run integration tests:
awscli on ec2 is on version 1.2.X, aws-cpp-sdk Aws::InitAPI() needs a newer version, so you need setup AWS_* env vars to be able to run it.
```
$ export AWS_ACCESS_KEY_ID=<ACCESS_KEY>
$ export AWS_SECRET_ACCESS_KEY=<SECRET_KEY>
$ export AWS_DEFAULT_REGION=<REGION>

$ cd aws-cpp-sdk-sqs-extended-lib-integration-tests
$ ./runSQSExtendedLibIntegrationTest
```

## How to Use this lib:
* See [here](https://aws.amazon.com/pt/blogs/developer/using-cmake-exports-with-the-aws-sdk-for-c/) for how to use aws-sdk-cpp libs in general.
* See [here](http://docs.aws.amazon.com/AWSSimpleQueueService/latest/SQSDeveloperGuide/s3-messages.html) for how to use the aws-sqs-java-extended-lib (cpp-extended-lib will be quite similar).

## PS:
This port was only tested for macos and linux platforms.
