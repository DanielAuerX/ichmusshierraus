#include <HTTPClient.h>
#include <base64.h>
#include "imhr_http_client.h"
#include "imhr_secrets.h"
#include "mbedtls/md.h"

namespace imhr
{
    static const char *hvvUser = HVV_USER;
    static const char *hvvSecret = HVV_SECRET;

    // todo make static again
     String signRequest(const char *payload, const char *secret)
    {
        byte hmac[20];
        mbedtls_md_context_t ctx;
        mbedtls_md_init(&ctx);
        mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA1), 1);
        mbedtls_md_hmac_starts(&ctx, (unsigned char *)secret, strlen(secret));
        mbedtls_md_hmac_update(&ctx, (unsigned char *)payload, strlen(payload));
        mbedtls_md_hmac_finish(&ctx, hmac);
        mbedtls_md_free(&ctx);
        return base64::encode(hmac, 20);
    }

    HttpResponse sendPostRequest(const String &url, const String &body)
    {
        String signature = signRequest(body.c_str(), hvvSecret);
        HTTPClient http;
        http.begin(url);
        http.addHeader("Content-Type", "application/json");
        http.addHeader("geofox-auth-user", hvvUser);
        http.addHeader("geofox-auth-signature", signature);
        int code = http.POST((uint8_t *)body.c_str(), body.length());
        String bodyResponse = http.getString();
        http.end();
        return {code, bodyResponse};
    }
}
