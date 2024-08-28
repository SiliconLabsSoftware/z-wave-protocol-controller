/* © 2014 Silicon Laboratories Inc.
 */
/*
 * S2.c
 *
 *  Created on: Jun 25, 2015
 *      Author: aes
 */
#include "S2.h"
#include "s2_protocol.h"
#include "s2_classcmd.h"
#include "../inclusion/s2_inclusion_internal.h"
#include<string.h>
#include "ccm.h"
#include "aes_cmac.h"
#include "nextnonce.h"
#include "kderiv.h"
#include "aes.h"

#include <platform.h>
#include "ZW_classcmd.h"
#include "s2_keystore.h"
#ifdef ZWAVE_PSA_SECURE_VAULT
#include "s2_psa.h"
#endif

#ifdef SINGLE_CONTEXT
struct S2 the_context;
#endif

#define CTX_DEF struct S2* ctxt = p_context;

CTR_DRBG_CTX s2_ctr_drbg;

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wunused-function"  //TODO: Remove after merging in complete multicast support
#endif

#define AUTH_TAG_LEN 8

/*"This is the longest duration we expect a senddata callback to have.
Comes from the Z-Wave protocol spec. After this timeout, we just declare the
transmission a success, so we dont block forever."*/
#define SEND_DATA_TIMEOUT 65000

//Forwards
static void
S2_fsm_post_event(struct S2* p_context, event_t e, event_data_t* d);

static void
S2_set_peer(struct S2* p_context, const s2_connection_t* peer, const uint8_t* buf, uint16_t len);
static int
S2_span_ok(struct S2* p_context, const s2_connection_t* con);
static uint8_t
S2_register_nonce(struct S2* p_context, const uint8_t* buf, uint16_t len);
static void
S2_send_nonce_get(struct S2* p_context);
static int
S2_verify_seq(struct S2* p_context, const s2_connection_t* peer, uint8_t seq);
static void
S2_encrypt_and_send(struct S2* p_context);
static void
S2_send_nonce_report(struct S2* p_context, const s2_connection_t* conn, uint8_t flags);
static int
S2_is_peernode(struct S2* p_context, const s2_connection_t* peer);
#ifdef ZW_CONTROLLER
static void S2_send_nls_state_set(struct S2* p_context, s2_connection_t* con, bool nls_active);
static void S2_send_nls_state_get(struct S2* p_context, s2_connection_t* con);
#endif /* ZW_CONTROLLER */
static void S2_send_nls_state_report(struct S2* p_context, s2_connection_t* con);
static void S2_command_handler(struct S2* p_context, s2_connection_t* src, uint8_t* cmd, uint16_t cmd_length);

static void
next_mpan_state(struct MPAN* mpan);

static decrypt_return_code_t
S2_decrypt_msg(struct S2* p_context, s2_connection_t* conn, uint8_t* msg, uint16_t msg_len, uint8_t** plain_text,
    uint16_t* plain_text_len);
static struct SPAN*
find_span_by_node(struct S2* p_context, const s2_connection_t* con);
static int
S2_make_aad(struct S2* p_context, node_t sender, node_t receiver, uint8_t* msg, uint16_t hdr_len, uint16_t msg_len,
    uint8_t* aad, uint16_t max_size);

static void
S2_send_raw(struct S2* p_context, uint8_t* buf, uint16_t len);
static uint8_t
S2_is_mos(struct S2* p_context, node_t node_id, uint8_t clear);
static void convert_normal_to_lr_keyclass(s2_connection_t *con);
static void convert_lr_to_normal_keyclass(s2_connection_t *con);

/**
 * Send function for both singlecast and multicast.
 *
 * Whether it is singlecast or multicast is determined by the ev parameter.
 *
 * \return 1 if send succeeds. 0 if send fails due to S2 busy or parameter errors.
 */
static uint8_t
S2_send_data_all_cast(struct S2* p_context, const s2_connection_t* con, const uint8_t* buf, uint16_t len, event_t ev);

#ifdef ZWAVE_PSA_SECURE_VAULT
static uint32_t convert_key_slot_to_keyid(uint8_t slot_id)
{
  uint32_t class_id;
  switch (slot_id) {
    case 0:
      class_id = KEY_CLASS_S2_UNAUTHENTICATED;
      break;
    case 1:
      class_id = KEY_CLASS_S2_AUTHENTICATED;
      break;
    case 2:
      class_id = KEY_CLASS_S2_ACCESS;
      break;
    case 3:
      class_id = KEY_CLASS_S2_AUTHENTICATED_LR;
      break;
    case 4:
      class_id = KEY_CLASS_S2_ACCESS_LR;
      break;
    default:
      class_id = 0xFF;
      break;
  }
  return class_id;
}
#endif

/**
 * Find or allocate an mpan by group_id id no match can be found
 * we use a new entry.
 */
static struct MPAN*
find_mpan_by_group_id(struct S2* p_context, node_t owner_id, uint8_t group_id, uint8_t create_new)
{
  CTX_DEF
  uint8_t rnd[RANDLEN];
  int i;

  for (i = 0; i < MPAN_TABLE_SIZE; i++)
  {
    if ((ctxt->mpan_table[i].state != MPAN_NOT_USED) && (ctxt->mpan_table[i].group_id == group_id)
        && (ctxt->mpan_table[i].owner_id == owner_id) && ((1 << ctxt->mpan_table[i].class_id) &  ctxt->loaded_keys))
    {
      return &ctxt->mpan_table[i];
    }
  }
  if (!create_new)
  {
    return 0;
  }
  /*We need to be find an unused group handle */
  AES_CTR_DRBG_Generate(&s2_ctr_drbg, rnd);

  /*Allocate new entry if possible */
  for (i = 0; i < MPAN_TABLE_SIZE; i++)
  {
    if (ctxt->mpan_table[i].state == MPAN_NOT_USED)
    {
      break;
    }
  }

  /*Just select a random entry Note this will overwrite existing entries
   * TODO we should really select the oldest entry
   * */
  if (i == MPAN_TABLE_SIZE)
  {
    // dropping random span entry
    i = rnd[0] % MPAN_TABLE_SIZE;
  }

  ctxt->mpan_table[i].state = owner_id ? MPAN_MOS : MPAN_SET;
  ctxt->mpan_table[i].group_id = group_id;
  ctxt->mpan_table[i].owner_id = owner_id;
  ctxt->mpan_table[i].class_id = ctxt->peer.class_id; //Here we assume that peer is set...

  AES_CTR_DRBG_Generate(&s2_ctr_drbg, ctxt->mpan_table[i].inner_state);
  ;

  return &ctxt->mpan_table[i];
}

static struct SPAN  *
find_span_by_node(struct S2* p_context, const s2_connection_t* con)
{
  CTX_DEF
  uint8_t rnd[RANDLEN];
  int i;
  /* Locate existing entry */
  for (i = 0; i < SPAN_TABLE_SIZE; i++)
  {
    if (ctxt->span_table[i].state != SPAN_NOT_USED && (ctxt->span_table[i].lnode == con->l_node)
        && (ctxt->span_table[i].rnode == con->r_node))
    {
      return &ctxt->span_table[i];
    }
  }

  AES_CTR_DRBG_Generate(&s2_ctr_drbg, rnd);

  /*Allocate new entry if possible */
  for (i = 0; i < SPAN_TABLE_SIZE; i++)
  {
    if (ctxt->span_table[i].state == SPAN_NOT_USED)
    {
      break;
    }
  }

  /*Just select a random entry Note this will overwrite existing entries*/
  if (i == SPAN_TABLE_SIZE)
  {
    // dropping random span entry
    i = rnd[0] % SPAN_TABLE_SIZE;
  }

  ctxt->span_table[i].state = SPAN_NO_SEQ;
  ctxt->span_table[i].lnode = con->l_node;
  ctxt->span_table[i].rnode = con->r_node;
  ctxt->span_table[i].tx_seq = rnd[1];

  return &ctxt->span_table[i];
}

/**
 * Check if the span is synchronized.
 */
static int
S2_span_ok(struct S2* p_context, const s2_connection_t* con)
{
  CTX_DEF

  struct SPAN  *span = find_span_by_node(ctxt, con);

  if (span)
  {
    return ((span->state == SPAN_NEGOTIATED) || (span->state == SPAN_SOS_REMOTE_NONCE))
        && (span->class_id == con->class_id);
  }
  else
  {
    return 0;
  }
}

/*
 * Send nonce get to ctxt->peer
 */
static void
S2_send_nonce_get(struct S2* p_context)
{
  CTX_DEF
  static uint8_t nonce_get[] =
    { COMMAND_CLASS_SECURITY_2, SECURITY_2_NONCE_GET, 0 };

  struct SPAN  *span = find_span_by_node(ctxt, &ctxt->peer);

  assert(span);

  nonce_get[2] = span->tx_seq;
  S2_send_raw(ctxt, nonce_get, 3);
}

