#include "discovery.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <main.h>

#include <euicc/es10b.h>
#include <euicc/es9p.h>

static int applet_main(int argc, char **argv)
{
    int opt;
    static const char *opt_string = "s:i:h?";

    char *smds = NULL;
    char *imei = NULL;

    struct es9p_ctx es9p_ctx = {0};

    char *b64_euicc_challenge = NULL;
    char *b64_euicc_info_1 = NULL;

    struct es10b_AuthenticateServer_param es10b_AuthenticateServer_param;
    char *b64_authenticate_server_response = NULL;

    struct es11_AuthenticateClient_resp es11_AuthenticateClient_resp;

    opt = getopt(argc, argv, opt_string);
    while (opt != -1)
    {
        switch (opt)
        {
        case 's':
            smds = strdup(optarg);
            break;
        case 'i':
            imei = strdup(optarg);
            break;
        case 'h':
        case '?':
            printf("Usage: %s [OPTIONS]\r\n", argv[0]);
            printf("\t -s SM-DS Domain\r\n");
            printf("\t -i IMEI\r\n");
            printf("\t -h This help info\r\n");
            return -1;
            break;
        }
        opt = getopt(argc, argv, opt_string);
    }

    if (smds == NULL)
    {
        // smds = "prod.smds.rsp.goog";
        // smds = "lpa.live.esimdiscovery.com";
        smds = "lpa.ds.gsma.com";
    }

    es9p_ctx.euicc_ctx = &euicc_ctx;
    es9p_ctx.address = smds;

    jprint_progress("es10b_GetEUICCChallenge");
    if (es10b_GetEUICCChallenge(&euicc_ctx, &b64_euicc_challenge))
    {
        jprint_error("es10b_GetEUICCChallenge", NULL);
        return -1;
    }

    jprint_progress("es10b_GetEUICCInfo");
    if (es10b_GetEUICCInfo(&euicc_ctx, &b64_euicc_info_1))
    {
        jprint_error("es10b_GetEUICCInfo", NULL);
        return -1;
    }

    jprint_progress("es9p_InitiateAuthentication");
    if (es9p_InitiateAuthentication(&es9p_ctx, &es10b_AuthenticateServer_param, b64_euicc_challenge, b64_euicc_info_1))
    {
        jprint_error("es11_initiate_authentication", es9p_ctx.statusCodeData.message);
        return -1;
    }

    es10b_AuthenticateServer_param.matchingId = NULL;
    es10b_AuthenticateServer_param.imei = imei;
    es10b_AuthenticateServer_param.tac = NULL;

    jprint_progress("es10b_AuthenticateServer");
    if (es10b_AuthenticateServer(&euicc_ctx, &b64_authenticate_server_response, &es10b_AuthenticateServer_param))
    {
        jprint_error("es10b_AuthenticateServer", NULL);
        return -1;
    }

    jprint_progress("es11_AuthenticateClient");
    if (es11_AuthenticateClient(&es9p_ctx, &es11_AuthenticateClient_resp, b64_authenticate_server_response))
    {
        jprint_error("es11_AuthenticateClient", es11_AuthenticateClient_resp.status);
        return -1;
    }

    jprint_success((cJSON *)es11_AuthenticateClient_resp.cjson_array_result);
    return 0;
}

struct applet_entry applet_profile_discovery = {
    .name = "discovery",
    .main = applet_main,
};
