#include "put_object.h"

namespace uh::cluster::rest::http::model
{

    std::map<std::string, std::string> put_object::get_headers() const
    {
        std::map<std::string, std::string> headers;
        std::stringstream ss;

        if(m_aCLHasBeenSet)
        {
            headers.emplace("x-amz-acl", get_name_for_object_canned_acl(m_aCL));
        }

        if(m_cacheControlHasBeenSet)
        {
            ss << m_cacheControl;
            headers.emplace("cache-control",  ss.str());
            ss.str("");
        }

        if(m_contentDispositionHasBeenSet)
        {
            ss << m_contentDisposition;
            headers.emplace("content-disposition",  ss.str());
            ss.str("");
        }

        if(m_contentEncodingHasBeenSet)
        {
            ss << m_contentEncoding;
            headers.emplace("content-encoding",  ss.str());
            ss.str("");
        }

        if(m_contentLanguageHasBeenSet)
        {
            ss << m_contentLanguage;
            headers.emplace("content-language",  ss.str());
            ss.str("");
        }

        if(m_contentLengthHasBeenSet)
        {
            ss << m_contentLength;
            headers.emplace("content-length",  ss.str());
            ss.str("");
        }

        if(m_contentMD5HasBeenSet)
        {
            ss << m_contentMD5;
            headers.emplace("content-md5",  ss.str());
            ss.str("");
        }

//        if(m_checksumAlgorithmHasBeenSet)
//        {
//            headers.emplace("x-amz-sdk-checksum-algorithm", ChecksumAlgorithmMapper::GetNameForChecksumAlgorithm(m_checksumAlgorithm));
//        }

        if(m_checksumCRC32HasBeenSet)
        {
            ss << m_checksumCRC32;
            headers.emplace("x-amz-checksum-crc32",  ss.str());
            ss.str("");
        }

        if(m_checksumCRC32CHasBeenSet)
        {
            ss << m_checksumCRC32C;
            headers.emplace("x-amz-checksum-crc32c",  ss.str());
            ss.str("");
        }

        if(m_checksumSHA1HasBeenSet)
        {
            ss << m_checksumSHA1;
            headers.emplace("x-amz-checksum-sha1",  ss.str());
            ss.str("");
        }

        if(m_checksumSHA256HasBeenSet)
        {
            ss << m_checksumSHA256;
            headers.emplace("x-amz-checksum-sha256",  ss.str());
            ss.str("");
        }

//        if(m_expiresHasBeenSet)
//        {
//            headers.emplace("expires", m_expires.ToGmtString(Aws::Utils::DateFormat::RFC822));
//        }

        if(m_grantFullControlHasBeenSet)
        {
            ss << m_grantFullControl;
            headers.emplace("x-amz-grant-full-control",  ss.str());
            ss.str("");
        }

        if(m_grantReadHasBeenSet)
        {
            ss << m_grantRead;
            headers.emplace("x-amz-grant-read",  ss.str());
            ss.str("");
        }

        if(m_grantReadACPHasBeenSet)
        {
            ss << m_grantReadACP;
            headers.emplace("x-amz-grant-read-acp",  ss.str());
            ss.str("");
        }

        if(m_grantWriteACPHasBeenSet)
        {
            ss << m_grantWriteACP;
            headers.emplace("x-amz-grant-write-acp",  ss.str());
            ss.str("");
        }

        if(m_metadataHasBeenSet)
        {
            for(const auto& item : m_metadata)
            {
                ss << "x-amz-meta-" << item.first;
                headers.emplace(ss.str(), item.second);
                ss.str("");
            }
        }

//        if(m_serverSideEncryptionHasBeenSet)
//        {
//            headers.emplace("x-amz-server-side-encryption", ServerSideEncryptionMapper::GetNameForServerSideEncryption(m_serverSideEncryption));
//        }

//        if(m_storageClassHasBeenSet)
//        {
//            headers.emplace("x-amz-storage-class", StorageClassMapper::GetNameForStorageClass(m_storageClass));
//        }

        if(m_websiteRedirectLocationHasBeenSet)
        {
            ss << m_websiteRedirectLocation;
            headers.emplace("x-amz-website-redirect-location",  ss.str());
            ss.str("");
        }

        if(m_sSECustomerAlgorithmHasBeenSet)
        {
            ss << m_sSECustomerAlgorithm;
            headers.emplace("x-amz-server-side-encryption-customer-algorithm",  ss.str());
            ss.str("");
        }

        if(m_sSECustomerKeyHasBeenSet)
        {
            ss << m_sSECustomerKey;
            headers.emplace("x-amz-server-side-encryption-customer-key",  ss.str());
            ss.str("");
        }

        if(m_sSECustomerKeyMD5HasBeenSet)
        {
            ss << m_sSECustomerKeyMD5;
            headers.emplace("x-amz-server-side-encryption-customer-key-md5",  ss.str());
            ss.str("");
        }

        if(m_sSEKMSKeyIdHasBeenSet)
        {
            ss << m_sSEKMSKeyId;
            headers.emplace("x-amz-server-side-encryption-aws-kms-key-id",  ss.str());
            ss.str("");
        }

        if(m_sSEKMSEncryptionContextHasBeenSet)
        {
            ss << m_sSEKMSEncryptionContext;
            headers.emplace("x-amz-server-side-encryption-context",  ss.str());
            ss.str("");
        }

        if(m_bucketKeyEnabledHasBeenSet)
        {
            ss << std::boolalpha << m_bucketKeyEnabled;
            headers.emplace("x-amz-server-side-encryption-bucket-key-enabled", ss.str());
            ss.str("");
        }

//        if(m_requestPayerHasBeenSet)
//        {
//            headers.emplace("x-amz-request-payer", RequestPayerMapper::GetNameForRequestPayer(m_requestPayer));
//        }

        if(m_taggingHasBeenSet)
        {
            ss << m_tagging;
            headers.emplace("x-amz-tagging",  ss.str());
            ss.str("");
        }

//        if(m_objectLockModeHasBeenSet)
//        {
//            headers.emplace("x-amz-object-lock-mode", ObjectLockModeMapper::GetNameForObjectLockMode(m_objectLockMode));
//        }

//        if(m_objectLockRetainUntilDateHasBeenSet)
//        {
//            headers.emplace("x-amz-object-lock-retain-until-date", m_objectLockRetainUntilDate.ToGmtString(Aws::Utils::DateFormat::ISO_8601));
//        }

//        if(m_objectLockLegalHoldStatusHasBeenSet)
//        {
//            headers.emplace("x-amz-object-lock-legal-hold", ObjectLockLegalHoldStatusMapper::GetNameForObjectLockLegalHoldStatus(m_objectLockLegalHoldStatus));
//        }

        if(m_expectedBucketOwnerHasBeenSet)
        {
            ss << m_expectedBucketOwner;
            headers.emplace("x-amz-expected-bucket-owner",  ss.str());
            ss.str("");
        }

        return headers;
    }

} // uh::cluster::http::rest::model
