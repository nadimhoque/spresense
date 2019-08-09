/****************************************************************************
 * modules/lte/altcom/api/mbedtls/cipher_setup.c
 *
 *   Copyright 2018 Sony Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Sony Semiconductor Solutions Corporation nor
 *    the names of its contributors may be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "dbg_if.h"
#include "altcom_errno.h"
#include "altcom_seterrno.h"
#include "apicmd_cipher_setup.h"
#include "apiutil.h"
#include "mbedtls/cipher.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CIPHER_SETUP_REQ_DATALEN (sizeof(struct apicmd_cipher_setup_s))
#define CIPHER_SETUP_RES_DATALEN (sizeof(struct apicmd_cipher_setupres_s))

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct cipher_setup_req_s
{
  uint32_t ctx_id;
  uint32_t info_id;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int32_t cipher_setup_request(FAR struct cipher_setup_req_s *req)
{
  int32_t                             ret;
  uint16_t                            reslen = 0;
  FAR struct apicmd_cipher_setup_s    *cmd = NULL;
  FAR struct apicmd_cipher_setupres_s *res = NULL;

  /* Allocate send and response command buffer */

  if (!altcom_mbedtls_alloc_cmdandresbuff(
    (FAR void **)&cmd, APICMDID_TLS_CIPHER_SETUP, CIPHER_SETUP_REQ_DATALEN,
    (FAR void **)&res, CIPHER_SETUP_RES_DATALEN))
    {
      return MBEDTLS_ERR_CIPHER_BAD_INPUT_DATA;
    }

  /* Fill the data */

  cmd->ctx = htonl(req->ctx_id);
  cmd->cipher_info = htonl(req->info_id);

  DBGIF_LOG1_DEBUG("[cipher_setup]ctx id: %d\n", req->ctx_id);
  DBGIF_LOG1_DEBUG("[cipher_setup]info id: %d\n", req->info_id);

  /* Send command and block until receive a response */

  ret = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)res,
                      CIPHER_SETUP_RES_DATALEN, &reslen,
                      SYS_TIMEO_FEVR);

  if (ret < 0)
    {
      DBGIF_LOG1_ERROR("apicmdgw_send error: %d\n", ret);
      goto errout_with_cmdfree;
    }

  if (reslen != CIPHER_SETUP_RES_DATALEN)
    {
      DBGIF_LOG1_ERROR("Unexpected response data length: %d\n", reslen);
      goto errout_with_cmdfree;
    }

  ret = ntohl(res->ret_code);

  DBGIF_LOG1_DEBUG("[cipher_setup res]ret: %d\n", ret);

  altcom_mbedtls_free_cmdandresbuff(cmd, res);

  return ret;

errout_with_cmdfree:
  altcom_mbedtls_free_cmdandresbuff(cmd, res);
  return MBEDTLS_ERR_CIPHER_BAD_INPUT_DATA;
}



/****************************************************************************
 * Public Functions
 ****************************************************************************/

int mbedtls_cipher_setup(mbedtls_cipher_context_t *ctx, const mbedtls_cipher_info_t *cipher_info)
{
  int32_t                   result;
  struct cipher_setup_req_s req;

  if(ctx == NULL || cipher_info == NULL)
    {
      return MBEDTLS_ERR_CIPHER_BAD_INPUT_DATA;
    }

  if (!altcom_isinit())
    {
      DBGIF_LOG_ERROR("Not intialized\n");
      altcom_seterrno(ALTCOM_ENETDOWN);
      return MBEDTLS_ERR_CIPHER_BAD_INPUT_DATA;
    }

  req.ctx_id = ctx->id;
  req.info_id = cipher_info->id;

  result = cipher_setup_request(&req);

  return result;
}