/**
 * Verify the sequence of the received frame.
 */
static int
S2_verify_seq(struct S2* p_context, const s2_connection_t* peer, uint8_t seq)
{
  CTX_DEF
  struct SPAN  *span = find_span_by_node(ctxt, peer);
  /* If this is a completely new entry, we will just copy seq number
     and accept it.
     To allow detection of old frames in the network, we use a window
     with more than one frame in the duplicate check. */
  if (span->state == SPAN_NO_SEQ
      || (uint8_t)(span->rx_seq - seq) >= S2_SEQ_DUPL_WINDOW_SIZE)
  {
    span->rx_seq = seq;
    return 1;
  }
  else
  {
    // Duplicate frame dropped
    return 0;
  }

#if 0
  if (ctxt->span->rx_seq < seq)
  {
    return 1;
  }
  else
  {
    if ((seq - ctxt->span->rx_seq) > (0xFF - 4))
    { //TODO verify this algorithm
      return 1;
    }
    else
    {
      return 0;
    }
  }
#endif
}

/**
 * Return node if the node in question has reported MOS
 */
static uint8_t
S2_is_node_mos(struct S2* p_context, node_t nodeid)
{
  CTX_DEF
  uint8_t i;
  for (i = 0; i < MOS_LIST_LENGTH; i++)
  {
    if (ctxt->mos_list[i].node_id == nodeid)
    {
      return 1;
    }
  }
  return 0;

}
/* Add MPAN extensions for the current ctxt->peer by checks our mpan table
 * for nodes who is reported MOS.
 *
 *
 */
static uint16_t
S2_add_mpan_extensions(struct S2* p_context, uint8_t* ext_data)
{
  CTX_DEF
  uint8_t i, k;
  uint8_t *p;
  struct MPAN* mpan;

  p = ext_data;
  k = 0;
  for (i = 0; i < MOS_LIST_LENGTH; i++)
  {
    if (ctxt->mos_list[i].node_id == ctxt->peer.r_node)
    {
      mpan = find_mpan_by_group_id(ctxt, 0, ctxt->mos_list[i].group_id, 0);
      if (!mpan)
      {
        // could not find MPAN
        continue;
      }
      k++;
      *p++ = 19;
      *p++ = S2_MSG_EXTHDR_TYPE_MPAN | S2_MSG_EXTHDR_MORE_FLAG | S2_MSG_EXTHDR_CRITICAL_FLAG;
      *p++ = ctxt->mos_list[i].group_id;
      memcpy(p, mpan->inner_state, 16);

      //Remove the node from the mos list
      ctxt->mos_list[i].node_id = 0;

    }
  }
  /*Clear the more flag for the last extension, FIXME this does not quite work
   * if we append extensions after this one */
  if (k)
  {
    ext_data[(k - 1) * 19 + 1] &= ~S2_MSG_EXTHDR_MORE_FLAG;
  }
  return k * 19;
}

/**
 * Encrypt a single cast message stored in ctxt and send it
 */
void
S2_encrypt_and_send(struct S2* p_context)
{
  CTX_DEF
  uint8_t aad[64];
  uint16_t aad_len;
  uint8_t ei_sender[RANDLEN]; //Note we are actually only using the first 16 bytes
  uint8_t ei_receiver[16];
  uint8_t nonce[16];

  uint8_t* ciphertext;

  uint8_t* ext_data;
  uint16_t hdr_len; //Length of unencrypted data
  uint16_t shdr_len; //Length of encrypted header
  uint8_t* msg;
  uint8_t n_ext;
  uint16_t msg_len;

  struct SPAN  *span = find_span_by_node(ctxt, &ctxt->peer);

  msg = ctxt->workbuf;
  msg[0] = COMMAND_CLASS_SECURITY_2;
  msg[1] = SECURITY_2_MESSAGE_ENCAPSULATION;
  msg[2] = span->tx_seq;

  msg[3] = 0;

  hdr_len = 4;
  n_ext = 0;

  /*If span is not negotiated, include senders nonce (SN) */
  ext_data = &msg[4];

  if (span->state == SPAN_SOS_REMOTE_NONCE)
  {
    AES_CTR_DRBG_Generate(&s2_ctr_drbg, ei_sender);
    memcpy(ei_receiver, span->d.r_nonce, sizeof(ei_receiver));

    next_nonce_instantiate(&span->d.rng, ei_sender, ei_receiver, ctxt->sg[ctxt->peer.class_id].nonce_key);

    span->class_id = ctxt->peer.class_id;
    span->state = SPAN_NEGOTIATED; //TODO is it better to set this on send_data complete?

    *ext_data++ = 2 + sizeof(span->d.r_nonce); //Extension length
    *ext_data++ = S2_MSG_EXTHDR_CRITICAL_FLAG | S2_MSG_EXTHDR_TYPE_SN;
    memcpy(ext_data, ei_sender, 16);
    hdr_len += 2 + 16;
    ext_data += 16;
    n_ext++;
  }

  if ((ctxt->peer.tx_options & (S2_TXOPTION_SINGLECAST_FOLLOWUP | S2_TXOPTION_FIRST_SINGLECAST_FOLLOWUP)) && ctxt->mpan)
  {

    /* If the destination is mos, then we will add the MPAN extension instead */
    if (!S2_is_node_mos(ctxt, ctxt->peer.r_node))
    {

      /* Add the MGRP header extension */
      *ext_data++ = 3;
      *ext_data++ = S2_MSG_EXTHDR_CRITICAL_FLAG | S2_MSG_EXTHDR_TYPE_MGRP;
      *ext_data++ = ctxt->mpan->group_id;
      hdr_len += 3;
      n_ext++;
    }

    if ((ctxt->peer.tx_options & S2_TXOPTION_FIRST_SINGLECAST_FOLLOWUP) && ctxt->retry == 2)
    {
      next_mpan_state(ctxt->mpan);
    }
  }

  /*Add MOS extension */
  if (ctxt->mpan && ctxt->mpan->state == MPAN_MOS)
  {
    ctxt->mpan->state = MPAN_NOT_USED;
    ctxt->mpan = 0;
    *ext_data++ = 2;
    *ext_data++ = S2_MSG_EXTHDR_TYPE_MOS;
    hdr_len += 2;
    n_ext++;
  }

  /*Insert more flag*/
  if (n_ext)
  {
    msg[3] |= SECURITY_2_MESSAGE_ENCAPSULATION_PROPERTIES1_EXTENSION_BIT_MASK;
    ext_data = &msg[4];
    while (--n_ext)
    {
      ext_data[1] |= S2_MSG_EXTHDR_MORE_FLAG;
    }
    ext_data += *ext_data;
  }

  ciphertext = &msg[hdr_len];

  /* Add the secure extensions */
  shdr_len = S2_add_mpan_extensions(ctxt, ciphertext);
  if (shdr_len)
  {
    msg[3] |=
    SECURITY_2_MESSAGE_ENCAPSULATION_PROPERTIES1_ENCRYPTED_EXTENSION_BIT_MASK;
  }

  memcpy(ciphertext + shdr_len, ctxt->buf, ctxt->length);
  aad_len = S2_make_aad(ctxt, ctxt->peer.l_node, ctxt->peer.r_node, msg, hdr_len,
      ctxt->length + shdr_len + hdr_len + AUTH_TAG_LEN, aad, sizeof(aad));
  /*TODO we should consider to roll the nonce when we have recevied in ACK*/
  next_nonce_generate(&span->d.rng, nonce); //Create the new nonce

#if defined(ZWAVE_PSA_SECURE_VAULT) && defined(ZWAVE_PSA_AES)
  size_t out_len = 0;
  uint32_t ccm_key_id = ZWAVE_CCM_TEMP_ENC_KEY_ID;
  if (ctxt->is_keys_restored == false)
  {
       /* Import key into secure vault */
     zw_wrap_aes_key_secure_vault(&ccm_key_id, ctxt->sg[ctxt->peer.class_id].enc_key, ZW_PSA_ALG_CCM);
  }
  else
  {
    /* Use secure vault for encryption using PSA APIs */
    ccm_key_id = convert_keyclass_to_derived_key_id(convert_key_slot_to_keyid(ctxt->peer.class_id), ZWAVE_KEY_TYPE_SINGLE_CAST);
  }
  zw_psa_aead_encrypt_ccm(ccm_key_id, nonce, ZWAVE_PSA_AES_NONCE_LENGTH, aad, aad_len, ciphertext,
                        ctxt->length + shdr_len, ciphertext, ctxt->length+shdr_len+ZWAVE_PSA_AES_MAC_LENGTH, &out_len);
  msg_len = out_len;
  assert(msg_len == (ctxt->length + shdr_len + ZWAVE_PSA_AES_MAC_LENGTH));
  /* Remove key from vault */
  if (ctxt->is_keys_restored == false) {
    zw_psa_destroy_key(ccm_key_id);
  }
#else
  msg_len = CCM_encrypt_and_auth(ctxt->sg[ctxt->peer.class_id].enc_key, nonce, aad, aad_len, ciphertext,
        ctxt->length + shdr_len);
#endif

  assert(msg_len > 0);
  S2_send_raw(ctxt, msg, msg_len + hdr_len);
}

