/*
 *
 *    Copyright (c) 2017 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/**
 *    @file
 *      Supporting code for testing Weave key export.
 *
 */

#include <Weave/Core/WeaveCore.h>
#include <Weave/Support/NestCerts.h>
#include <Weave/Profiles/security/WeaveAccessToken.h>
#include <Weave/Profiles/security/WeaveDummyGroupKeyStore.h>
#include "WeaveKeyExportClient.h"

// This code is only available in contexts that support malloc and system time.
#if HAVE_MALLOC && HAVE_FREE && HAVE_TIME_H

#include <time.h>
#include <stdlib.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace Security {
namespace KeyExport {

class SimulatedDeviceGroupKeyStore : public nl::Weave::Profiles::Security::AppKeys::DummyGroupKeyStore
{
public:
    virtual WEAVE_ERROR RetrieveGroupKey(uint32_t keyId, WeaveGroupKey& key)
    {
        WEAVE_ERROR err = WEAVE_NO_ERROR;

        // Only support the fabric secret.
        VerifyOrExit(keyId == WeaveKeyId::kFabricSecret, err = WEAVE_ERROR_INVALID_KEY_ID);

        // Return a fixed key value.
        memset(&key, 0, sizeof(key));
        key.KeyId = keyId;
        key.KeyLen = nl::Weave::Profiles::Security::AppKeys::kWeaveFabricSecretSize;
        for (uint8_t i = 0; i < key.KeyLen; i++)
            key.Key[i] = i;

    exit:
        return err;
    }
};

class SimulatedDeviceKeyExportDelegate : public WeaveKeyExportDelegate
{
public:
    SimulatedDeviceKeyExportDelegate(const uint8_t *deviceCert, uint16_t deviceCertLen,
                                     const uint8_t *devicePrivKey, uint16_t devicePrivKeyLen,
                                     const uint8_t *rootCert, uint16_t rootCertLen)
    {
        mDeviceCert = deviceCert;
        mDeviceCertLen = deviceCertLen;
        mDevicePrivKey = devicePrivKey;
        mDevicePrivKeyLen = devicePrivKeyLen;
        mRootCert = rootCert;
        mRootCertLen = rootCertLen;
    }

    virtual WEAVE_ERROR GetNodeCertSet(bool isInitiator, WeaveCertificateSet& certSet)
    {
        WEAVE_ERROR err;
        WeaveCertificateData *cert;

        VerifyOrExit(!isInitiator, err = WEAVE_ERROR_INVALID_ARGUMENT);

        err = certSet.Init(10, 4096);
        SuccessOrExit(err);

        err = certSet.LoadCert(mDeviceCert, mDeviceCertLen, 0, cert);
        SuccessOrExit(err);

    exit:
        if (err != WEAVE_NO_ERROR)
        {
            certSet.Release();
        }
        return err;
    }

    virtual WEAVE_ERROR ReleaseNodeCertSet(bool isInitiator, WeaveCertificateSet& certSet)
    {
        WEAVE_ERROR err = WEAVE_NO_ERROR;

        VerifyOrExit(!isInitiator, err = WEAVE_ERROR_INVALID_ARGUMENT);

        certSet.Release();

    exit:
        return err;
    }

    virtual WEAVE_ERROR GetNodePrivateKey(bool isInitiator, const uint8_t *& weavePrivKey, uint16_t& weavePrivKeyLen)
    {
        weavePrivKey = mDevicePrivKey;
        weavePrivKeyLen = mDevicePrivKeyLen;
        return WEAVE_NO_ERROR;
    }

    virtual WEAVE_ERROR ReleaseNodePrivateKey(bool isInitiator, const uint8_t *& weavePrivKey)
    {
        return WEAVE_NO_ERROR;
    }

