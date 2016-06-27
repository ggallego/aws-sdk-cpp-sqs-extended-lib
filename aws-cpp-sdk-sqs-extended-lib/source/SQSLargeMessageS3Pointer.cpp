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
#include <aws/sqs/extendedlib/SQSLargeMessageS3Pointer.h>

using namespace Aws::Utils::Json;

namespace Aws
{
  namespace SQS
  {
    namespace ExtendedLib
    {

      SQSLargeMessageS3Pointer::SQSLargeMessageS3Pointer () :
          m_s3BucketNameHasBeenSet (false), m_s3KeyHasBeenSet (false)
      {
      }

      SQSLargeMessageS3Pointer::SQSLargeMessageS3Pointer (const JsonValue& jsonValue) :
          m_s3BucketNameHasBeenSet (false), m_s3KeyHasBeenSet (false)
      {
        *this = jsonValue;
      }

      SQSLargeMessageS3Pointer&
      SQSLargeMessageS3Pointer::operator = (const JsonValue& jsonValue)
      {
        if (jsonValue.ValueExists ("S3BucketName"))
        {
          m_s3BucketName = jsonValue.GetString ("S3BucketName");
          m_s3BucketNameHasBeenSet = true;
        }

        if (jsonValue.ValueExists ("S3Key"))
        {
          m_s3Key = jsonValue.GetString ("S3Key");
          m_s3KeyHasBeenSet = true;
        }

        return *this;
      }

      JsonValue
      SQSLargeMessageS3Pointer::Jsonize () const
      {
        JsonValue payload;

        if (m_s3BucketNameHasBeenSet)
          payload.WithString ("S3BucketName", m_s3BucketName);

        if (m_s3KeyHasBeenSet)
          payload.WithString ("S3Key", m_s3Key);

        return payload;
      }

    } // namespace Model
  } // namespace ACM
} // namespace Aws