static inline uint8_t
bigint_add(uint8_t *r, const uint8_t *a, const uint8_t *b, uint16_t len)
{
  uint16_t i;
  uint16_t tmp = 0;
  for (i=0; i<len; i++) 
  {
    tmp = ((uint16_t)a[i]) + ((uint16_t)b[i]) + tmp;
    r[i] = tmp & 0xff;
    tmp >>= 8;
  }
  return (uint8_t)tmp;
}

static void
next_mpan_state(struct MPAN * mpan)
{
  static const uint8_t one[] =
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
  //TODO check that it will roll around
  bigint_add(mpan->inner_state, mpan->inner_state, one, 16);
}

void
S2_encrypt_and_send_multi(struct S2* p_context)
{
  CTX_DEF
  uint8_t aad[64];
  uint16_t aad_len;
  uint8_t nonce[16];
  uint8_t* ciphertext;
  uint16_t hdr_len;
  uint8_t* msg;
  uint16_t msg_len;
  event_data_t e;
  msg = ctxt->workbuf;
  msg[0] = COMMAND_CLASS_SECURITY_2;
  msg[1] = SECURITY_2_MESSAGE_ENCAPSULATION;
  msg[2] = 0xFF; //TODO
  msg[3] = SECURITY_2_MESSAGE_ENCAPSULATION_PROPERTIES1_EXTENSION_BIT_MASK;

  /* Add the encrypted header extension */
  msg[4] = 3;
  msg[5] = S2_MSG_EXTHDR_CRITICAL_FLAG | S2_MSG_EXTHDR_TYPE_MGRP;
  msg[6] = ctxt->mpan->group_id;

  hdr_len = 4 + 3;

  ciphertext = &msg[hdr_len];

  memcpy(ciphertext, ctxt->buf, ctxt->length);

  aad_len = S2_make_aad(ctxt, ctxt->peer.l_node, ctxt->peer.r_node, msg, hdr_len, ctxt->length + hdr_len + AUTH_TAG_LEN,
      aad, sizeof(aad));

#if defined(ZWAVE_PSA_SECURE_VAULT) && defined(ZWAVE_PSA_AES)
    uint32_t key_id = ZWAVE_ECB_TEMP_ENC_KEY_ID;
    /* Import key into secure vault */
    zw_wrap_aes_key_secure_vault(&key_id, ctxt->sg[ctxt->mpan->class_id].mpan_key, ZW_PSA_ALG_ECB_NO_PAD);
    zw_psa_aes_ecb_encrypt(key_id, ctxt->mpan->inner_state, nonce);
    /* Remove key from vault */
    zw_psa_destroy_key(key_id);
#else
  AES128_ECB_encrypt(ctxt->mpan->inner_state, ctxt->sg[ctxt->mpan->class_id].mpan_key, nonce);
#endif

  next_mpan_state(ctxt->mpan);

#if defined(ZWAVE_PSA_SECURE_VAULT) && defined(ZWAVE_PSA_AES)
  //////////////////////////////////////////////
  size_t out_len = 0;
  key_id = ZWAVE_CCM_TEMP_ENC_KEY_ID;
  if (ctxt->is_keys_restored == false)
  {
       /* Import key into secure vault */
    zw_wrap_aes_key_secure_vault(&key_id, ctxt->sg[ctxt->mpan->class_id].enc_key, ZW_PSA_ALG_CCM);
  }
  else
  {
    /* Use secure vault for encryption using PSA APIs */
    key_id = convert_keyclass_to_derived_key_id(convert_key_slot_to_keyid(ctxt->mpan->class_id), ZWAVE_KEY_TYPE_SINGLE_CAST);
  }
  zw_psa_aead_encrypt_ccm(key_id, nonce, ZWAVE_PSA_AES_NONCE_LENGTH, aad, aad_len, ciphertext,
                                     ctxt->length, ciphertext, ctxt->length+ZWAVE_PSA_AES_MAC_LENGTH, &out_len);
  msg_len = out_len;
  assert(msg_len == (ctxt->length + ZWAVE_PSA_AES_MAC_LENGTH));
  /* Remove key from vault */
  if (ctxt->is_keys_restored == false) {
    zw_psa_destroy_key(key_id);
  }
#else
  msg_len = CCM_encrypt_and_auth(ctxt->sg[ctxt->mpan->class_id].enc_key, nonce, aad, aad_len, ciphertext, ctxt->length);
#endif

  assert(msg_len > 0);

  if (S2_send_frame_multi(ctxt, &ctxt->peer, msg, msg_len + hdr_len))
  {
    //TX seq?
  }
  else
  {
    e.d.tx.status = S2_TRANSMIT_COMPLETE_FAIL;
    S2_fsm_post_event(ctxt, SEND_DONE, &e);
  }
}

void
S2_send_frame_done_notify(struct S2* p_context, s2_tx_status_t status, uint16_t tx_time)
{
  CTX_DEF
  event_data_t e;
  e.d.tx.status = status;
  e.d.tx.time = tx_time;
  S2_fsm_post_event(ctxt, SEND_DONE, &e);
}

uint8_t S2_is_busy(struct S2* p_context)
{
  CTX_DEF
  if(ctxt->inclusion_state != S2_INC_IDLE)
  {
    return 1;
  }

  if( (ctxt->fsm != IDLE) && (ctxt->fsm != IS_MOS_WAIT_REPLY) )
  {
    return 1;
  }

  return 0;
}

void S2_free_mpan(struct S2* p_context, node_t owner_id, uint8_t group_id) {
  CTX_DEF
  // Search for a MPAN with the Group ID / owner ID, and if found, set it back to NOT USED.
  for (uint8_t i = 0; i < MPAN_TABLE_SIZE; i++) {
    if ((ctxt->mpan_table[i].group_id == group_id)
        && (ctxt->mpan_table[i].owner_id == owner_id)) {
      ctxt->mpan_table[i].state = MPAN_NOT_USED;
      return;
    }
  }
}

/**
 * Wrapper function to send_data, which increases tx_seq and guarantees a
 * SendDone event.
 */
static void
S2_send_raw(struct S2* p_context, uint8_t* buf, uint16_t len)
{
  CTX_DEF
  event_data_t e;

  if (S2_send_frame(ctxt, &ctxt->peer, buf, len))
  {
    struct SPAN  *span = find_span_by_node(ctxt, &ctxt->peer);
    span->tx_seq++;
  }
  else
  {
    e.d.tx.status = S2_TRANSMIT_COMPLETE_FAIL;
    S2_fsm_post_event(ctxt, SEND_DONE, &e);
  }
}

/**
 * Returns true if we are MOS with node_id
 * \param clear if set the mos state will be
 * cleared
 */
static uint8_t
S2_is_mos(struct S2* p_context, node_t node_id, uint8_t clear)
{
  CTX_DEF
  uint8_t i;
  for (i = 0; i < MPAN_TABLE_SIZE; i++)
  {
    if ((ctxt->mpan_table[i].owner_id == node_id) && (ctxt->mpan_table[i].state == MPAN_MOS))
    {
      if (clear)
      {
        ctxt->mpan_table[i].state = MPAN_NOT_USED;
      }
      return 1;;
    }
  }
  return 0;
}
/**
 * Send calculate our part of a new nonce and
 * send nonce-report, it checks if we are MOS
 * or SOS before sending.
 *
 * flags 1 : force_new_nonce
 * flags 2 : mos
 */
static void
S2_send_nonce_report(struct S2* p_context, const s2_connection_t* conn, uint8_t flags)
{
  CTX_DEF
  struct SPAN  *span;
  uint8_t rnd[RANDLEN];

  static uint8_t nonce_report[2 + 2 + sizeof(span->d.r_nonce)];

  span = find_span_by_node(ctxt, conn);
  nonce_report[0] = COMMAND_CLASS_SECURITY_2;
  nonce_report[1] = SECURITY_2_NONCE_REPORT;
  nonce_report[3] = flags;
  nonce_report[2] = span->tx_seq;

  if (flags & SECURITY_2_NONCE_REPORT_PROPERTIES1_SOS_BIT_MASK)
  {
    span->state = SPAN_SOS_LOCAL_NONCE;
    AES_CTR_DRBG_Generate(&s2_ctr_drbg, rnd);
    memcpy(span->d.r_nonce, rnd, 16);
    memcpy(&nonce_report[4], span->d.r_nonce, sizeof(span->d.r_nonce));
  }

  span->tx_seq++;
  /*Return code is ignored here */
  S2_send_frame_no_cb(ctxt, conn, nonce_report, nonce_report[3] & SECURITY_2_NONCE_REPORT_PROPERTIES1_SOS_BIT_MASK ? 20 : 4);
}

