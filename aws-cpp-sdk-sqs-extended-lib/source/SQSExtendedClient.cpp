/*
 * Copyright 2010-2016 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
#include <aws/core/utils/memory/stl/AWSStringStream.h>
#include <aws/core/utils/json/JsonSerializer.h>
#include <aws/sqs/extendedlib/SQSExtendedClient.h>
#include <aws/sqs/extendedlib/SQSExtendedClientConfiguration.h>
#include <aws/sqs/extendedlib/SQSLargeMessageS3Pointer.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/DeleteObjectRequest.h>

using namespace Aws;
using namespace Aws::S3::Model;
using namespace Aws::SQS::Model;
using namespace Aws::SQS::ExtendedLib;
using namespace Aws::Utils::Json;

static const char* ALLOCATION_TAG = "SQSExtendedClient";
static const char* RESERVED_ATTRIBUTE_NAME = "SQSLargePayloadSize";
static const char* S3_BUCKET_NAME_MARKER = "-..s3BucketName..-";
static const char* S3_KEY_MARKER = "-..s3Key..-";

SQSExtendedClient::SQSExtendedClient (const std::shared_ptr<SQSClient>& sqsclient,
                                      const std::shared_ptr<SQSExtendedClientConfiguration>& sqsconfig) :
    m_sqsclient (sqsclient), m_sqsconfig (sqsconfig)
{
}

SendMessageOutcome SQSExtendedClient::SendMessage (const SendMessageRequest& request) const
{
  if (!m_sqsconfig->IsLargePayloadSupportEnabled ())
  {
    return SQSClient::SendMessage (request);
  }

  if (m_sqsconfig->IsAlwaysThroughS3 () || SQSExtendedClient::IsLargeMessage (request))
  {
    SendMessageRequest reqWithS3Support = SQSExtendedClient::StoreMessageInS3 (request);
    return SQSClient::SendMessage (reqWithS3Support);
  }

  return SQSClient::SendMessage (request);
}

ReceiveMessageOutcome SQSExtendedClient::ReceiveMessage (const ReceiveMessageRequest& request) const
{
  if (!m_sqsconfig->IsLargePayloadSupportEnabled ())
  {
    return SQSClient::ReceiveMessage (request);
  }

  ReceiveMessageRequest reqWithS3Support = request;
  reqWithS3Support.AddMessageAttributeNames (RESERVED_ATTRIBUTE_NAME);

  ReceiveMessageOutcome outcome = SQSClient::ReceiveMessage (reqWithS3Support);
  ReceiveMessageResult result = outcome.GetResult ();

  Aws::Vector<Message> rebuildedMessages;
  for (Message message : result.GetMessages ())
  {

    Aws::Map<Aws::String, MessageAttributeValue> messageAttributes = message.GetMessageAttributes ();
    if (messageAttributes.find (RESERVED_ATTRIBUTE_NAME) != messageAttributes.end ())
    {
      Aws::String messageBody = message.GetBody ();

      // unjsonize object
      SQSLargeMessageS3Pointer s3Pointer = JsonValue (messageBody);
      Aws::String s3BucketName = s3Pointer.GetS3BucketName ();
      Aws::String s3Key = s3Pointer.GetS3Key ();

      // get payload from s3
      GetObjectRequest getObjectRequest;
      getObjectRequest.SetBucket (s3BucketName);
      getObjectRequest.SetKey (s3Key);
      GetObjectOutcome getObjectOutcome = m_sqsconfig->GetS3Client ()->GetObject (getObjectRequest);

      // set original body to message
      Aws::StringStream originalBody;
      originalBody << getObjectOutcome.GetResult ().GetBody ().rdbuf ();
      message.SetBody (originalBody.str ());

      // remove largepayload attribute from message
      messageAttributes.erase (RESERVED_ATTRIBUTE_NAME);
      message.SetMessageAttributes (messageAttributes);

      // Embed s3 object pointer in the receipt handle.
      Aws::String receiptHandle = S3_BUCKET_NAME_MARKER + s3BucketName + S3_BUCKET_NAME_MARKER + S3_KEY_MARKER + s3Key
          + S3_KEY_MARKER + message.GetReceiptHandle ();

      message.SetReceiptHandle (receiptHandle);
    }

    rebuildedMessages.push_back (message);

  }
  result.SetMessages (rebuildedMessages);

  return ReceiveMessageOutcome (result);
}

DeleteMessageOutcome SQSExtendedClient::DeleteMessage (const DeleteMessageRequest& request) const
{
  if (!m_sqsconfig->IsLargePayloadSupportEnabled ()) {
    return SQSClient::DeleteMessage (request);
  }

  Aws::String receiptHandle = request.GetReceiptHandle ();
  if (receiptHandle.find (S3_BUCKET_NAME_MARKER) != std::string::npos
      && receiptHandle.find (S3_KEY_MARKER) != std::string::npos)
  {
    Aws::String s3BucketName = SQSExtendedClient::GetFromReceiptHandleByMarker (receiptHandle, S3_BUCKET_NAME_MARKER);
    Aws::String s3Key = SQSExtendedClient::GetFromReceiptHandleByMarker (receiptHandle, S3_KEY_MARKER);

    DeleteObjectRequest deleteObjectRequest;
    deleteObjectRequest.SetBucket (s3BucketName);
    deleteObjectRequest.SetKey (s3Key);
    m_sqsconfig->GetS3Client ()->DeleteObject (deleteObjectRequest);

    int lastOccurence = receiptHandle.rfind (S3_KEY_MARKER);
    Aws::String cleannedReceiptHandle = receiptHandle.substr (lastOccurence + std::string (S3_KEY_MARKER).length ());

    DeleteMessageRequest reqWithS3Support = request;
    reqWithS3Support.SetReceiptHandle (cleannedReceiptHandle);

    return SQSClient::DeleteMessage (reqWithS3Support);
  }

  return SQSClient::DeleteMessage (request);
}

SendMessageBatchOutcome SQSExtendedClient::SendMessageBatch (const SendMessageBatchRequest& request) const
{
  if (!m_sqsconfig->IsLargePayloadSupportEnabled ()) {
    return SQSClient::SendMessageBatch (request);
  }

  SendMessageBatchRequest reqWithS3Support = request;

  Aws::Vector<SendMessageBatchRequestEntry> batchEntries;
  for (SendMessageBatchRequestEntry entry : reqWithS3Support.GetEntries ())
  {
    if (m_sqsconfig->IsAlwaysThroughS3 () || SQSExtendedClient::IsLargeMessageBatch (entry))
    {
      SendMessageBatchRequestEntry entryWithS3Support = SQSExtendedClient::StoreMessageBatchInS3 (entry);
      batchEntries.push_back (entryWithS3Support);
    }
    else
    {
      batchEntries.push_back (entry);
    }
  }
  reqWithS3Support.SetEntries (batchEntries);

  return SQSClient::SendMessageBatch (reqWithS3Support);
}

DeleteMessageBatchOutcome SQSExtendedClient::DeleteMessageBatch (const DeleteMessageBatchRequest& request) const
{
  if (!m_sqsconfig->IsLargePayloadSupportEnabled ()) {
    return SQSClient::DeleteMessageBatch (request);
  }

  Aws::Vector<DeleteMessageBatchRequestEntry> batchEntries;
  for (auto entry : request.GetEntries ())
  {
    Aws::String receiptHandle = entry.GetReceiptHandle ();
    if (receiptHandle.find (S3_BUCKET_NAME_MARKER) != std::string::npos
        && receiptHandle.find (S3_KEY_MARKER) != std::string::npos)
    {
      Aws::String s3BucketName = SQSExtendedClient::GetFromReceiptHandleByMarker (receiptHandle, S3_BUCKET_NAME_MARKER);
      Aws::String s3Key = SQSExtendedClient::GetFromReceiptHandleByMarker (receiptHandle, S3_KEY_MARKER);

      DeleteObjectRequest deleteObjectRequest;
      deleteObjectRequest.SetBucket (s3BucketName);
      deleteObjectRequest.SetKey (s3Key);
      m_sqsconfig->GetS3Client ()->DeleteObject (deleteObjectRequest);

      int lastOccurence = receiptHandle.rfind (S3_KEY_MARKER);
      Aws::String cleannedReceiptHandle = receiptHandle.substr (lastOccurence + std::string (S3_KEY_MARKER).length ());

      DeleteMessageBatchRequestEntry entryWithS3Support = entry;
      entryWithS3Support.SetReceiptHandle (cleannedReceiptHandle);

      batchEntries.push_back (entryWithS3Support);
    }
    else
    {
      batchEntries.push_back (entry);
    }
  }
  DeleteMessageBatchRequest reqWithS3Support = request;
  reqWithS3Support.SetEntries (batchEntries);

  return SQSClient::DeleteMessageBatch (reqWithS3Support);

}

// ---

// TODO: Need to find a good UUID, but until there, simple randomize it with rand()
Aws::String SQSExtendedClient::RandomizedS3Key () const
{
  srand (time (NULL));

  Aws::String randomizedS3Key = RESERVED_ATTRIBUTE_NAME;
  for (int i = 0; i < 20; ++i)
  {
    randomizedS3Key += static_cast<char>('a' + rand() % 26);
  }
  return randomizedS3Key.c_str();
  //return RESERVED_ATTRIBUTE_NAME + Utils::DateTime::Now().CalculateLocalTimestampAsString("%Y%m%dt%H%M%Sz");
}

unsigned SQSExtendedClient::GetMsgAttributesSize (
    const Aws::Map<Aws::String, Model::MessageAttributeValue>& messageAttributes) const
{
  unsigned size = 0;

  for (auto attribute : messageAttributes)
  {
    Aws::String key = attribute.first;
    Model::MessageAttributeValue value = attribute.second;

    size += key.size ();
    size += value.GetDataType ().size ();
    size += value.GetStringValue ().size ();
    size += value.GetBinaryValue ().GetLength ();
  }

  return size;
}

Aws::String SQSExtendedClient::GetFromReceiptHandleByMarker (const Aws::String receiptHandle, const Aws::String marker) const
{
  int firstOccurence = receiptHandle.find (marker);
  int secondOccurence = receiptHandle.find (marker, firstOccurence + 1);
  int receiptHandleLenght = secondOccurence - firstOccurence - marker.length ();
  return receiptHandle.substr (firstOccurence + marker.length (), receiptHandleLenght);
}

bool SQSExtendedClient::IsLargeMessage (const SendMessageRequest& request) const
{
  unsigned msgAttributesSize = SQSExtendedClient::GetMsgAttributesSize (request.GetMessageAttributes ());
  unsigned msgBodySize = request.GetMessageBody ().size ();
  unsigned totalMsgSize = msgAttributesSize + msgBodySize;
  return (totalMsgSize > m_sqsconfig->GetMessageSizeThreshold ());
}

bool SQSExtendedClient::IsLargeMessageBatch (const SendMessageBatchRequestEntry& request) const
{
  unsigned msgAttributesSize = SQSExtendedClient::GetMsgAttributesSize (request.GetMessageAttributes ());
  unsigned msgBodySize = request.GetMessageBody ().size ();
  unsigned totalMsgSize = msgAttributesSize + msgBodySize;
  return (totalMsgSize > m_sqsconfig->GetMessageSizeThreshold ());
}

SendMessageRequest SQSExtendedClient::StoreMessageInS3 (const SendMessageRequest& request) const
{
  SendMessageRequest reqWithS3Support = request;

  Aws::String s3Key = SQSExtendedClient::RandomizedS3Key ();
  Aws::String body = reqWithS3Support.GetMessageBody ();
  unsigned size = body.size ();
  std::shared_ptr<Aws::IOStream> bodyAsStream = Aws::MakeShared<Aws::StringStream> (ALLOCATION_TAG);
  *bodyAsStream << body;
  bodyAsStream->flush ();

  // Add message attribute as a flag
  MessageAttributeValue messageAttributeValue;
  messageAttributeValue.SetDataType ("Number");
  messageAttributeValue.SetStringValue (std::to_string (size).c_str ());
  reqWithS3Support.AddMessageAttributes (RESERVED_ATTRIBUTE_NAME, messageAttributeValue);

  // Upload payload to S3
  PutObjectRequest putObjectRequest;
  putObjectRequest.SetBucket (m_sqsconfig->GetS3BucketName ());
  putObjectRequest.SetKey (s3Key);
  putObjectRequest.SetBody (bodyAsStream);
  putObjectRequest.SetContentLength (static_cast<long> (putObjectRequest.GetBody ()->tellp ()));
  PutObjectOutcome putObjectOutcome = m_sqsconfig->GetS3Client ()->PutObject (putObjectRequest);

  // Get S3 Handler/Pointer
  SQSLargeMessageS3Pointer s3Pointer;
  s3Pointer.SetS3BucketName (m_sqsconfig->GetS3BucketName ());
  s3Pointer.SetS3Key (s3Key);
  JsonValue json = s3Pointer.Jsonize ();
  reqWithS3Support.SetMessageBody (json.WriteReadable ());

  return reqWithS3Support;
}

SendMessageBatchRequestEntry SQSExtendedClient::StoreMessageBatchInS3 (const SendMessageBatchRequestEntry& request) const
{
  SendMessageBatchRequestEntry reqWithS3Support = request;

  Aws::String s3Key = SQSExtendedClient::RandomizedS3Key ();
  Aws::String body = reqWithS3Support.GetMessageBody ();
  unsigned size = body.size ();
  std::shared_ptr<Aws::IOStream> bodyAsStream = Aws::MakeShared<Aws::StringStream> (ALLOCATION_TAG);
  *bodyAsStream << body;
  bodyAsStream->flush ();

  // Add message attribute as a flag
  MessageAttributeValue messageAttributeValue;
  messageAttributeValue.SetDataType ("Number");
  messageAttributeValue.SetStringValue (std::to_string (size).c_str ());
  reqWithS3Support.AddMessageAttributes (RESERVED_ATTRIBUTE_NAME, messageAttributeValue);

  // Upload payload to S3
  PutObjectRequest putObjectRequest;
  putObjectRequest.SetBucket (m_sqsconfig->GetS3BucketName ());
  putObjectRequest.SetKey (s3Key);
  putObjectRequest.SetBody (bodyAsStream);
  putObjectRequest.SetContentLength (static_cast<long> (putObjectRequest.GetBody ()->tellp ()));
  PutObjectOutcome putObjectOutcome = m_sqsconfig->GetS3Client ()->PutObject (putObjectRequest);

  // Get S3 Handler/Pointer
  SQSLargeMessageS3Pointer s3Pointer;
  s3Pointer.SetS3BucketName (m_sqsconfig->GetS3BucketName ());
  s3Pointer.SetS3Key (s3Key);
  JsonValue json = s3Pointer.Jsonize ();
  reqWithS3Support.SetMessageBody (json.WriteReadable ());

  return reqWithS3Support;
}
