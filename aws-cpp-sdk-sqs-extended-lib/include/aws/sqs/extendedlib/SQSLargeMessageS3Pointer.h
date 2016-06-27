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
#include <aws/core/utils/memory/stl/AWSString.h>
#include <aws/core/utils/json/JsonSerializer.h>
#include <aws/sqs/SQS_EXPORTS.h>

namespace Aws
{
  namespace Utils
  {
    namespace Json
    {
      class JsonValue;
    } // namespace Json
  } // namespace Utils

  namespace SQS
  {
    namespace ExtendedLib
    {

      class AWS_SQS_API SQSLargeMessageS3Pointer
      {

      private:
        Aws::String m_s3BucketName;
        bool m_s3BucketNameHasBeenSet;
        Aws::String m_s3Key;
        bool m_s3KeyHasBeenSet;

      public:
        SQSLargeMessageS3Pointer ();
        SQSLargeMessageS3Pointer (const Aws::Utils::Json::JsonValue& jsonValue);
        SQSLargeMessageS3Pointer& operator= (const Aws::Utils::Json::JsonValue& jsonValue);
        Aws::Utils::Json::JsonValue Jsonize () const;

        inline const Aws::String& GetS3BucketName () const
        {
          return m_s3BucketName;
        }

        inline void SetS3BucketName (const Aws::String& s3BucketName)
        {
          m_s3BucketNameHasBeenSet = true;
          m_s3BucketName = s3BucketName;
        }
        inline void SetS3BucketName (Aws::String&& s3BucketName)
        {
          m_s3BucketNameHasBeenSet = true;
          m_s3BucketName = s3BucketName;
        }
        inline void SetS3BucketName (const char* s3BucketName)
        {
          m_s3BucketNameHasBeenSet = true;
          m_s3BucketName.assign (s3BucketName);
        }

        inline SQSLargeMessageS3Pointer& WithS3BucketName (const Aws::String& s3BucketName)
        {
          SetS3BucketName (s3BucketName);
          return *this;
        }
        inline SQSLargeMessageS3Pointer& WithS3BucketName (Aws::String&& s3BucketName)
        {
          SetS3BucketName (s3BucketName);
          return *this;
        }
        inline SQSLargeMessageS3Pointer& WithS3BucketName (const char* s3BucketName)
        {
          SetS3BucketName (s3BucketName);
          return *this;
        }

        inline const Aws::String& GetS3Key () const
        {
          return m_s3Key;
        }

        inline void SetS3Key (const Aws::String& s3Key)
        {
          m_s3KeyHasBeenSet = true;
          m_s3Key = s3Key;
        }
        inline void SetS3Key (Aws::String&& s3Key)
        {
          m_s3KeyHasBeenSet = true;
          m_s3Key = s3Key;
        }
        inline void SetS3Key (const char* s3Key)
        {
          m_s3KeyHasBeenSet = true;
          m_s3Key.assign (s3Key);
        }

        inline SQSLargeMessageS3Pointer& WithS3Key (const Aws::String& s3Key)
        {
          SetS3Key (s3Key);
          return *this;
        }

        inline SQSLargeMessageS3Pointer& WithS3Key (Aws::String&& s3Key)
        {
          SetS3Key (s3Key);
          return *this;
        }

        inline SQSLargeMessageS3Pointer& WithS3Key (const char* s3Key)
        {
          SetS3Key (s3Key);
          return *this;
        }

      };

    } // namespace Model
  } // namespace ACM
} // namespace Aws