static void
S2_set_node_mos(struct S2* p_context, node_t node)
{
  CTX_DEF
  uint8_t i;
  if ((ctxt->fsm == VERIFYING_DELIVERY || ctxt->fsm == SENDING_MSG)
      && (ctxt->peer.tx_options & (S2_TXOPTION_SINGLECAST_FOLLOWUP | S2_TXOPTION_FIRST_SINGLECAST_FOLLOWUP))
      && ctxt->mpan)
  {
    for (i = 0; i < MOS_LIST_LENGTH; i++)
    {
      if (ctxt->mos_list[i].node_id == 0)
      {
        ctxt->mos_list[i].group_id = ctxt->mpan->group_id;
        ctxt->mos_list[i].node_id = node;
        break;
      }
    }
  }
}

static uint8_t
S2_register_nonce(struct S2* p_context, const uint8_t* buf, uint16_t len)
{
  CTX_DEF
  struct SPAN  *span;

  if(!S2_verify_seq(ctxt, &ctxt->peer, buf[2])) {
    return 0;
  }

  span = find_span_by_node(ctxt, &ctxt->peer);

  if (len >= (4 + 16) && (buf[3] & SECURITY_2_NONCE_REPORT_PROPERTIES1_SOS_BIT_MASK))
  {
    memcpy(span->d.r_nonce, &buf[4], sizeof(span->d.r_nonce));
    span->state = SPAN_SOS_REMOTE_NONCE;
  }

  /*Register MOS, but only if we are expecting it */
  if ((buf[3] & SECURITY_2_NONCE_REPORT_PROPERTIES1_MOS_BIT_MASK) && (len >= 3))
  {
    S2_set_node_mos(ctxt,ctxt->peer.r_node);
  }

  return buf[3];
}

//Todo: Get this macro from ZW_transport_api.h once we have updated gateway
#define LOWEST_LONG_RANGE_NODE_ID  0x0100

/*
 * Generate Additional authentication data (AAD)
 * \param msg pointer to start of encapsulated message, ie. the first byte is COMMAND_CLASS_SECURITY_2 ...
 * \param hdr_len the length of the security header, ie. the offset of the first ciphertext byte
 * \param msg_len the total length of the S2 ENCAP message
 * \param aad buffer to write the aad into.
 * \param max_size the size of the aad buffer.
 * \return the number of bytes written into the AAD. 0 indicates that the buffer was not big enough.
 */
static int
S2_make_aad(struct S2* p_context, node_t sender, node_t receiver, uint8_t* msg, uint16_t hdr_len, uint16_t msg_len,
    uint8_t* aad, uint16_t max_size)
{
  CTX_DEF
  if (max_size < (hdr_len - 2 + 8))
  {
    return 0;
  }

  uint32_t i = 0;

  if((LOWEST_LONG_RANGE_NODE_ID <= sender) || (LOWEST_LONG_RANGE_NODE_ID <= receiver))
  {
    //Use 16-bit nodeIDs for long range communication
    aad[i++] = (sender >> 8) & 0xFF;
    aad[i++] = (sender >> 0) & 0xFF;
    aad[i++] = (receiver >> 8) & 0xFF;
    aad[i++] = (receiver >> 0) & 0xFF;
  }
  else
  {
    //Keep 8-bit nodeIDs for Classic communication so
    //all legacy products can be supported.
    aad[i++] = sender & 0xFF;
    aad[i++] = receiver & 0xFF;
  }

  /* Convert from platform byte order to big endian */
  aad[i++] = (ctxt->my_home_id >> 24) & 0xFF;
  aad[i++] = (ctxt->my_home_id >> 16) & 0xFF;
  aad[i++] = (ctxt->my_home_id >> 8)  & 0xFF;
  aad[i++] = (ctxt->my_home_id >> 0)  & 0xFF;
  aad[i++] = (msg_len >> 8) & 0xFF;
  aad[i++] = (msg_len >> 0) & 0xFF;

  memcpy(&aad[i], &msg[2], hdr_len - 2);
  return i + hdr_len - 2;
}


/* Decrypt message
 * emits AuthOK or auth fail
 *  */
