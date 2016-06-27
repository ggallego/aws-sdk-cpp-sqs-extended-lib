/*
 * Copyright 2010-2015 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 * 
 *  http://aws.amazon.com/apache2.0
 * 
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include <aws/external/gtest.h>
#include <aws/core/client/ClientConfiguration.h>
#include <aws/core/auth/AWSCredentialsProviderChain.h>
#include <aws/s3/S3Client.h>
#include <aws/sqs/SQSClient.h>
#include <aws/sqs/model/CreateQueueRequest.h>
#include <aws/sqs/model/DeleteQueueRequest.h>
#include <aws/sqs/model/DeleteMessageRequest.h>
#include <aws/sqs/model/ListQueuesRequest.h>
#include <aws/sqs/model/SendMessageRequest.h>
#include <aws/sqs/model/ReceiveMessageRequest.h>
#include <aws/s3/model/CreateBucketRequest.h>
#include <aws/s3/model/DeleteBucketRequest.h>
#include <aws/s3/model/DeleteObjectRequest.h>
#include <aws/s3/model/ListObjectsRequest.h>
#include <aws/s3/model/HeadBucketRequest.h>
#include <aws/s3/model/HeadObjectRequest.h>
#include <aws/access-management/AccessManagementClient.h>
#include <aws/iam/IAMClient.h>
#include <aws/cognito-identity/CognitoIdentityClient.h>
#include <aws/sqs/extendedlib/SQSExtendedClient.h>
#include <aws/sqs/extendedlib/SQSExtendedClientConfiguration.h>
#include <math.h>

using namespace Aws;
using namespace Aws::Http;
using namespace Aws::Auth;
using namespace Aws::Client;
using namespace Aws::S3;
using namespace Aws::S3::Model;
using namespace Aws::SQS;
using namespace Aws::SQS::Model;
using namespace Aws::SQS::ExtendedLib;

static const char* ALLOCATION_TAG = "ExtendedQueueOperationTest";

static const unsigned QUEUE_SIZE_LIMIT = 262144;
static const char* S3_BUCKET_NAME_MARKER = "-..s3BucketName..-";
static const char* S3_KEY_MARKER = "-..s3Key..-";

static const int S3_TIMEOUT_MAX = 10;

#define BUCKET_PREFIX "ExtendedQueue_ITest_"

//static const char* SMALLMESSAGE_WITHPAYLOADSUPPORTENABLED_BUCKET = BUCKET_PREFIX "SMessageWithPayloadSupportEnabled";
static const char* LARGEMESSAGE_WITHPAYLOADSUPPORTENABLED_BUCKET = BUCKET_PREFIX "LMessageWithPayloadSupportEnabled";
//static const char* SMALLMESSAGE_WITHPAYLOADSUPPORTEDISABLED_BUCKET = BUCKET_PREFIX "SMessageWithPayloadSupportDisabled";
//static const char* LARGEMESSAGE_WITHPAYLOADSUPPORTEDISABLED_BUCKET = BUCKET_PREFIX "LMessageWithPayloadSupportDisabled";
static const char* SMALLMESSAGE_WITHALLWAYSTHROUGHS3ENABLED_BUCKET = BUCKET_PREFIX "SMessageWithAllwaysThroughS3Enabled";
static const char* LARGEMESSAGE_WITHALLWAYSTHROUGHS3ENABLED_BUCKET = BUCKET_PREFIX "LMessageWithAllwaysThroughS3Enabled";
static const char* RANDOMBATCHMESSAGES_BUCKET = BUCKET_PREFIX "RamdomBatchMessages";

#define QUEUENAME_PREFIX "ExtendedQueue_ITest_"

static const char* SMALLMESSAGE_WITHPAYLOADSUPPORTENABLED_QUEUENAME = QUEUENAME_PREFIX "SMessageWithLargePayloadSupportEnabled";
static const char* LARGEMESSAGE_WITHPAYLOADSUPPORTENABLED_QUEUENAME = QUEUENAME_PREFIX "LMessageWithLargePayloadSupportEnabled";
static const char* SMALLMESSAGE_WITHPAYLOADSUPPORTEDISABLED_QUEUENAME = QUEUENAME_PREFIX "SMessageWithLPayloadSupportDisabled";
static const char* LARGEMESSAGE_WITHPAYLOADSUPPORTEDISABLED_QUEUENAME = QUEUENAME_PREFIX "LMessageWithLPayloadSupportDisabled";
static const char* SMALLMESSAGE_WITHALLWAYSTHROUGHS3ENABLED_QUEUENAME = QUEUENAME_PREFIX "SMessageWithAllwaysThroughS3Enabled";
static const char* LARGEMESSAGE_WITHALLWAYSTHROUGHS3ENABLED_QUEUENAME = QUEUENAME_PREFIX "LMessageWithAllwaysThroughS3Enabled";
static const char* RANDOMBATCHMESSAGES_QUEUENAME = QUEUENAME_PREFIX "RamdomBatchMessages";

namespace
{

  class ExtendedQueueOperationTest : public ::testing::Test
  {

  public:
    std::shared_ptr<SQSClient> sqsStdClient;
    std::shared_ptr<S3Client> s3Client;
    Aws::String accountId;
    Aws::String timeStamp;

  protected:

    virtual void SetUp ()
    {
      ClientConfiguration stdConfig;
      stdConfig.scheme = Scheme::HTTPS;
      stdConfig.region = Region::US_EAST_1;

#if USE_PROXY_FOR_TESTS
      stdConfig.scheme = Scheme::HTTP;
      stdConfig.proxyHost = PROXY_HOST;
      stdConfig.proxyPort = PROXY_PORT;
#endif

      // get account_id
      auto cognitoClient = Aws::MakeShared<Aws::CognitoIdentity::CognitoIdentityClient> (ALLOCATION_TAG, stdConfig);
      auto iamClient = Aws::MakeShared<Aws::IAM::IAMClient> (ALLOCATION_TAG, stdConfig);
      Aws::AccessManagement::AccessManagementClient accessManagementClient (iamClient, cognitoClient);
      accountId = accessManagementClient.GetAccountId ();

      // build standard sqsClient
      sqsStdClient = Aws::MakeShared<SQSClient> (ALLOCATION_TAG,
                                                 Aws::MakeShared<DefaultAWSCredentialsProviderChain> (ALLOCATION_TAG),
                                                 stdConfig);

      // build s3Client
      s3Client = Aws::MakeShared<S3Client> (ALLOCATION_TAG,
                                            Aws::MakeShared<DefaultAWSCredentialsProviderChain> (ALLOCATION_TAG),
                                            stdConfig, false);

      // TimeStamp to use in bucket names
      timeStamp = Utils::DateTime::Now ().CalculateLocalTimestampAsString ("%Y%m%dt%H%M%Sz");

      // delete buckets and queues, just in case
      DeleteAllQueues ();
      DeleteAllBuckets();
    }

    virtual void TearDown ()
    {
      // delete buckets and queues, just in case
      DeleteAllQueues ();
      DeleteAllBuckets();
      sqsStdClient = nullptr;
      s3Client = nullptr;
    }

    void DeleteAllQueues ()
    {
      ListQueuesRequest listQueueRequest;
      listQueueRequest.WithQueueNamePrefix (QUEUENAME_PREFIX);

      ListQueuesOutcome listQueuesOutcome = sqsStdClient->ListQueues (listQueueRequest);
      ListQueuesResult listQueuesResult = listQueuesOutcome.GetResult ();
      Aws::Vector<Aws::String> urls = listQueuesResult.GetQueueUrls ();
      for (auto& url : listQueuesResult.GetQueueUrls ())
      {
        DeleteQueueRequest deleteQueueRequest;
        deleteQueueRequest.WithQueueUrl (url);
        DeleteQueueOutcome deleteQueueOutcome = sqsStdClient->DeleteQueue (deleteQueueRequest);
      }

      bool done = false;
      while (!done)
      {
        listQueuesOutcome = sqsStdClient->ListQueues (listQueueRequest);
        listQueuesResult = listQueuesOutcome.GetResult ();
        if (listQueuesResult.GetQueueUrls ().size () == 0)
        {
          break;
        }

        std::this_thread::sleep_for (std::chrono::seconds (1));
      }
    }

    void DeleteAllBuckets ()
    {
      ListBucketsOutcome listBucketsOutcome = s3Client->ListBuckets ();
      for (auto& bucket : listBucketsOutcome.GetResult ().GetBuckets ())
      {
        if (bucket.GetName ().find (BUCKET_PREFIX) != std::string::npos)
        {
          Aws::String bucketName = bucket.GetName ();
          EmptyBucket(bucketName);
          DeleteBucketRequest deleteBucketRequest;
          deleteBucketRequest.SetBucket(bucketName);
          DeleteBucketOutcome deleteBucketOutcome = s3Client->DeleteBucket(deleteBucketRequest);
        }
      }
    }

    void EmptyBucket(const Aws::String& bucketName)
    {
        //cleanup bucket
        ListObjectsRequest listObjectsRequest;
        listObjectsRequest.SetBucket(bucketName);
        ListObjectsOutcome listObjectsOutcome = s3Client->ListObjects(listObjectsRequest);
        if (!listObjectsOutcome.IsSuccess())
            return;

        for (const auto& object : listObjectsOutcome.GetResult().GetContents())
        {
            DeleteObjectRequest deleteObjectRequest;
            deleteObjectRequest.SetBucket(bucketName);
            deleteObjectRequest.SetKey(object.GetKey());
            s3Client->DeleteObject(deleteObjectRequest);
        }

        // wait bucket
        unsigned checkForObjectsCount = 0;
        while (checkForObjectsCount++ < S3_TIMEOUT_MAX)
        {
            ListObjectsOutcome listObjectsOutcome = s3Client->ListObjects(listObjectsRequest);
            ASSERT_TRUE(listObjectsOutcome.IsSuccess());
            if (listObjectsOutcome.GetResult().GetContents().size() > 0)
            {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            else
            {
                break;
            }
        }

    }

    bool CreateBucket (const std::shared_ptr<S3Client> s3Client, const Aws::String bucketName)
    {
      CreateBucketRequest createBucketRequest;
      createBucketRequest.SetBucket (bucketName);
      createBucketRequest.SetACL (BucketCannedACL::private_);

      CreateBucketOutcome createBucketOutcome = s3Client->CreateBucket (createBucketRequest);
      if (!createBucketOutcome.IsSuccess ()) {
        return false;
      }

      const CreateBucketResult& createBucketResult = createBucketOutcome.GetResult ();
      if (createBucketResult.GetLocation ().empty ()) {
        return false;
      }

      return WaitForBucketToPropagate (s3Client, bucketName);
    }

    DeleteBucketOutcome DeleteBucket (const std::shared_ptr<S3Client> s3Client, const Aws::String bucketName)
    {
      DeleteBucketRequest deleteBucketRequest;
      deleteBucketRequest.SetBucket (bucketName);
      return s3Client->DeleteBucket (deleteBucketRequest);
    }

    Aws::String CreateQueue (const std::shared_ptr<SQSClient> sqsClient, const Aws::String queueName)
    {
      CreateQueueRequest request;
      request.SetQueueName (queueName);
      request.AddAttributes (QueueAttributeName::MaximumMessageSize, std::to_string (QUEUE_SIZE_LIMIT).c_str ());

      bool shouldContinue = true;
      while (shouldContinue)
      {
        CreateQueueOutcome outcome = sqsClient->CreateQueue (request);
        if (outcome.IsSuccess ())
        {
          return outcome.GetResult ().GetQueueUrl ();
        }
        if (outcome.GetError ().GetErrorType () != SQSErrors::QUEUE_DELETED_RECENTLY)
        {
          return "";
        }
        std::this_thread::sleep_for (std::chrono::seconds (10));
      }

      return "";
    }

    DeleteQueueOutcome DeleteQueue (const std::shared_ptr<SQSClient> sqsClient, const Aws::String queueUrl)
    {
      DeleteQueueRequest deleteQueueRequest;
      deleteQueueRequest.SetQueueUrl (queueUrl);
      DeleteQueueOutcome deleteQueueOutcome = sqsClient->DeleteQueue (deleteQueueRequest);

      bool done = false;
      while (!done)
      {
        if (deleteQueueOutcome.IsSuccess ())
        {
          break;
        }
        std::this_thread::sleep_for (std::chrono::seconds (1));
      }
      return deleteQueueOutcome;
    }

    SendMessageOutcome SendMessage (const std::shared_ptr<SQSClient> sqsClient, const Aws::String queueUrl, const Aws::String messageBody)
    {
      SendMessageRequest sendMessageRequest;
      sendMessageRequest.SetQueueUrl (queueUrl);
      sendMessageRequest.SetMessageBody (messageBody);
      return sqsClient->SendMessage (sendMessageRequest);
    }

    ReceiveMessageOutcome ReceiveMessage (const std::shared_ptr<SQSClient> sqsClient, const Aws::String queueUrl)
    {
      ReceiveMessageRequest receiveMessageRequest;
      receiveMessageRequest.SetMaxNumberOfMessages (1);
      receiveMessageRequest.SetQueueUrl (queueUrl);
      return sqsClient->ReceiveMessage (receiveMessageRequest);
    }

    DeleteMessageOutcome DeleteMessage (const std::shared_ptr<SQSClient> sqsClient, const Aws::String queueUrl,
                   const Aws::String receiptHandle)
    {
      DeleteMessageRequest deleteMessageRequest;
      deleteMessageRequest.SetQueueUrl (queueUrl);
      deleteMessageRequest.SetReceiptHandle (receiptHandle);
      return sqsClient->DeleteMessage (deleteMessageRequest);
    }

    bool WaitForBucketToPropagate (const std::shared_ptr<S3Client> s3Client, const Aws::String& bucketName)
    {
      unsigned timeoutCount = 0;
      while (timeoutCount++ < 10)
      {
        HeadBucketRequest headBucketRequest;
        headBucketRequest.SetBucket (bucketName);
        HeadBucketOutcome headBucketOutcome = s3Client->HeadBucket (headBucketRequest);
        if (headBucketOutcome.IsSuccess ())
        {
          return true;
        }
        std::this_thread::sleep_for (std::chrono::seconds (1));
      }
      return false;
    }

    Aws::String GenerateMessageBody (const unsigned messageLength)
    {
      return Aws::String (messageLength, 'x');
    }

    Aws::String GetFromReceiptHandleByMarker (const Aws::String receiptHandle, const Aws::String marker)
    {
      int firstOccurence = receiptHandle.find (marker);
      int secondOccurence = receiptHandle.find (marker, firstOccurence + 1);
      int receiptHandleLenght = secondOccurence - firstOccurence - marker.length ();
      return receiptHandle.substr (firstOccurence + marker.length (), receiptHandleLenght);
    }

    Aws::String RandomizedS3BucketName (const Aws::String s3BucketNamePrefix) const
    {
      srand (time (NULL));

      Aws::String randomizedS3BucketName;
      randomizedS3BucketName += s3BucketNamePrefix;
      randomizedS3BucketName += "_";
      randomizedS3BucketName += accountId;
      randomizedS3BucketName += "_";
      for (int i = 0; i < 20; ++i)
      {
        randomizedS3BucketName += static_cast<char>('a' + rand() % 26);
      }
      return randomizedS3BucketName.c_str();
    }

  };
} // anonymous namespace

TEST_F(ExtendedQueueOperationTest, TestSmallMessageWithLargePayloadSupportEnabled)
{
  // build an extended sqs config, an extended sqs client and a queue
  auto sqsConfig = Aws::MakeShared<SQSExtendedClientConfiguration> (ALLOCATION_TAG);

  std::shared_ptr<SQSClient> sqsClient = Aws::MakeShared<SQSExtendedClient> (ALLOCATION_TAG, sqsStdClient, sqsConfig);

  Aws::String queueUrl = CreateQueue (sqsClient, SMALLMESSAGE_WITHPAYLOADSUPPORTENABLED_QUEUENAME);

  // prepare a small payload
  Aws::String messageBody = ExtendedQueueOperationTest::GenerateMessageBody (QUEUE_SIZE_LIMIT - 1000);

  // send message
  SendMessageOutcome sendM = ExtendedQueueOperationTest::SendMessage (sqsClient, queueUrl, messageBody);
  ASSERT_TRUE(sendM.IsSuccess ());
  EXPECT_TRUE(sendM.GetResult ().GetMessageId ().length () > 0);

  // receive message
  ReceiveMessageOutcome receiveM = ExtendedQueueOperationTest::ReceiveMessage (sqsClient, queueUrl);
  ASSERT_TRUE(receiveM.IsSuccess ());
  ASSERT_EQ(1uL, receiveM.GetResult ().GetMessages ().size ());
  EXPECT_EQ(messageBody, receiveM.GetResult ().GetMessages ()[0].GetBody ());

  // check if s3 was used. It must not!
  Aws::String receiptHandle = receiveM.GetResult ().GetMessages ()[0].GetReceiptHandle ();
  ASSERT_FALSE(receiptHandle.find (S3_BUCKET_NAME_MARKER) != std::string::npos);
  ASSERT_FALSE(receiptHandle.find (S3_KEY_MARKER) != std::string::npos);

  // delete message
  DeleteMessageOutcome deleteM = ExtendedQueueOperationTest::DeleteMessage (sqsClient, queueUrl, receiptHandle);
  ASSERT_TRUE(deleteM.IsSuccess ());
  receiveM = ExtendedQueueOperationTest::ReceiveMessage (sqsClient, queueUrl);
  EXPECT_EQ(0uL, receiveM.GetResult ().GetMessages ().size ());

  // delete queue
  DeleteQueueOutcome deleteQ = DeleteQueue (sqsClient, queueUrl);
  ASSERT_TRUE(deleteQ.IsSuccess ());
}

TEST_F(ExtendedQueueOperationTest, TestLargeMessageWithLargePayloadSupportEnabled)
{
  // build a bucket, an extended sqs config, an extended sqs client a queue and message body
  Aws::String s3BucketName = RandomizedS3BucketName(LARGEMESSAGE_WITHPAYLOADSUPPORTENABLED_BUCKET);
  CreateBucket (s3Client, s3BucketName);

  auto sqsConfig = Aws::MakeShared<SQSExtendedClientConfiguration> (ALLOCATION_TAG);
  sqsConfig->SetLargePayloadSupportEnabled (s3Client, s3BucketName);

  std::shared_ptr<SQSClient> sqsClient = Aws::MakeShared<SQSExtendedClient> (ALLOCATION_TAG, sqsStdClient, sqsConfig);

  Aws::String queueUrl = CreateQueue (sqsClient, LARGEMESSAGE_WITHPAYLOADSUPPORTENABLED_QUEUENAME);

  // prepare a large payload
  Aws::String messageBody = ExtendedQueueOperationTest::GenerateMessageBody (QUEUE_SIZE_LIMIT + 1000);

  // send message
  SendMessageOutcome sendM = ExtendedQueueOperationTest::SendMessage (sqsClient, queueUrl, messageBody);
  ASSERT_TRUE(sendM.IsSuccess ());
  EXPECT_TRUE(sendM.GetResult ().GetMessageId ().length () > 0);

  // receive message
  ReceiveMessageOutcome receiveM = ExtendedQueueOperationTest::ReceiveMessage (sqsClient, queueUrl);
  ASSERT_TRUE(receiveM.IsSuccess ());
  ASSERT_EQ(1uL, receiveM.GetResult ().GetMessages ().size ());
  EXPECT_EQ(messageBody, receiveM.GetResult ().GetMessages ()[0].GetBody ());

  // check if s3 was used. It must!
  Aws::String receiptHandle = receiveM.GetResult ().GetMessages ()[0].GetReceiptHandle ();
  ASSERT_TRUE(receiptHandle.find (S3_BUCKET_NAME_MARKER) != std::string::npos);
  ASSERT_TRUE(receiptHandle.find (S3_KEY_MARKER) != std::string::npos);

  // delete message
  DeleteMessageOutcome deleteM = ExtendedQueueOperationTest::DeleteMessage (sqsClient, queueUrl, receiptHandle);
  ASSERT_TRUE(deleteM.IsSuccess ());
  receiveM = ExtendedQueueOperationTest::ReceiveMessage (sqsClient, queueUrl);
  EXPECT_EQ(0uL, receiveM.GetResult ().GetMessages ().size ());

  // check if s3key was removed
  Aws::String s3BucketNameToTest = ExtendedQueueOperationTest::GetFromReceiptHandleByMarker (receiptHandle,
                                                                                             S3_BUCKET_NAME_MARKER);
  Aws::String s3KeyToTest = ExtendedQueueOperationTest::GetFromReceiptHandleByMarker (receiptHandle, S3_KEY_MARKER);
  HeadObjectRequest headObjectRequest;
  headObjectRequest.SetBucket (s3BucketNameToTest);
  headObjectRequest.SetKey (s3KeyToTest);
  HeadObjectOutcome headObjectOutcome = s3Client->HeadObject (headObjectRequest);
  ASSERT_FALSE(headObjectOutcome.IsSuccess ());

  // delete queue
  DeleteQueueOutcome deleteQ = DeleteQueue (sqsClient, queueUrl);
  ASSERT_TRUE(deleteQ.IsSuccess ());

  //delete bucket
  DeleteBucketOutcome deleteB = DeleteBucket (s3Client, s3BucketName);
  ASSERT_TRUE(deleteB.IsSuccess ());
}

TEST_F(ExtendedQueueOperationTest, TestSmallMessageWithLargePayloadSupportDisabled)
{
  // build an extended sqs config, an extended sqs client and a queue
  auto sqsConfig = Aws::MakeShared<SQSExtendedClientConfiguration> (ALLOCATION_TAG);

  std::shared_ptr<SQSClient> sqsClient = Aws::MakeShared<SQSExtendedClient> (ALLOCATION_TAG, sqsStdClient, sqsConfig);
  sqsConfig->SetLargePayloadSupportDisabled ();

  Aws::String queueUrl = CreateQueue (sqsClient, SMALLMESSAGE_WITHPAYLOADSUPPORTEDISABLED_QUEUENAME);

  // prepare a small payload
  Aws::String messageBody = ExtendedQueueOperationTest::GenerateMessageBody (QUEUE_SIZE_LIMIT - 1000);

  // send message
  SendMessageOutcome sendM = ExtendedQueueOperationTest::SendMessage (sqsClient, queueUrl, messageBody);
  ASSERT_TRUE(sendM.IsSuccess ());
  EXPECT_TRUE(sendM.GetResult ().GetMessageId ().length () > 0);

  // receive message
  ReceiveMessageOutcome receiveM = ExtendedQueueOperationTest::ReceiveMessage (sqsClient, queueUrl);
  ASSERT_TRUE(receiveM.IsSuccess ());
  ASSERT_EQ(1uL, receiveM.GetResult ().GetMessages ().size ());
  EXPECT_EQ(messageBody, receiveM.GetResult ().GetMessages ()[0].GetBody ());

  // check if s3 was used. It must not!
  Aws::String receiptHandle = receiveM.GetResult ().GetMessages ()[0].GetReceiptHandle ();
  ASSERT_FALSE(receiptHandle.find (S3_BUCKET_NAME_MARKER) != std::string::npos);
  ASSERT_FALSE(receiptHandle.find (S3_KEY_MARKER) != std::string::npos);

  // delete message
  DeleteMessageOutcome deleteM = ExtendedQueueOperationTest::DeleteMessage (sqsClient, queueUrl, receiptHandle);
  ASSERT_TRUE(deleteM.IsSuccess ());
  receiveM = ExtendedQueueOperationTest::ReceiveMessage (sqsClient, queueUrl);
  EXPECT_EQ(0uL, receiveM.GetResult ().GetMessages ().size ());

  // delete queue
  DeleteQueueOutcome deleteQ = DeleteQueue (sqsClient, queueUrl);
  ASSERT_TRUE(deleteQ.IsSuccess ());
}

TEST_F(ExtendedQueueOperationTest, TestLargeMessageWithLargePayloadSupportDisabled)
{
  // build an extended sqs config, an extended sqs client and a queue
  auto sqsConfig = Aws::MakeShared<SQSExtendedClientConfiguration> (ALLOCATION_TAG);

  std::shared_ptr<SQSClient> sqsClient = Aws::MakeShared<SQSExtendedClient> (ALLOCATION_TAG, sqsStdClient, sqsConfig);
  sqsConfig->SetLargePayloadSupportDisabled ();

  Aws::String queueUrl = CreateQueue (sqsClient, LARGEMESSAGE_WITHPAYLOADSUPPORTEDISABLED_QUEUENAME);

  // prepare a large payload
  Aws::String messageBody = ExtendedQueueOperationTest::GenerateMessageBody (QUEUE_SIZE_LIMIT + 1000);

  // send message
  SendMessageOutcome sendM = ExtendedQueueOperationTest::SendMessage (sqsClient, queueUrl, messageBody);
  SQSErrors error = sendM.GetError ().GetErrorType ();
  EXPECT_TRUE(SQSErrors::INVALID_PARAMETER_VALUE == error);

  Aws::String errorMessage = sendM.GetError ().GetMessage ();
  EXPECT_TRUE(errorMessage.find (std::to_string (QUEUE_SIZE_LIMIT).c_str ()) != std::string::npos);
  // GetMessage(): One or more parameters are invalid. Reason: Message must be shorter than 262144 bytes.

  // delete queue
  DeleteQueueOutcome deleteQ = DeleteQueue (sqsClient, queueUrl);
  ASSERT_TRUE(deleteQ.IsSuccess ());
}

TEST_F(ExtendedQueueOperationTest, TestSmallMessageWithAlwaysThroughS3Enabled)
{
  // build an extended sqs config, an extended sqs client and a queue
  Aws::String s3BucketName = RandomizedS3BucketName(SMALLMESSAGE_WITHALLWAYSTHROUGHS3ENABLED_BUCKET);
  CreateBucket (s3Client, s3BucketName);

  auto sqsConfig = Aws::MakeShared<SQSExtendedClientConfiguration> (ALLOCATION_TAG);
  sqsConfig->SetLargePayloadSupportEnabled (s3Client, s3BucketName);
  sqsConfig->SetAlwaysThroughS3Enabled ();

  std::shared_ptr<SQSClient> sqsClient = Aws::MakeShared<SQSExtendedClient> (ALLOCATION_TAG, sqsStdClient, sqsConfig);

  Aws::String queueUrl = CreateQueue (sqsClient, SMALLMESSAGE_WITHALLWAYSTHROUGHS3ENABLED_QUEUENAME);

  // prepare a large payload
  Aws::String messageBody = ExtendedQueueOperationTest::GenerateMessageBody (QUEUE_SIZE_LIMIT - 1000);

  // send message
  SendMessageOutcome sendM = ExtendedQueueOperationTest::SendMessage (sqsClient, queueUrl, messageBody);
  ASSERT_TRUE(sendM.IsSuccess ());
  EXPECT_TRUE(sendM.GetResult ().GetMessageId ().length () > 0);

  // receive message
  ReceiveMessageOutcome receiveM = ExtendedQueueOperationTest::ReceiveMessage (sqsClient, queueUrl);
  ASSERT_TRUE(receiveM.IsSuccess ());
  ASSERT_EQ(1uL, receiveM.GetResult ().GetMessages ().size ());
  EXPECT_EQ(messageBody, receiveM.GetResult ().GetMessages ()[0].GetBody ());

  // check if s3 was used. It must!
  Aws::String receiptHandle = receiveM.GetResult ().GetMessages ()[0].GetReceiptHandle ();
  ASSERT_TRUE(receiptHandle.find (S3_BUCKET_NAME_MARKER) != std::string::npos);
  ASSERT_TRUE(receiptHandle.find (S3_KEY_MARKER) != std::string::npos);

  // delete message
  DeleteMessageOutcome deleteM = ExtendedQueueOperationTest::DeleteMessage (sqsClient, queueUrl, receiptHandle);
  ASSERT_TRUE(deleteM.IsSuccess ());
  receiveM = ExtendedQueueOperationTest::ReceiveMessage (sqsClient, queueUrl);
  EXPECT_EQ(0uL, receiveM.GetResult ().GetMessages ().size ());

  // check if s3key was removed
  Aws::String s3BucketNameToTest = ExtendedQueueOperationTest::GetFromReceiptHandleByMarker (receiptHandle,
                                                                                             S3_BUCKET_NAME_MARKER);
  Aws::String s3KeyToTest = ExtendedQueueOperationTest::GetFromReceiptHandleByMarker (receiptHandle, S3_KEY_MARKER);
  HeadObjectRequest headObjectRequest;
  headObjectRequest.SetBucket (s3BucketNameToTest);
  headObjectRequest.SetKey (s3KeyToTest);
  HeadObjectOutcome headObjectOutcome = s3Client->HeadObject (headObjectRequest);
  ASSERT_FALSE(headObjectOutcome.IsSuccess ());

  // delete queue
  DeleteQueueOutcome deleteQ = DeleteQueue (sqsClient, queueUrl);
  ASSERT_TRUE(deleteQ.IsSuccess ());

  //delete bucket
  DeleteBucketOutcome deleteB = DeleteBucket (s3Client, s3BucketName);
  ASSERT_TRUE(deleteB.IsSuccess ());
}

TEST_F(ExtendedQueueOperationTest, TestLargeMessageWithAlwaysThroughS3Enabled)
{
  // build an extended sqs config, an extended sqs client and a queue
  Aws::String s3BucketName = RandomizedS3BucketName(LARGEMESSAGE_WITHALLWAYSTHROUGHS3ENABLED_BUCKET);
  CreateBucket (s3Client, s3BucketName);

  auto sqsConfig = Aws::MakeShared<SQSExtendedClientConfiguration> (ALLOCATION_TAG);
  sqsConfig->SetLargePayloadSupportEnabled (s3Client, s3BucketName);
  sqsConfig->SetAlwaysThroughS3Enabled ();

  std::shared_ptr<SQSClient> sqsClient = Aws::MakeShared<SQSExtendedClient> (ALLOCATION_TAG, sqsStdClient, sqsConfig);

  Aws::String queueUrl = CreateQueue (sqsClient, LARGEMESSAGE_WITHALLWAYSTHROUGHS3ENABLED_QUEUENAME);

  // prepare a large payload
  Aws::String messageBody = ExtendedQueueOperationTest::GenerateMessageBody (QUEUE_SIZE_LIMIT + 1000);

  // send message
  SendMessageOutcome sendM = ExtendedQueueOperationTest::SendMessage (sqsClient, queueUrl, messageBody);
  ASSERT_TRUE(sendM.IsSuccess ());
  EXPECT_TRUE(sendM.GetResult ().GetMessageId ().length () > 0);

  // receive message
  ReceiveMessageOutcome receiveM = ExtendedQueueOperationTest::ReceiveMessage (sqsClient, queueUrl);
  ASSERT_TRUE(receiveM.IsSuccess ());
  ASSERT_EQ(1uL, receiveM.GetResult ().GetMessages ().size ());
  EXPECT_EQ(messageBody, receiveM.GetResult ().GetMessages ()[0].GetBody ());

  // check if s3 was used. It must!
  Aws::String receiptHandle = receiveM.GetResult ().GetMessages ()[0].GetReceiptHandle ();
  ASSERT_TRUE(receiptHandle.find (S3_BUCKET_NAME_MARKER) != std::string::npos);
  ASSERT_TRUE(receiptHandle.find (S3_KEY_MARKER) != std::string::npos);

  // delete message
  DeleteMessageOutcome deleteM = ExtendedQueueOperationTest::DeleteMessage (sqsClient, queueUrl, receiptHandle);
  ASSERT_TRUE(deleteM.IsSuccess ());
  receiveM = ExtendedQueueOperationTest::ReceiveMessage (sqsClient, queueUrl);
  EXPECT_EQ(0uL, receiveM.GetResult ().GetMessages ().size ());

  // check if s3key was removed
  Aws::String s3BucketNameToTest = ExtendedQueueOperationTest::GetFromReceiptHandleByMarker (receiptHandle,
                                                                                             S3_BUCKET_NAME_MARKER);
  Aws::String s3KeyToTest = ExtendedQueueOperationTest::GetFromReceiptHandleByMarker (receiptHandle, S3_KEY_MARKER);
  HeadObjectRequest headObjectRequest;
  headObjectRequest.SetBucket (s3BucketNameToTest);
  headObjectRequest.SetKey (s3KeyToTest);
  HeadObjectOutcome headObjectOutcome = s3Client->HeadObject (headObjectRequest);
  ASSERT_FALSE(headObjectOutcome.IsSuccess ());

  // delete queue
  DeleteQueueOutcome deleteQ = DeleteQueue (sqsClient, queueUrl);
  ASSERT_TRUE(deleteQ.IsSuccess ());

  //delete bucket
  DeleteBucketOutcome deleteB = DeleteBucket (s3Client, s3BucketName);
  ASSERT_TRUE(deleteB.IsSuccess ());
}


// TODO: FIND OUT WHY BUG IS HAPPENING: THIS TEST FAILS BECAUSE SOMETHING STRANGE IS ON SENDMESSAGEBATCH METHOD,
// THE MESSAGEATTRIBUTES ARE LOSING ITS ATTRIBUTES JUST AFTER SENDMESSAGEBATCH IS CALLED.
// MESSAGEATTRIBUTE 'SQSLargePayloadSize' ARE KEY CONTROL IF A SQS MESSAGE HAS LARGEPAYLOAD SUPPORT.
TEST_F(ExtendedQueueOperationTest, TestBatchMessagesWithRamdomPayloadSize)
{
  // build an extended sqs config, an extended sqs client and a queue
  Aws::String s3BucketName = RandomizedS3BucketName(RANDOMBATCHMESSAGES_BUCKET);
  CreateBucket (s3Client, s3BucketName);

  auto sqsConfig = Aws::MakeShared<SQSExtendedClientConfiguration> (ALLOCATION_TAG);
  sqsConfig->SetLargePayloadSupportEnabled (s3Client, s3BucketName);

  std::shared_ptr<SQSClient> sqsClient = Aws::MakeShared<SQSExtendedClient> (ALLOCATION_TAG, sqsStdClient, sqsConfig);

  Aws::String queueUrl = CreateQueue (sqsClient, RANDOMBATCHMESSAGES_QUEUENAME);

  Aws::Vector<unsigned> messageSizes = {100, 1000, 300000, 500000};
  unsigned numberOfMessages = messageSizes.size();

  // create sendBatchEntries
  Aws::Vector<SendMessageBatchRequestEntry> sendBatchEntries;
  for (unsigned i = 1; i <= numberOfMessages; i++)
  {
    SendMessageBatchRequestEntry entry;
    String messageBody = ExtendedQueueOperationTest::GenerateMessageBody (messageSizes[i-1]);
    entry.SetMessageBody (messageBody);
    entry.SetId (std::to_string (i).c_str ());
    sendBatchEntries.push_back (entry);
  }

  // send messages
  SendMessageBatchRequest sendMessageBatchRequest;
  sendMessageBatchRequest.SetQueueUrl (queueUrl);
  sendMessageBatchRequest.SetEntries (sendBatchEntries);
  SendMessageBatchOutcome sendM = sqsClient->SendMessageBatch (sendMessageBatchRequest);
  ASSERT_TRUE(sendM.IsSuccess ());
  ASSERT_EQ(numberOfMessages, sendM.GetResult ().GetSuccessful ().size ());

  // receive messages
  ReceiveMessageRequest receiveMessageRequest;
  receiveMessageRequest.SetQueueUrl (queueUrl);
  receiveMessageRequest.SetMaxNumberOfMessages (1);
  Vector<Message> messages;
  for (unsigned i = 1; i <= numberOfMessages; i++)
  {
    auto receiveM = sqsClient->ReceiveMessage (receiveMessageRequest);
    ASSERT_TRUE(receiveM.IsSuccess ());
    for (auto& message : receiveM.GetResult ().GetMessages ())
    {
      messages.push_back (message);
    }
  }
  ASSERT_EQ(numberOfMessages, messages.size ());

  // create deleteBatchEntries
  Aws::Vector<DeleteMessageBatchRequestEntry> deleteBatchEntries;
  for (unsigned i = 1; i <= numberOfMessages; i++)
  {
    DeleteMessageBatchRequestEntry entry;
    entry.SetReceiptHandle (messages[i - 1].GetReceiptHandle ());
    entry.SetId (std::to_string (i).c_str ());
    deleteBatchEntries.push_back (entry);
  }

  // delete messages
  DeleteMessageBatchRequest deleteMessageBatchRequest;
  deleteMessageBatchRequest.SetQueueUrl (queueUrl);
  deleteMessageBatchRequest.SetEntries (deleteBatchEntries);
  DeleteMessageBatchOutcome deleteM = sqsClient->DeleteMessageBatch (deleteMessageBatchRequest);
  ASSERT_TRUE(deleteM.IsSuccess ());
  ReceiveMessageOutcome receiveM = ExtendedQueueOperationTest::ReceiveMessage (sqsClient, queueUrl);
  EXPECT_EQ(0uL, receiveM.GetResult ().GetMessages ().size ());

  // delete queue
  DeleteQueueOutcome deleteQ = DeleteQueue (sqsClient, queueUrl);
  ASSERT_TRUE(deleteQ.IsSuccess ());

  //delete bucket
  DeleteBucketOutcome deleteB = DeleteBucket (s3Client, s3BucketName);
  ASSERT_TRUE(deleteB.IsSuccess ());
}

