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
#pragma once
#include <aws/sqs/extendedlib/SQSExtendedClientConfiguration.h>
#include <aws/sqs/model/MessageAttributeValue.h>
#include <aws/sqs/model/SendMessageRequest.h>
#include <aws/sqs/model/SendMessageBatchRequest.h>
#include <aws/sqs/model/SendMessageBatchRequestEntry.h>
#include <aws/sqs/model/ReceiveMessageRequest.h>
#include <aws/sqs/model/DeleteMessageRequest.h>
#include <aws/sqs/model/DeleteMessageBatchRequest.h>
#include <aws/sqs/SQSClient.h>
#include <aws/sqs/SQS_EXPORTS.h>

namespace Aws
{
  namespace SQS
  {
    namespace ExtendedLib
    {

    class AWS_SQS_API SQSExtendedClient : public SQSClient
    {

    private:
      std::shared_ptr<SQS::SQSClient> m_sqsclient;
      std::shared_ptr<SQSExtendedClientConfiguration> m_sqsconfig;

      virtual Aws::String RandomizedS3Key() const;
      virtual unsigned GetMsgAttributesSize(const Aws::Map<Aws::String, Model::MessageAttributeValue>& messageAttributes) const;
      virtual Aws::String GetFromReceiptHandleByMarker(const Aws::String receiptHandle, const Aws::String marker) const;
      virtual bool IsLargeMessage (const Model::SendMessageRequest& request) const;
      virtual bool IsLargeMessageBatch (const Model::SendMessageBatchRequestEntry& request) const;
      virtual Model::SendMessageRequest StoreMessageInS3 (const Model::SendMessageRequest& request) const;
      virtual Model::SendMessageBatchRequestEntry StoreMessageBatchInS3 (const Model::SendMessageBatchRequestEntry& request) const;

    public:
      SQSExtendedClient (const std::shared_ptr<SQSClient>& sqsclient, const std::shared_ptr<SQSExtendedClientConfiguration>& sqsconfig);

      virtual Model::SendMessageOutcome SendMessage (const Model::SendMessageRequest& request) const;
      virtual Model::ReceiveMessageOutcome ReceiveMessage(const Model::ReceiveMessageRequest& request) const;
      virtual Model::DeleteMessageOutcome DeleteMessage(const Model::DeleteMessageRequest& request) const;
      virtual Model::SendMessageBatchOutcome SendMessageBatch(const Model::SendMessageBatchRequest& request) const;
      virtual Model::DeleteMessageBatchOutcome DeleteMessageBatch(const Model::DeleteMessageBatchRequest& request) const;

    };

    } // namespace extendedLib
  } // namespace SQS
} // namespace Aws