static decrypt_return_code_t
S2_decrypt_msg(struct S2* p_context, s2_connection_t* conn,
    uint8_t* msg, uint16_t msg_len, uint8_t** plain_text,
    uint16_t* plain_text_len)
{
  CTX_DEF
  uint8_t aad_buf[64]; //We could reduce this spec says min 30 bytes
  uint8_t nonce[16];
  uint8_t* aad;
  uint16_t aad_len;
  uint8_t flags;
  uint8_t* ciphertext;
  uint16_t ciphertext_len;
  uint16_t decrypt_len;

  uint8_t* ext_data;
  uint8_t ext_len;
  uint16_t hdr_len;
  struct SPAN* span;
  struct MPAN* mpan;
  uint8_t r_nonce[16] = { 0 };
  uint8_t s_nonce[16] = { 0 };
  uint8_t i;

  hdr_len = 4;
  decrypt_len = 0;
  *plain_text = 0;
  *plain_text_len = 0;

  flags = msg[3];
  if (msg_len < (hdr_len + AUTH_TAG_LEN))
  {
    goto parse_fail;
  }

  mpan = 0;
  if (conn->rx_options & S2_RXOPTION_MULTICAST)
  {
    span = 0;
  }
  else
  {
    /* Verify sequence */
    if (!S2_verify_seq(ctxt, conn, msg[2]))
    {
      return SEQUENCE_FAIL;
    }

    span = find_span_by_node(ctxt, conn);
  }

  /* Parse clear text extensions */
  if (flags & SECURITY_2_MESSAGE_ENCAPSULATION_PROPERTIES1_EXTENSION_BIT_MASK)
  {
    ext_data = &msg[4];
    do
    {
      ext_len = ext_data[0];
      hdr_len += ext_len;

      if (msg_len < (hdr_len + AUTH_TAG_LEN)
          || 0 == ext_len)
      {
        goto parse_fail;
      }

      switch (ext_data[1] & S2_MSG_EXTHDR_TYPE_MASK)
      {
      case S2_MSG_EXTHDR_TYPE_SN:
        /*We only update SPAN if we expect an update */
        if (span && span->state == SPAN_SOS_LOCAL_NONCE && (ext_len == (2 + sizeof(span->d.r_nonce))))
        { /*Save the nonces */
          memcpy(s_nonce, &ext_data[2], 16);
          memcpy(r_nonce, span->d.r_nonce, 16);
          span->state = SPAN_INSTANTIATE;

          next_nonce_instantiate(&span->d.rng, s_nonce, r_nonce, ctxt->sg[span->class_id].nonce_key);
        }
        break;
      case S2_MSG_EXTHDR_TYPE_MGRP:
        if (ext_len != 3)
        {
          goto parse_fail;
        }
        /*Only create new MPAN if this was a single cast followup*/
        mpan = find_mpan_by_group_id(ctxt, conn->r_node, ext_data[2], (conn->rx_options & S2_RXOPTION_MULTICAST) == 0);
        break;
      case S2_MSG_EXTHDR_TYPE_MOS:
        if (ext_len != 2)
        {
          goto parse_fail;
        }
        S2_set_node_mos(ctxt,conn->r_node);
        break;
      default:
        if (ext_data[1] & S2_MSG_EXTHDR_CRITICAL_FLAG)
        { //Unsupported critical option
          goto parse_fail;
        }
      }

      if (ext_data[1] & S2_MSG_EXTHDR_MORE_FLAG)
      {
        ext_data += ext_len;
      }
      else
      {
        break;
      }
    }
    while (1);
  }

  if (conn->rx_options & S2_RXOPTION_MULTICAST)
  {
    if (mpan == 0 || mpan->state != MPAN_SET)
    {
      goto auth_fail;
    }
    conn->l_node = mpan->group_id; //Used to form the aad
  }
  else
  {
    if (span->state != SPAN_NEGOTIATED && span->state != SPAN_INSTANTIATE)
    {
      // Unexpected span state
      goto auth_fail;
    }
  }

  ciphertext = &msg[hdr_len];
  ciphertext_len = msg_len - hdr_len;

  aad = &aad_buf[0];

  aad_len = S2_make_aad(ctxt, conn->r_node, conn->l_node, msg, hdr_len, msg_len, aad, sizeof(aad_buf));

  if (span)
  {
    /*Single cast decryption */

    /*In this state we don't know which class_id was used to encrypt the frame, so
     * we will try de-crypting with all our classes */

    /*Check the fsm before using the workbuf */
    if (ctxt->fsm == IDLE && span->state == SPAN_INSTANTIATE)
    {
      memcpy(ctxt->workbuf, ciphertext, ciphertext_len);
    }

    for (i = 0; i < N_SEC_CLASS; i++)
    {
      /*Only decrypt with a key which is loaded */
      if (ctxt->loaded_keys & (1 << span->class_id))
      {
        next_nonce_generate(&span->d.rng, nonce);

#if defined(ZWAVE_PSA_SECURE_VAULT) && defined(ZWAVE_PSA_AES)
       size_t out_len;
       zw_status_t status;
       uint32_t ccm_key_id = ZWAVE_CCM_TEMP_DEC_KEY_ID;
       if (ctxt->is_keys_restored == false) {
         /* Import key into secure vault */
         zw_wrap_aes_key_secure_vault(&ccm_key_id, ctxt->sg[span->class_id].enc_key, ZW_PSA_ALG_CCM);
       } else {
         /* Use secure vault for encryption using PSA APIs */
         ccm_key_id = convert_keyclass_to_derived_key_id(convert_key_slot_to_keyid(span->class_id), ZWAVE_KEY_TYPE_SINGLE_CAST);
       }
        status = zw_psa_aead_decrypt_ccm(ccm_key_id, nonce, ZWAVE_PSA_AES_NONCE_LENGTH, aad, aad_len,
                                ciphertext, ciphertext_len, ciphertext, ciphertext_len+ZWAVE_PSA_AES_MAC_LENGTH, &out_len);
        if (status == ZW_PSA_ERROR_INVALID_SIGNATURE) {
          decrypt_len = 0;
        } else {
          decrypt_len = out_len;
        }
        /* Remove key from vault */
        if (ctxt->is_keys_restored == false) {
          zw_psa_destroy_key(ccm_key_id);
        }
#else
        decrypt_len = CCM_decrypt_and_auth(ctxt->sg[span->class_id].enc_key, nonce, aad, aad_len, ciphertext,
            ciphertext_len);
#endif

        if (decrypt_len)
        {
          span->state = SPAN_NEGOTIATED;
          conn->class_id = span->class_id;

          if (mpan)
          { //This means that a MGRP extension was included in the message
            /* If  it was a multicast followup, set rx option */
            conn->rx_options |= S2_RXOPTION_FOLLOWUP;
            if (mpan->state == MPAN_MOS)
            {
              event_data_t e;
              e.con = conn;
              ctxt->mpan = mpan;
              S2_fsm_post_event(ctxt, GOT_ENC_MSG_MOS,&e);

              //S2_send_nonce_report(ctxt, conn, SECURITY_2_NONCE_REPORT_PROPERTIES1_MOS_BIT_MASK);
            }
            else
            {
              next_mpan_state(mpan);
            }
          }
          break;
        }
      }

      if (ctxt->fsm != IDLE || span->state == SPAN_NEGOTIATED)
      {
        /*We were not able to backup the cipher-text so we will not be able to decrypt the message*/
        goto auth_fail;
      }

      /*try the next security class */
      span->class_id++;
      if (span->class_id >= N_SEC_CLASS)
      {
        span->class_id = 0;
      }

      //Restore the ciphertext
      memcpy(ciphertext, ctxt->workbuf, ciphertext_len);

      /*reset prng to the negotiated state with the right new test key */
      next_nonce_instantiate(&span->d.rng, s_nonce, r_nonce, ctxt->sg[span->class_id].nonce_key);
    }
  }
  else
  {
    /*Multicast decryption*/
#ifdef ZWAVE_PSA_AES
    uint32_t key_id = ZWAVE_CCM_TEMP_ENC_KEY_ID;
    zw_wrap_aes_key_secure_vault(&key_id, ctxt->sg[mpan->class_id].mpan_key, ZW_PSA_ALG_ECB_NO_PAD);
    /* Import key into secure vault */
    zw_psa_aes_ecb_encrypt(key_id, mpan->inner_state, nonce);
    /* Remove key from vault */
    zw_psa_destroy_key(key_id);
#else
    AES128_ECB_encrypt(mpan->inner_state, ctxt->sg[mpan->class_id].mpan_key, nonce);
#endif
    next_mpan_state(mpan);

#if defined(ZWAVE_PSA_SECURE_VAULT) && defined(ZWAVE_PSA_AES)
        size_t out_len = 0;
        key_id = ZWAVE_CCM_TEMP_DEC_KEY_ID;
        zw_status_t status;
        /* Import key into secure vault */
        zw_wrap_aes_key_secure_vault(&key_id, ctxt->sg[mpan->class_id].enc_key, ZW_PSA_ALG_CCM);
        /* Use secure vault for decryption using PSA APIs */
        status = zw_psa_aead_decrypt_ccm(key_id, nonce, ZWAVE_PSA_AES_NONCE_LENGTH, aad, aad_len,
                                ciphertext, ciphertext_len, ciphertext, ciphertext_len+ZWAVE_PSA_AES_MAC_LENGTH, &out_len);
        if (status == ZW_PSA_ERROR_INVALID_SIGNATURE) {
          decrypt_len = 0;
        } else {
          decrypt_len = out_len;
        }
        /* Remove key from vault */
        zw_psa_destroy_key(key_id);
#else
    decrypt_len = CCM_decrypt_and_auth(ctxt->sg[mpan->class_id].enc_key, nonce, aad, aad_len, ciphertext,
        ciphertext_len);
#endif
    conn->class_id = mpan->class_id;
  }

  if (decrypt_len == 0 || aad_len == 0)
  {
    goto auth_fail;
  }

  hdr_len = 0;
  /* Parse encrypted extensions */
  if (flags & SECURITY_2_MESSAGE_ENCAPSULATION_PROPERTIES1_ENCRYPTED_EXTENSION_BIT_MASK)
  {
    ext_data = ciphertext;
    do
    {
      ext_len = ext_data[0];
      hdr_len += ext_len;

      if (hdr_len > decrypt_len
          || 0 == ext_len)
      {
        goto parse_fail;
      }

      switch (ext_data[1] & S2_MSG_EXTHDR_TYPE_MASK)
      {
      case S2_MSG_EXTHDR_TYPE_MPAN:
        if (conn->rx_options & S2_RXOPTION_MULTICAST)
        {
          /* This extension is only allowed in singlecast messages, drop it */
          goto parse_fail;
        }
        if (ext_len != 19)
        {
          goto parse_fail;
        }
        mpan = find_mpan_by_group_id(ctxt, conn->r_node, ext_data[2],1);
        memcpy(mpan->inner_state, &ext_data[3], 16);
        mpan->state = MPAN_SET;
        mpan->class_id = span->class_id;
        /* If a new mpan was created and it wasn't a multicast, then it was a multicast followup */
        if ((conn->rx_options & S2_RXOPTION_MULTICAST) == 0)
        {
          conn->rx_options |= S2_RXOPTION_FOLLOWUP;
        }
        break;
      default:
        if (ext_data[1] & S2_MSG_EXTHDR_CRITICAL_FLAG)
        { //Unsupported critical option
          goto parse_fail;
        }
      }

      if (ext_data[1] & S2_MSG_EXTHDR_MORE_FLAG)
      {
        ext_data += ext_len;
      }
      else
      {
        break;
      }
    }
    while (1);
  }

  *plain_text = ciphertext + hdr_len;
  *plain_text_len = decrypt_len - hdr_len;
  return AUTH_OK;

parse_fail:

  return PARSE_FAIL;

auth_fail:
  if (mpan)
  {
    mpan->state = MPAN_MOS;
  }

  if (span)
  {
    span->state = SPAN_SOS; //Just invalidate the span
  }

  /*Send nonce report if this is not a multicast*/
  if ((conn->rx_options & S2_RXOPTION_MULTICAST) == 0)
  {
    S2_send_nonce_report(ctxt, conn,
        mpan ?
            (SECURITY_2_NONCE_REPORT_PROPERTIES1_SOS_BIT_MASK | SECURITY_2_NONCE_REPORT_PROPERTIES1_MOS_BIT_MASK) :
            SECURITY_2_NONCE_REPORT_PROPERTIES1_SOS_BIT_MASK);
  }

  return AUTH_FAIL;

}

/*Return true if the node is the same node as we are currently handling*/
int
S2_is_peernode(struct S2* p_context, const s2_connection_t* peer)
{
  CTX_DEF
  return peer->l_node == ctxt->peer.l_node && peer->r_node == ctxt->peer.r_node;
}

/*
 * Set the peer and message data
 */
static void
S2_set_peer(struct S2* p_context, const s2_connection_t* peer, const uint8_t* buf, uint16_t len)
{
  CTX_DEF
  ctxt->peer = *peer;
  ctxt->buf = buf; //TODO decide if we want a local copy?
  ctxt->length = len;
}

/***************** PUBLIC functions ********************/
/* Send S2 encrypted frame
 * \param dst Destination nodeid. Security scheme as 0=UNAUTH, 1=AUTH, 2=ACCES, (3=Sec0, not allowed here)
 *
 */