    virtual WEAVE_ERROR BeginCertValidation(bool isInitiator, WeaveCertificateSet& certSet, ValidationContext& validContext)
    {
        WEAVE_ERROR err;
        WeaveCertificateData *cert;

        VerifyOrExit(!isInitiator, err = WEAVE_ERROR_INVALID_ARGUMENT);

        err = certSet.Init(10, 4096);
        SuccessOrExit(err);

        // Initialize the validation context.
        memset(&validContext, 0, sizeof(validContext));
        validContext.EffectiveTime = SecondsSinceEpochToPackedCertTime(time(NULL));
        validContext.RequiredKeyUsages = kKeyUsageFlag_DigitalSignature;
        validContext.ValidateFlags = kValidateFlag_IgnoreNotAfter;

        // Load the Nest Production Device CA certificate so that it is available for chain validation.
        err = certSet.LoadCert(mRootCert, mRootCertLen, kDecodeFlag_IsTrusted, cert);
        SuccessOrExit(err);

    exit:
        return err;
    }

    virtual WEAVE_ERROR HandleCertValidationResult(bool isInitiator, WeaveCertificateSet& certSet, ValidationContext& validContext,
                                                   const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo, uint32_t requestedKeyId)
    {
        WEAVE_ERROR err = WEAVE_NO_ERROR;

        VerifyOrExit(requestedKeyId == WeaveKeyId::kClientRootKey, err = WEAVE_ERROR_INVALID_KEY_ID);

    exit:
        return err;
    }

    virtual WEAVE_ERROR EndCertValidation(bool isInitiator, WeaveCertificateSet& certSet, ValidationContext& validContext)
    {
        WEAVE_ERROR err = WEAVE_NO_ERROR;

        VerifyOrExit(!isInitiator, err = WEAVE_ERROR_INVALID_ARGUMENT);

        certSet.Release();

    exit:
        return err;
    }

    virtual WEAVE_ERROR ValidateUnsignedKeyExportMessage(bool isInitiator, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo, uint32_t requestedKeyId)
    {
        // Should never be called.
        return WEAVE_ERROR_INCORRECT_STATE;
    }

private:
    const uint8_t *mDeviceCert;
    uint16_t mDeviceCertLen;
    const uint8_t *mDevicePrivKey;
    uint16_t mDevicePrivKeyLen;
    const uint8_t *mRootCert;
    uint16_t mRootCertLen;
};

WEAVE_ERROR SimulateDeviceKeyExport(const uint8_t *deviceCert, uint16_t deviceCertLen,
                                    const uint8_t *devicePrivKey, uint16_t devicePrivKeyLen,
                                    const uint8_t *trustRootCert, uint16_t trustRootCertLen,
                                    const uint8_t *exportReq, uint16_t exportReqLen,
                                    uint8_t *exportRespBuf, uint16_t exportRespBufSize, uint16_t& exportRespLen,
                                    bool& respIsReconfig)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    SimulatedDeviceKeyExportDelegate keyExportDelegate(deviceCert, deviceCertLen, devicePrivKey, devicePrivKeyLen, trustRootCert, trustRootCertLen);
    SimulatedDeviceGroupKeyStore keyStore;
    WeaveKeyExport keyExportObj;

    keyExportObj.Init(&keyExportDelegate, &keyStore);
    keyExportObj.SetAllowedConfigs(kKeyExportSupportedConfig_All);

    exportRespLen = 0;
    respIsReconfig = false;

    // Call the key export object to process the key export request.
    err = keyExportObj.ProcessKeyExportRequest(exportReq, exportReqLen, NULL, NULL);

    if (err == WEAVE_ERROR_KEY_EXPORT_RECONFIGURE_REQUIRED)
    {
        // Call the key export object to generate a key export reconfigure message.
        err = keyExportObj.GenerateKeyExportReconfigure(exportRespBuf, exportRespBufSize, exportRespLen);
        SuccessOrExit(err);

        respIsReconfig = true;
    }
    else
    {
        SuccessOrExit(err);

        // Call the key export object to generate a key export response message.
        err = keyExportObj.GenerateKeyExportResponse(exportRespBuf, exportRespBufSize, exportRespLen);
        SuccessOrExit(err);
    }

exit:
    keyExportObj.Reset();
    return err;
}

} // namespace KeyExport
} // namespace Security
} // namespace Profiles
} // namespace Weave
} // namespace nl

#endif // HAVE_MALLOC && HAVE_FREE && HAVE_TIME_H
