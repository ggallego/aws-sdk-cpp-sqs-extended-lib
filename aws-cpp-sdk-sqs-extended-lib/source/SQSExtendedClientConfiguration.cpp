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

#include <aws/core/utils/memory/stl/AWSStringStream.h>
#include <aws/sqs/extendedlib/SQSExtendedClientConfiguration.h>

using namespace Aws::SQS::ExtendedLib;

SQSExtendedClientConfiguration::SQSExtendedClientConfiguration () :
    m_s3Client (nullptr),
    m_s3BucketName ("bucket"),
    m_messageSizeThreshold (262144),
    m_largePayloadSupport (true),
    m_alwaysThroughS3 (false)
{
}

void SQSExtendedClientConfiguration::SetLargePayloadSupportEnabled (const std::shared_ptr<Aws::S3::S3Client> s3Client,
                                                                    const Aws::String s3BucketName)
{
  m_s3Client = s3Client;
  m_s3BucketName = s3BucketName;
  m_largePayloadSupport = true;
}

void SQSExtendedClientConfiguration::SetLargePayloadSupportDisabled ()
{
  m_s3Client = nullptr;
  m_s3BucketName = "";
  m_largePayloadSupport = false;
}

bool SQSExtendedClientConfiguration::IsLargePayloadSupportEnabled () const
{
  return m_largePayloadSupport;
}

void SQSExtendedClientConfiguration::SetAlwaysThroughS3Enabled ()
{
  m_alwaysThroughS3 = true;
}

void SQSExtendedClientConfiguration::SetAlwaysThroughS3Disabled ()
{
  m_alwaysThroughS3 = false;
}

bool SQSExtendedClientConfiguration::IsAlwaysThroughS3 () const
{
  return m_alwaysThroughS3;
}

std::shared_ptr<Aws::S3::S3Client> SQSExtendedClientConfiguration::GetS3Client () const
{
  return m_s3Client;
}

Aws::String SQSExtendedClientConfiguration::GetS3BucketName () const
{
  return m_s3BucketName;
}

unsigned SQSExtendedClientConfiguration::GetMessageSizeThreshold () const
{
  return m_messageSizeThreshold;
}