uint8_t
S2_send_data(struct S2* p_context, s2_connection_t* dst, const uint8_t* buf, uint16_t len)
{
  CTX_DEF
  #ifdef ZW_CONTROLLER
  if (IS_LR_NODE(dst->r_node)) {
    convert_normal_to_lr_keyclass(dst);
  }
  #endif
  return S2_send_data_all_cast(ctxt, dst, buf, len, SEND_MSG);
}

uint8_t
S2_is_send_data_busy(struct S2* p_context)
{
  CTX_DEF

  return (ctxt->fsm != IDLE) && (ctxt->fsm != IS_MOS_WAIT_REPLY);
}

void
S2_init_prng(void)
{
  uint8_t entropy[32] = { 0};

  S2_get_hw_random(entropy, sizeof(entropy));
  AES_CTR_DRBG_Instantiate(&s2_ctr_drbg, entropy, NULL);
}

struct S2*
S2_init_ctx(uint32_t home)
{
  struct S2* ctx;

#ifdef SINGLE_CONTEXT
  ctx = &the_context;
#else
  ctx = malloc(sizeof(struct S2));
  if (!ctx)
  {
    return 0;
  }
#endif
  memset(ctx, 0, sizeof(struct S2));

  ctx->my_home_id = home;
  ctx->loaded_keys = 0;

  ctx->fsm = IDLE;
  ctx->is_keys_restored = false;
  s2_restore_keys(ctx, false);

  return ctx;
}

uint8_t
S2_network_key_update(struct S2 *p_context, uint32_t key_id, security_class_t class_id, const network_key_t net_key,
    uint8_t temp_key_expand, __attribute__((unused)) bool make_keys_persist_se)
{
  CTX_DEF
  if (class_id >= N_SEC_CLASS)
  {
    return 0;
  }
  if (temp_key_expand)
  {
    tempkey_expand(key_id, net_key, ctxt->sg[class_id].enc_key, ctxt->sg[class_id].nonce_key, ctxt->sg[class_id].mpan_key);
  }
  else
  {
    networkkey_expand(key_id, net_key, ctxt->sg[class_id].enc_key, ctxt->sg[class_id].nonce_key, ctxt->sg[class_id].mpan_key);
#ifdef ZWAVE_PSA_SECURE_VAULT
    if (make_keys_persist_se) {
      uint32_t ccm_key_id = convert_keyclass_to_derived_key_id(convert_key_slot_to_keyid(class_id), ZWAVE_KEY_TYPE_SINGLE_CAST);
      assert((ccm_key_id >= ZWAVE_PSA_KEY_ID_MIN) && (ccm_key_id <= ZWAVE_PSA_KEY_ID_MAX));
      zw_wrap_aes_key_secure_vault(&ccm_key_id, ctxt->sg[class_id].enc_key, ZW_PSA_ALG_CCM);

      ccm_key_id = convert_keyclass_to_derived_key_id(convert_key_slot_to_keyid(class_id), ZWAVE_KEY_TYPE_MULTI_CAST);
      assert((ccm_key_id >= ZWAVE_PSA_KEY_ID_MIN) && (ccm_key_id <= ZWAVE_PSA_KEY_ID_MAX));
      zw_wrap_aes_key_secure_vault(&ccm_key_id, ctxt->sg[class_id].mpan_key, ZW_PSA_ALG_CCM);
      ctxt->is_keys_restored = true;
    }
#endif /*#ifdef ZWAVE_PSA_SECURE_VAULT*/
  }

  ctxt->loaded_keys |= 1 << class_id;
  return 1;
}

void
S2_destroy(struct S2* p_context)
{
  CTX_DEF
  memset(ctxt, 0, sizeof(struct S2));
#ifndef SINGLE_CONTEXT
  free(ctxt);
#endif
}


void
S2_application_command_handler(struct S2* p_context, s2_connection_t* src, uint8_t* buf, uint16_t len)
{
  CTX_DEF
  uint8_t *plain_text;
  uint16_t plain_text_len;
  decrypt_return_code_t rc;
  event_data_t d;

  d.d.buf.buffer = buf;
  d.d.buf.len = len;
  d.con = src;

  switch (buf[1])
  {
  case SECURITY_2_NONCE_GET:
    if ((src->rx_options & S2_RXOPTION_MULTICAST) != S2_RXOPTION_MULTICAST)
    {
      if( (len >=3) && S2_verify_seq(ctxt, src,buf[2]) ) {
        S2_send_nonce_report(ctxt,src,SECURITY_2_NONCE_REPORT_PROPERTIES1_SOS_BIT_MASK);
      }
    }
    break;
  case SECURITY_2_NONCE_REPORT:
    S2_fsm_post_event(ctxt, GOT_NONCE_REPORT, &d);
    ;
    break;
  case SECURITY_2_MESSAGE_ENCAPSULATION:
    rc = S2_decrypt_msg(ctxt, src, buf, len, &plain_text, &plain_text_len);
    if (rc == AUTH_OK)
    {
      S2_command_handler(ctxt, src, plain_text, plain_text_len);
    }
    else if (rc == AUTH_FAIL)
    {
      S2_fsm_post_event(ctxt, GOT_BAD_ENC_MSG, &d);
      s2_inclusion_decryption_failure(ctxt,src);
    }
    else
    {
      assert(rc == SEQUENCE_FAIL || rc == PARSE_FAIL); // decrypt error
    }
    break;
  default:
    /*
     * If S2 is busy, ctxt->buf may be in use for sending an encrypted message.
     * KEX_FAIL is an exception. Must be passed to the inclusion fsm to abort all S2 action.
     */
    if((ctxt->fsm != IDLE) && (buf[1] != KEX_FAIL)) return;

    if ((src->rx_options & S2_RXOPTION_MULTICAST) != S2_RXOPTION_MULTICAST)
    {
      ctxt->buf = buf; //TODO is this a good idea?
      ctxt->length = len;
      src->class_id = UNENCRYPTED_CLASS;
      s2_inclusion_post_event(ctxt,src);
    }
  }

}

static void S2_command_handler(struct S2* p_context, s2_connection_t* src, uint8_t* cmd, uint16_t cmd_length)
{
  CTX_DEF
  event_data_t d;

  d.d.buf.buffer = cmd;
  d.d.buf.len = cmd_length;
  d.con = src;

  uint8_t n_commands_supported;
  const uint8_t* classes;

  S2_fsm_post_event(ctxt, GOT_ENC_MSG, &d);
  if (cmd_length)
  {
    if (cmd[0] == COMMAND_CLASS_SECURITY_2 &&
       (cmd[1] != SECURITY_2_COMMANDS_SUPPORTED_REPORT))
    {
      if(src->rx_options & S2_RXOPTION_MULTICAST)
      {
        //S2 encrypted multi-cast frames shouln't exist.
        return;
      }

      switch(cmd[1])
      {
        case SECURITY_2_COMMANDS_SUPPORTED_GET_V2:        
          ctxt->u.commands_sup_report_buf[0] = COMMAND_CLASS_SECURITY_2;
          ctxt->u.commands_sup_report_buf[1] = SECURITY_2_COMMANDS_SUPPORTED_REPORT;

          S2_get_commands_supported(src->l_node,src->class_id, &classes, &n_commands_supported);

          if (n_commands_supported + 2 > sizeof(ctxt->u.commands_sup_report_buf))
          {
            return;
          }
          memcpy(&ctxt->u.commands_sup_report_buf[2], classes, n_commands_supported);
          /*TODO If ctxt->fsm is busy the report is not going to be sent*/
          S2_send_data(ctxt, src, ctxt->u.commands_sup_report_buf, n_commands_supported + 2);
          break;
        case NLS_STATE_GET_V2:
          S2_send_nls_state_report(p_context, src);
          break;
        case NLS_STATE_SET_V2:
          p_context->nls_state = cmd[SECURITY_2_V2_NLS_STATE_SET_STATE_POS];
          break;
#ifdef ZW_CONTROLLER
        case NLS_STATE_REPORT_V2:
          S2_notify_nls_state_report(src->l_node, src->class_id,
                                     cmd[SECURITY_2_V2_NLS_STATE_REPORT_CAPABILITY_FIELD],
                                     cmd[SECURITY_2_V2_NLS_STATE_REPORT_STATE_FIELD]);
          break;
        case NLS_NODE_LIST_GET_V2:
          S2_nls_node_list_get(src->l_node, src->class_id, cmd[SECURITY_2_V2_NLS_NODE_LIST_GET_REQUEST_POS]);
          break;
        case NLS_NODE_LIST_REPORT_V2:
          S2_nls_node_list_report(src->l_node, src->class_id,
                                  cmd[SECURITY_2_V2_NLS_NODE_LIST_REPORT_LAST_NODE_POS],
                                  (uint16_t) (cmd[SECURITY_2_V2_NLS_NODE_LIST_REPORT_NODE_ID_MSB_POS] << 8 | cmd[SECURITY_2_V2_NLS_NODE_LIST_REPORT_NODE_ID_LSB_POS]),
                                  cmd[SECURITY_2_V2_NLS_NODE_LIST_REPORT_GRANTED_KEYS_POS],
                                  cmd[SECURITY_2_V2_NLS_NODE_LIST_REPORT_NLS_STATE_POS])
          break;
#endif // ZW_CONTROLLER
        default:
          /* Don't validate inclusion_peer.l_node as it may not be initialized yet due to early start */
          ctxt->buf = cmd;
          ctxt->length = cmd_length;
          //Default just send the command to the inclusion fsm
          s2_inclusion_post_event(ctxt,src);
          break;
      }
    }
    else
    {
  #ifdef ZW_CONTROLLER
      /* Convert LR key classes to normal before passing out via external API */
      if (IS_LR_NODE(src->r_node)) {
        convert_lr_to_normal_keyclass(src);
      }
  #endif
      S2_msg_received_event(ctxt, src, cmd, cmd_length);
    }
  }
}

