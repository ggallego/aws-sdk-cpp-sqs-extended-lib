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
#include <aws/s3/S3Client.h>

namespace Aws
{
  namespace SQS
  {
    namespace ExtendedLib
    {

      class SQSExtendedClientConfiguration
      {

      private:
        std::shared_ptr<Aws::S3::S3Client> m_s3Client;
        Aws::String m_s3BucketName;
        unsigned m_messageSizeThreshold;
        bool m_largePayloadSupport;
        bool m_alwaysThroughS3;

      public:
        SQSExtendedClientConfiguration ();

        virtual void SetLargePayloadSupportEnabled (const std::shared_ptr<Aws::S3::S3Client> s3Client,
                                                    const Aws::String s3BucketName);
        virtual void SetLargePayloadSupportDisabled ();
        virtual bool IsLargePayloadSupportEnabled () const;

        virtual void SetAlwaysThroughS3Enabled ();
        virtual void SetAlwaysThroughS3Disabled ();
        virtual bool IsAlwaysThroughS3 () const;

        virtual std::shared_ptr<Aws::S3::S3Client> GetS3Client () const;
        virtual Aws::String GetS3BucketName () const;
        virtual unsigned GetMessageSizeThreshold () const;

      };

    } // namespace extendedLib
  } // namespace SQS
} // namespace Aws