void
S2_timeout_notify(struct S2* p_context)
{
  CTX_DEF
  S2_fsm_post_event(ctxt, TIMEOUT, NULL);
}

static void
S2_post_send_done_event(struct S2* p_context, s2_tx_status_t status)
{
  CTX_DEF

  s2_inclusion_send_done(ctxt, (status == S2_TRANSMIT_COMPLETE_OK) || (status == S2_TRANSMIT_COMPLETE_VERIFIED));
  S2_send_done_event(ctxt, status);
}

static void emit_S2_synchronization_event(sos_event_reason_t reason, event_data_t* d)
{
  S2_resynchronization_event(d->con->r_node, reason, d->d.buf.buffer[2], d->con->l_node);
}

/**
 * Update state machine
 */
void
S2_fsm_post_event(struct S2* p_context, event_t e, event_data_t* d)
{
  CTX_DEF

  uint8_t nr_flag;

  switch (ctxt->fsm)
  {
  case IS_MOS_WAIT_REPLY:
  case IDLE:
    //S2_set_peer(ctxt, d->con, d->buffer, d->len);
    if (e == SEND_MSG && S2_span_ok(ctxt, d->con))
    {
      S2_set_peer(ctxt, d->con, d->d.buf.buffer, d->d.buf.len);
      ctxt->retry = 2;

      goto send_msg_state_enter;
    }
    else if (e == SEND_MSG)
    {
      ctxt->fsm = WAIT_NONCE_RAPORT;
      ctxt->retry = 2;

      S2_set_peer(ctxt, d->con, d->d.buf.buffer, d->d.buf.len);
      S2_send_nonce_get(ctxt);
      S2_set_timeout(ctxt, SEND_DATA_TIMEOUT);
    }
    else if (e == GOT_NONCE_GET && (d->d.buf.len >= 3) && S2_verify_seq(ctxt, d->con, d->d.buf.buffer[2]))
    {
      S2_send_nonce_report(ctxt, d->con, SECURITY_2_NONCE_REPORT_PROPERTIES1_SOS_BIT_MASK);
    }
    else if (e == GOT_NONCE_REPORT )
    {
      S2_set_peer(ctxt, d->con, d->d.buf.buffer, d->d.buf.len);
      S2_register_nonce(ctxt, d->d.buf.buffer, d->d.buf.len);
      emit_S2_synchronization_event(SOS_EVENT_REASON_UNANSWERED, d);
    }
    else if (e == SEND_MULTICAST)
    {
      S2_set_peer(ctxt, d->con, d->d.buf.buffer, d->d.buf.len);
      //For Multicast: 8-bit group_id is stored in d->con->r_node
      ctxt->mpan = find_mpan_by_group_id(ctxt, 0, d->con->r_node, 1);
      ctxt->fsm = SENDING_MSG;
      S2_encrypt_and_send_multi(ctxt);
    }
    else if (e == SEND_DONE)
    {
      /* pass message to the inclusion FSM */
      s2_inclusion_send_done(ctxt, d->d.tx.status == S2_TRANSMIT_COMPLETE_OK);
    }
    else if (e == GOT_ENC_MSG_MOS)
    {
      S2_set_timeout(ctxt, 10);
      S2_set_peer(ctxt,d->con,0,0);
      ctxt->fsm = IS_MOS_WAIT_REPLY;
    }
    else if (e == TIMEOUT && ctxt->fsm == IS_MOS_WAIT_REPLY)
    {
      ctxt->mpan = 0;
      ctxt->fsm = IDLE;
      S2_send_nonce_report(ctxt, &ctxt->peer, SECURITY_2_NONCE_REPORT_PROPERTIES1_MOS_BIT_MASK);
    }
    break;
  case WAIT_NONCE_RAPORT:
    if ((e == SEND_DONE) && (d->d.tx.status == S2_TRANSMIT_COMPLETE_NO_ACK))
    {
      p_context->fsm = IDLE;
      S2_stop_timeout(p_context);
      S2_post_send_done_event(p_context, S2_TRANSMIT_COMPLETE_FAIL);
    }
    else if ((e == SEND_DONE) && (d->d.tx.status == S2_TRANSMIT_COMPLETE_OK))
    {
      S2_set_timeout(ctxt, d->d.tx.time); //Just shorten timer but stay in this state
    }
    else if ((e == SEND_DONE) || (e == TIMEOUT))
    {
      ctxt->fsm = IDLE;
      if (e == TIMEOUT) {
          S2_post_send_done_event(ctxt, S2_TRANSMIT_COMPLETE_FAIL);
      } else {
          S2_post_send_done_event(ctxt, d->d.tx.status);
      }
    }
    else if ((e == GOT_NONCE_REPORT) && S2_is_peernode(ctxt, d->con))
    {
      if (S2_register_nonce(ctxt, d->d.buf.buffer, d->d.buf.len) & SECURITY_2_NONCE_REPORT_PROPERTIES1_SOS_BIT_MASK)
      {
        goto send_msg_state_enter;
      }
    }
    else if ((e == GOT_NONCE_REPORT) && !S2_is_peernode(ctxt, d->con))
    {
      emit_S2_synchronization_event(SOS_EVENT_REASON_UNANSWERED, d);
    }
    break;
  case SENDING_MSG:
    if (e == SEND_DONE)
    {
      ctxt->fsm = IDLE;
      S2_post_send_done_event(ctxt, d->d.tx.status);
    }
    else if (e == GOT_NONCE_REPORT && !S2_is_peernode(ctxt, d->con))
    {
      emit_S2_synchronization_event(SOS_EVENT_REASON_UNANSWERED, d);
    }
    // else: Unexpected event while in state SENDING_MSG
    break;
  case VERIFYING_DELIVERY:
    if (e == SEND_DONE)
    { /* shorten timer on ACK */
      if (d->d.tx.status == S2_TRANSMIT_COMPLETE_OK)
      {
        S2_set_timeout(ctxt, d->d.tx.time); //Just shorten timer but stay in this state
      }
      else
      { /* bail out */
        ctxt->fsm = IDLE;
        S2_post_send_done_event(ctxt, d->d.tx.status);
      }
    }
    else if (e == GOT_ENC_MSG && S2_is_peernode(ctxt, d->con))
    {
      if(S2_is_node_mos(ctxt,d->con->r_node) && (ctxt->retry > 0)) {
        ctxt->length = 0;
        ctxt->peer.tx_options &= ~S2_TXOPTION_VERIFY_DELIVERY;
        goto send_msg_state_enter;
      } else {
        ctxt->fsm = IDLE;
        /* Stop S2 timer when we have verified delivery so ZW protocol can go back to sleep */
        S2_stop_timeout(ctxt);
        S2_post_send_done_event(ctxt, S2_TRANSMIT_COMPLETE_VERIFIED);
      }
    }
    /*
     * As we know there will a nonce report under way, it is better to
     * wait for the NONCE_REPORT for the sake of keeping synchronization.
     *
    else if (e == GOT_BAD_ENC_MSG && S2_is_peernode(ctxt, d->con))
    {
      ctxt->fsm = IDLE; //No more retries
      S2_post_send_done_event(ctxt, S2_TRANSMIT_COMPLETE_FAIL);
    }*/
    else if (e == TIMEOUT)
    {
      ctxt->fsm = IDLE; //The frame seems to be handled but we don't know for sure
      S2_post_send_done_event(ctxt, S2_TRANSMIT_COMPLETE_OK);
    }
    else if (e == GOT_NONCE_REPORT && S2_is_peernode(ctxt, d->con))
    {
      nr_flag = S2_register_nonce(ctxt, d->d.buf.buffer, d->d.buf.len);
      if (nr_flag == 0)
      {
        return;
      }
      else if (nr_flag == SECURITY_2_NONCE_REPORT_PROPERTIES1_MOS_BIT_MASK)
      {
        /* if we only get a MOS flag back, set the payload length to 0,
         * as we don't need to retransmit the payload, we should only
         * send the MPAN*/
        ctxt->length = 0;
      }

      if (ctxt->retry == 0)
      {
        ctxt->fsm = IDLE; //No more retries
        S2_post_send_done_event(ctxt, S2_TRANSMIT_COMPLETE_FAIL);
      }
      else
      {
        goto send_msg_state_enter;
        // send again
      }
    }
    else if (e == GOT_NONCE_REPORT && !S2_is_peernode(ctxt, d->con))
    {
      emit_S2_synchronization_event(SOS_EVENT_REASON_UNANSWERED, d);
    }
    // else: Unexpected event while in state VERIFYING_DELIVERY
    break;
  default:
    assert(0);
  }

  return;
send_msg_state_enter:
  ctxt->fsm = SENDING_MSG;

  if (ctxt->peer.tx_options & S2_TXOPTION_VERIFY_DELIVERY)
  {
    ctxt->fsm = VERIFYING_DELIVERY;
    S2_set_timeout(ctxt, SEND_DATA_TIMEOUT);
  }

  S2_encrypt_and_send(ctxt);
  ctxt->retry--;


  return;
}

uint8_t
S2_send_data_multicast(struct S2* p_context, const s2_connection_t* con, const uint8_t* buf, uint16_t len)
{
  CTX_DEF
  // No key conversion here, because we get a Group ID and we cannot know
  // which keyset to use. Send data multicast will be for Z-Wave only.
  return S2_send_data_all_cast(ctxt, con, buf, len, SEND_MULTICAST);
}

uint8_t
S2_send_data_singlecast_follow_up_with_keyset(struct S2* ctxt,
                                              s2_connection_t* connection,
                                              zwave_s2_keyset_t keyset,
                                              const uint8_t* payload,
                                              uint16_t payload_length)
{
  if (keyset == ZWAVE_KEYSET) {
    return S2_send_data_all_cast(ctxt,
                                 connection,
                                 payload,
                                 payload_length,
                                 SEND_FOLLOW_UP);

  } else if (keyset == ZWAVE_LONG_RANGE_KEYSET) {
    convert_normal_to_lr_keyclass(connection);
    return S2_send_data_all_cast(ctxt,
                                 connection,
                                 payload,
                                 payload_length,
                                 SEND_FOLLOW_UP);
  }

  // Unknown keyset, we ignore.
  return 0;
}

uint8_t S2_send_data_multicast_with_keyset(struct S2* ctxt,
                                           s2_connection_t* connection,
                                           zwave_s2_keyset_t keyset,
                                           const uint8_t* payload,
                                           uint16_t payload_length)
{
  if (keyset == ZWAVE_KEYSET) {
    return S2_send_data_all_cast(ctxt,
                                 connection,
                                 payload,
                                 payload_length,
                                 SEND_MULTICAST);

  } else if (keyset == ZWAVE_LONG_RANGE_KEYSET) {
    convert_normal_to_lr_keyclass(connection);
    return S2_send_data_all_cast(ctxt,
                                 connection,
                                 payload,
                                 payload_length,
                                 SEND_MULTICAST);
  }

  // Unknown keyset, we ignore.
  return 0;
}

uint8_t S2_send_data_singlecast_with_keyset(struct S2* ctxt,
                                            s2_connection_t* connection,
                                            zwave_s2_keyset_t keyset,
                                            const uint8_t* payload,
                                            uint16_t payload_length)
{
  if (keyset == ZWAVE_KEYSET) {
    return S2_send_data_all_cast(ctxt,
                                 connection,
                                 payload,
                                 payload_length,
                                 SEND_MSG);

  } else if (keyset == ZWAVE_LONG_RANGE_KEYSET) {
    convert_normal_to_lr_keyclass(connection);
    return S2_send_data_all_cast(ctxt,
                                 connection,
                                 payload,
                                 payload_length,
                                 SEND_MSG);
  }

  // Unknown keyset, we ignore.
  return 0;
}


/*
 * Converts the class_id of a s2_connection_t object for a
 * ZWAVE_KEYSET to ZWAVE_LONG_RANGE_KEYSET
 *
 * This function must not be applied if the node operates with another keyset.
 *
 * \param [out] con             Pointer to the s2_connection_t to modify
 */
static void convert_normal_to_lr_keyclass(s2_connection_t *con)
{
  if(con->class_id == ACCESS_KEY_SLOT) {
    con->class_id = LR_ACCESS_KEY_SLOT;
  } else if(con->class_id == AUTHENTICATED_KEY_SLOT) {
    con->class_id = LR_AUTHENTICATED_KEY_SLOT;
  }
  // Ideally if we ask an impossible conversion
  // (e.g. UNAUTHENTICATED to Long range),
  // we should fall back on an invalid class_id.
}


/*
 * Converts the class_id of a s2_connection_t object for a
 * ZWAVE_LONG_RANGE_KEYSET keyset to a ZWAVE_KEYSET.
 *
 * \param [out] con             Pointer to the s2_connection_t to modify
 */
static void convert_lr_to_normal_keyclass(s2_connection_t *con)
{
  if(con->class_id == LR_ACCESS_KEY_SLOT) {
    con->class_id = ACCESS_KEY_SLOT;
  } else if(con->class_id == LR_AUTHENTICATED_KEY_SLOT) {
    con->class_id = AUTHENTICATED_KEY_SLOT;
  }
  // Ideally if we ask an impossible conversion
  // (e.g. UNAUTHENTICATED to Long range),
  // we should fall back on an invalid class_id.
}

static uint8_t
S2_send_data_all_cast(struct S2* p_context, const s2_connection_t* con, const uint8_t* buf, uint16_t len, event_t ev)
{
  CTX_DEF
  event_data_t e;

  if (len == 0 || len > WORKBUF_SIZE || buf == 0 || S2_is_send_data_busy(ctxt))
  {
    return 0;
  }

  e.d.buf.buffer = buf;
  e.d.buf.len = len;
  e.con = con;

  if (ev == SEND_MULTICAST) {
    S2_fsm_post_event(ctxt, SEND_MULTICAST, &e);

  } else if (ev == SEND_MSG) {
    S2_fsm_post_event(ctxt, SEND_MSG, &e);

  } else if (ev == SEND_FOLLOW_UP) {
    // Here the user has selected a MGRP Group ID for sending.
    // Make a context MPAN switch, if we can find it
    struct MPAN *new_mpan = find_mpan_by_group_id(ctxt, 0, con->mgrp_group_id, 0);
    if (new_mpan) {
      ctxt->mpan = new_mpan;
    }
    // Convert to a regular message event.
    S2_fsm_post_event(ctxt, SEND_MSG, &e);
  }

  return 1;
}

uint8_t
S2_is_send_data_multicast_busy(struct S2* p_context)
{
  CTX_DEF
  return ctxt->fsm != IDLE;
}

static void S2_send_nls_state_set(struct S2* p_context, s2_connection_t* con, bool nls_active)
{
  CTX_DEF
  ctxt->workbuf[SECURITY_2_COMMAND_CLASS_POS]              = COMMAND_CLASS_SECURITY_2_V2;
  ctxt->workbuf[SECURITY_2_COMMAND_POS]                    = NLS_STATE_SET_V2;
  ctxt->workbuf[SECURITY_2_V2_NLS_STATE_SET_STATE_POS]     = nls_active;

  S2_send_data(ctxt, con, ctxt->workbuf, SECURITY_2_V2_NLS_STATE_SET_LENGTH);
}

static void S2_send_nls_state_get(struct S2* p_context, s2_connection_t* con)
{
  CTX_DEF

  uint8_t plain_text[SECURITY_2_V2_NLS_STATE_GET_LENGTH] = { 0 };
  plain_text[SECURITY_2_COMMAND_CLASS_POS]  = COMMAND_CLASS_SECURITY_2_V2;
  plain_text[SECURITY_2_COMMAND_POS]        = NLS_STATE_GET_V2;

  S2_send_data(p_context, con, p_context->workbuf, SECURITY_2_V2_NLS_STATE_GET_LENGTH);
}

static void S2_send_nls_state_report(struct S2* p_context, s2_connection_t* con)
{
  CTX_DEF

  uint8_t plain_text[SECURITY_2_V2_NLS_STATE_REPORT_LENGTH] = { 0 };
  uint8_t nls_bitfield;
  nls_bitfield = ctxt->nls_state ? SECURITY_2_V2_NLS_STATE_REPORT_STATE_FIELD | SECURITY_2_V2_NLS_STATE_REPORT_CAPABILITY_FIELD : 0; // A node sending this frame will always support NLS
  plain_text[SECURITY_2_COMMAND_CLASS_POS]  = COMMAND_CLASS_SECURITY_2_V2;
  plain_text[SECURITY_2_COMMAND_POS]        = NLS_STATE_REPORT_V2;
  plain_text[SECURITY_2_V2_NLS_STATE_REPORT_BITFIELD_POS] = nls_bitfield;

  S2_send_data(ctxt, con, ctxt->workbuf, SECURITY_2_V2_NLS_STATE_GET_LENGTH);
}