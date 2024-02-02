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
#include "misc.h"
#include "../inclusion/s2_inclusion_internal.h"
#include<string.h>
#include "ccm.h"
#include "aes_cmac.h"
#include "nextnonce.h"
#include "kderiv.h"
#include <bigint.h>
#include "aes.h"

#include <platform.h>
#include "ZW_classcmd.h"
#include "s2_keystore.h"
#ifdef ZWAVE_PSA_SECURE_VAULT
#include "s2_psa.h"
#endif

//#define DEBUG       // To enable debug_print_hex()
//#define DEBUGPRINT

#ifdef DEBUGPRINT
#include "../../../Components/DebugPrint/DebugPrint.h"
#else
#define DPRINT(PSTRING)
#define DPRINTF(PFORMAT, ...)
#endif

#ifdef SINGLE_CONTEXT
struct S2 the_context;
#endif

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


#ifdef DEBUG

void
debug_print_hex(uint8_t* vector, uint32_t length)
{
  for(int i=0; i<length; i++)
  {
    DPRINTF("0x%02x ", *(vector + i));
  }
  DPRINT("\n");
}
#else
#define debug_print_hex(a,b)
#endif

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
  uint8_t rnd[RANDLEN];
  int i;

  for (i = 0; i < MPAN_TABLE_SIZE; i++)
  {
    if ((p_context->mpan_table[i].state != MPAN_NOT_USED) && (p_context->mpan_table[i].group_id == group_id)
        && (p_context->mpan_table[i].owner_id == owner_id) && ((1 << p_context->mpan_table[i].class_id) &  p_context->loaded_keys))
    {
      return &p_context->mpan_table[i];
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
    if (p_context->mpan_table[i].state == MPAN_NOT_USED)
    {
      break;
    }
  }

  /*Just select a random entry Note this will overwrite existing entries
   * TODO we should really select the oldest entry
   * */
  if (i == MPAN_TABLE_SIZE)
  {
    i = rnd[0] % MPAN_TABLE_SIZE;
    DPRINT("dropping random span entry\n");
  }

  p_context->mpan_table[i].state = owner_id ? MPAN_MOS : MPAN_SET;
  p_context->mpan_table[i].group_id = group_id;
  p_context->mpan_table[i].owner_id = owner_id;
  p_context->mpan_table[i].class_id = p_context->peer.class_id; //Here we assume that peer is set...

  AES_CTR_DRBG_Generate(&s2_ctr_drbg, p_context->mpan_table[i].inner_state);
  ;

  return &p_context->mpan_table[i];
}

static struct SPAN  *
find_span_by_node(struct S2* p_context, const s2_connection_t* con)
{
  uint8_t rnd[RANDLEN];
  int i;
  /* Locate existing entry */
  for (i = 0; i < SPAN_TABLE_SIZE; i++)
  {
    if (p_context->span_table[i].state != SPAN_NOT_USED && (p_context->span_table[i].lnode == con->l_node)
        && (p_context->span_table[i].rnode == con->r_node))
    {
      return &p_context->span_table[i];
    }
  }

  AES_CTR_DRBG_Generate(&s2_ctr_drbg, rnd);

  /*Allocate new entry if possible */
  for (i = 0; i < SPAN_TABLE_SIZE; i++)
  {
    if (p_context->span_table[i].state == SPAN_NOT_USED)
    {
      break;
    }
  }

  /*Just select a random entry Note this will overwrite existing entries*/
  if (i == SPAN_TABLE_SIZE)
  {
    i = rnd[0] % SPAN_TABLE_SIZE;
    DPRINT("dropping random span entry\n");
  }

  p_context->span_table[i].state = SPAN_NO_SEQ;
  p_context->span_table[i].lnode = con->l_node;
  p_context->span_table[i].rnode = con->r_node;
  p_context->span_table[i].tx_seq = rnd[1];
  return &p_context->span_table[i];
}

/**
 * Check if the span is synchronized.
 */
static int
S2_span_ok(struct S2* p_context, const s2_connection_t* con)
{
  struct SPAN  *span = find_span_by_node(p_context, con);

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
 * Send nonce get to p_context->peer
 */
static void
S2_send_nonce_get(struct S2* p_context)
{
  static uint8_t nonce_get[] =
    { COMMAND_CLASS_SECURITY_2, SECURITY_2_NONCE_GET, 0 };

  struct SPAN  *span = find_span_by_node(p_context, &p_context->peer);

  ASSERT(span);

  nonce_get[2] = span->tx_seq;
  S2_send_raw(p_context, nonce_get, 3);
}

/**
 * Verify the sequence of the received frame.
 */
static int
S2_verify_seq(struct S2* p_context, const s2_connection_t* peer, uint8_t seq)
{
  struct SPAN  *span = find_span_by_node(p_context, peer);
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
    DPRINTF("Duplicate frame dropped with seq %d\n", seq);

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
  uint8_t i;
  for (i = 0; i < MOS_LIST_LENGTH; i++)
  {
    if (p_context->mos_list[i].node_id == nodeid)
    {
      return 1;
    }
  }
  return 0;

}
/* Add MPAN extensions for the current p_context->peer by checks our mpan table
 * for nodes who is reported MOS.
 *
 *
 */
static uint16_t
S2_add_mpan_extensions(struct S2* p_context, uint8_t* ext_data)
{
  uint8_t i, k;
  uint8_t *p;
  struct MPAN* mpan;

  p = ext_data;
  k = 0;
  for (i = 0; i < MOS_LIST_LENGTH; i++)
  {
    if (p_context->mos_list[i].node_id == p_context->peer.r_node)
    {
      DPRINTF("Adding MPAN for node %i:%i\n",p_context->mos_list[i].node_id, p_context->mos_list[i].group_id);
      mpan = find_mpan_by_group_id(p_context, 0, p_context->mos_list[i].group_id, 0);
      if (!mpan)
      {
        DPRINT("could not find MPAN");
        continue;
      }
      k++;
      *p++ = 19;
      *p++ = S2_MSG_EXTHDR_TYPE_MPAN | S2_MSG_EXTHDR_MORE_FLAG | S2_MSG_EXTHDR_CRITICAL_FLAG;
      *p++ = p_context->mos_list[i].group_id;
      memcpy(p, mpan->inner_state, 16);

      //Remove the node from the mos list
      p_context->mos_list[i].node_id = 0;

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
 * Encrypt a single cast message stored in p_context and send it
 */
void
S2_encrypt_and_send(struct S2* p_context)
{
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

  struct SPAN  *span = find_span_by_node(p_context, &p_context->peer);

  msg = p_context->workbuf;
  msg[0] = COMMAND_CLASS_SECURITY_2;
  msg[1] = SECURITY_2_MESSAGE_ENCAPSULATION;
  msg[2] = span->tx_seq;

  msg[3] = 0;

  hdr_len = 4;
  n_ext = 0;
  DPRINT("S2_encrypt_and_send\r\n");
  /*If span is not negotiated, include senders nonce (SN) */
  ext_data = &msg[4];

  if (span->state == SPAN_SOS_REMOTE_NONCE)
  {
    DPRINTF("SPAN_SOS_REMOTE_NONCE. class_id: %u\n", p_context->peer.class_id);
    AES_CTR_DRBG_Generate(&s2_ctr_drbg, ei_sender);
    memcpy(ei_receiver, span->d.r_nonce, sizeof(ei_receiver));

    next_nonce_instantiate(&span->d.rng, ei_sender, ei_receiver, p_context->sg[p_context->peer.class_id].nonce_key);

    span->class_id = p_context->peer.class_id;
    span->state = SPAN_NEGOTIATED; //TODO is it better to set this on send_data complete?

    *ext_data++ = 2 + sizeof(span->d.r_nonce); //Extension length
    *ext_data++ = S2_MSG_EXTHDR_CRITICAL_FLAG | S2_MSG_EXTHDR_TYPE_SN;
    memcpy(ext_data, ei_sender, 16);
    hdr_len += 2 + 16;
    ext_data += 16;
    n_ext++;
  }

  if ((p_context->peer.tx_options & (S2_TXOPTION_SINGLECAST_FOLLOWUP | S2_TXOPTION_FIRST_SINGLECAST_FOLLOWUP)) && p_context->mpan)
  {

    /* If the destination is mos, then we will add the MPAN extension instead */
    if (!S2_is_node_mos(p_context, p_context->peer.r_node))
    {

      /* Add the MGRP header extension */
      *ext_data++ = 3;
      *ext_data++ = S2_MSG_EXTHDR_CRITICAL_FLAG | S2_MSG_EXTHDR_TYPE_MGRP;
      *ext_data++ = p_context->mpan->group_id;
      hdr_len += 3;
      n_ext++;
    }

    if ((p_context->peer.tx_options & S2_TXOPTION_FIRST_SINGLECAST_FOLLOWUP) && p_context->retry == 2)
    {
      next_mpan_state(p_context->mpan);
    }
  }

  /*Add MOS extension */
  if (p_context->mpan && p_context->mpan->state == MPAN_MOS)
  {
    p_context->mpan->state = MPAN_NOT_USED;
    p_context->mpan = 0;
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
  DPRINT("before mpan\r\n");
  /* Add the secure extensions */
  shdr_len = S2_add_mpan_extensions(p_context, ciphertext);
  if (shdr_len)
  {
    msg[3] |=
    SECURITY_2_MESSAGE_ENCAPSULATION_PROPERTIES1_ENCRYPTED_EXTENSION_BIT_MASK;
  }

  memcpy(ciphertext + shdr_len, p_context->buf, p_context->length);
  DPRINT("after memcpy\r\n");
  aad_len = S2_make_aad(p_context, p_context->peer.l_node, p_context->peer.r_node, msg, hdr_len,
      p_context->length + shdr_len + hdr_len + AUTH_TAG_LEN, aad, sizeof(aad));
  DPRINT("before next_nonce_generate\r\n");
  /*TODO we should consider to roll the nonce when we have recevied in ACK*/
  next_nonce_generate(&span->d.rng, nonce); //Create the new nonce
  ZW_DEBUG_SEND_STR("after next_nonce_generate\r\n");
#ifdef DEBUG
  DPRINTF("%p Encryption class %i\n",p_context,p_context->peer.class_id);
  DPRINT("Nonce \n");
  debug_print_hex(nonce,16);
  DPRINT("key \n");
  debug_print_hex(p_context->sg[p_context->peer.class_id].enc_key,16);
  DPRINT("AAD \n");
  debug_print_hex(aad,aad_len);
  DPRINTF("State: %d\n",p_context->inclusion_state);
  DPRINTF("==>ReceivedKey: %X\n", mp_context->buf[SECURITY_2_NET_KEY_REP_GRANT_KEY_POS]);
  DPRINTF("==>KeyExchange: %X\n", mp_context->key_exchange);
  DPRINTF("==>KeyGranted : %X\n", mp_context->key_granted);
  DPRINTF("==>LoadedKeys: %X\n", mp_context->loaded_keys);
#endif

  DPRINT("ciphertext \n");
  debug_print_hex(ciphertext, p_context->length + shdr_len);

  DPRINT("CCM enc auth\r\n");
#if defined(ZWAVE_PSA_SECURE_VAULT) && defined(ZWAVE_PSA_AES)
  size_t out_len;
  uint32_t ccm_key_id = ZWAVE_CCM_TEMP_ENC_KEY_ID;
  if (p_context->is_keys_restored == false)
  {
       /* Import key into secure vault */
     zw_wrap_aes_key_secure_vault(&ccm_key_id, p_context->sg[p_context->peer.class_id].enc_key, ZW_PSA_ALG_CCM);
  }
  else
  {
    /* Use secure vault for encryption using PSA APIs */
    ccm_key_id = convert_keyclass_to_derived_key_id(convert_key_slot_to_keyid(p_context->peer.class_id), ZWAVE_KEY_TYPE_SINGLE_CAST);
  }
  zw_psa_aead_encrypt_ccm(ccm_key_id, nonce, ZWAVE_PSA_AES_NONCE_LENGTH, aad, aad_len, ciphertext,
                        p_context->length + shdr_len, ciphertext, p_context->length+shdr_len+ZWAVE_PSA_AES_MAC_LENGTH, &out_len);
  msg_len = out_len;
  ASSERT(msg_len == (p_context->length + shdr_len + ZWAVE_PSA_AES_MAC_LENGTH));
  /* Remove key from vault */
  if (p_context->is_keys_restored == false) {
    zw_psa_destroy_key(ccm_key_id);
  }
#else
  msg_len = CCM_encrypt_and_auth(p_context->sg[p_context->peer.class_id].enc_key, nonce, aad, aad_len, ciphertext,
        p_context->length + shdr_len);
#endif

  DPRINT("ciphertext \n");
  debug_print_hex(ciphertext, msg_len);

  ASSERT(msg_len > 0);
  S2_send_raw(p_context, msg, msg_len + hdr_len);
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
  uint8_t aad[64];
  uint16_t aad_len;
  uint8_t nonce[16];
  uint8_t* ciphertext;
  uint16_t hdr_len;
  uint8_t* msg;
  uint16_t msg_len;
  event_data_t e;
  msg = p_context->workbuf;
  msg[0] = COMMAND_CLASS_SECURITY_2;
  msg[1] = SECURITY_2_MESSAGE_ENCAPSULATION;
  msg[2] = 0xFF; //TODO
  msg[3] = SECURITY_2_MESSAGE_ENCAPSULATION_PROPERTIES1_EXTENSION_BIT_MASK;

  /* Add the encrypted header extension */
  msg[4] = 3;
  msg[5] = S2_MSG_EXTHDR_CRITICAL_FLAG | S2_MSG_EXTHDR_TYPE_MGRP;
  msg[6] = p_context->mpan->group_id;

  hdr_len = 4 + 3;

  ciphertext = &msg[hdr_len];

  memcpy(ciphertext, p_context->buf, p_context->length);

  aad_len = S2_make_aad(p_context, p_context->peer.l_node, p_context->peer.r_node, msg, hdr_len, p_context->length + hdr_len + AUTH_TAG_LEN,
      aad, sizeof(aad));

#if defined(ZWAVE_PSA_SECURE_VAULT) && defined(ZWAVE_PSA_AES)
    uint32_t key_id = ZWAVE_ECB_TEMP_ENC_KEY_ID;
    /* Import key into secure vault */
    zw_wrap_aes_key_secure_vault(&key_id, p_context->sg[p_context->mpan->class_id].mpan_key, ZW_PSA_ALG_ECB_NO_PAD);
    zw_psa_aes_ecb_encrypt(key_id, p_context->mpan->inner_state, nonce);
    /* Remove key from vault */
    zw_psa_destroy_key(key_id);
#else
  AES128_ECB_encrypt(p_context->mpan->inner_state, p_context->sg[p_context->mpan->class_id].mpan_key, nonce);
#endif

  next_mpan_state(p_context->mpan);

#ifdef DEBUGPRINT
  DPRINTF("%p Encryption\n",p_context);
  DPRINT("Nonce \n");
  debug_print_hex(nonce,16);
  DPRINT("key \n");
  debug_print_hex(p_context->sg[p_context->mpan->class_id].enc_key,16);
  DPRINT("AAD \n");
  debug_print_hex(aad,aad_len);
#endif

#if defined(ZWAVE_PSA_SECURE_VAULT) && defined(ZWAVE_PSA_AES)
  //////////////////////////////////////////////
  size_t out_len;
  key_id = ZWAVE_CCM_TEMP_ENC_KEY_ID;
  if (p_context->is_keys_restored == false)
  {
       /* Import key into secure vault */
    zw_wrap_aes_key_secure_vault(&key_id, p_context->sg[p_context->mpan->class_id].enc_key, ZW_PSA_ALG_CCM);
    DPRINTF("==> wrapping using temp <==\n");
  }
  else
  {
    /* Use secure vault for encryption using PSA APIs */
    key_id = convert_keyclass_to_derived_key_id(convert_key_slot_to_keyid(p_context->mpan->class_id), ZWAVE_KEY_TYPE_SINGLE_CAST);
    DPRINTF("==> Using Persistent:KeyId. Conversion : %lX <==\n", key_id);
  }
  zw_psa_aead_encrypt_ccm(key_id, nonce, ZWAVE_PSA_AES_NONCE_LENGTH, aad, aad_len, ciphertext,
                                     p_context->length, ciphertext, p_context->length+ZWAVE_PSA_AES_MAC_LENGTH, &out_len);
  msg_len = out_len;
  ASSERT(msg_len == (p_context->length + ZWAVE_PSA_AES_MAC_LENGTH));
  /* Remove key from vault */
  if (p_context->is_keys_restored == false) {
    zw_psa_destroy_key(key_id);
  }
#else
  msg_len = CCM_encrypt_and_auth(p_context->sg[p_context->mpan->class_id].enc_key, nonce, aad, aad_len, ciphertext, p_context->length);
#endif

  ASSERT(msg_len > 0);

  if (S2_send_frame_multi(p_context, &p_context->peer, msg, msg_len + hdr_len))
  {
    //TX seq?
  }
  else
  {
    e.d.tx.status = S2_TRANSMIT_COMPLETE_FAIL;
    S2_fsm_post_event(p_context, SEND_DONE, &e);
  }
}

void
S2_send_frame_done_notify(struct S2* p_context, s2_tx_status_t status, uint16_t tx_time)
{
  event_data_t e;
  e.d.tx.status = status;
  e.d.tx.time = tx_time;
  S2_fsm_post_event(p_context, SEND_DONE, &e);
}

uint8_t S2_is_busy(struct S2* p_context)
{
  if(p_context->inclusion_state != S2_INC_IDLE)
  {
    return 1;
  }

  if( (p_context->fsm != IDLE) && (p_context->fsm != IS_MOS_WAIT_REPLY) )
  {
    return 1;
  }

  return 0;
}

void S2_free_mpan(struct S2* p_context, node_t owner_id, uint8_t group_id) {
  // Search for a MPAN with the Group ID / owner ID, and if found, set it back to NOT USED.
  for (uint8_t i = 0; i < MPAN_TABLE_SIZE; i++) {
    if ((p_context->mpan_table[i].group_id == group_id)
        && (p_context->mpan_table[i].owner_id == owner_id)) {
      p_context->mpan_table[i].state = MPAN_NOT_USED;
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
  event_data_t e;

  if (S2_send_frame(p_context, &p_context->peer, buf, len))
  {
    struct SPAN  *span = find_span_by_node(p_context, &p_context->peer);
    span->tx_seq++;
  }
  else
  {
    e.d.tx.status = S2_TRANSMIT_COMPLETE_FAIL;
    S2_fsm_post_event(p_context, SEND_DONE, &e);
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
  uint8_t i;
  for (i = 0; i < MPAN_TABLE_SIZE; i++)
  {
    if ((p_context->mpan_table[i].owner_id == node_id) && (p_context->mpan_table[i].state == MPAN_MOS))
    {
      if (clear)
      {
        p_context->mpan_table[i].state = MPAN_NOT_USED;
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
  struct SPAN  *span;
  uint8_t rnd[RANDLEN];

  static uint8_t nonce_report[2 + 2 + sizeof(span->d.r_nonce)];

  span = find_span_by_node(p_context, conn);
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
  S2_send_frame_no_cb(p_context, conn, nonce_report, nonce_report[3] & SECURITY_2_NONCE_REPORT_PROPERTIES1_SOS_BIT_MASK ? 20 : 4);
}

static void
S2_set_node_mos(struct S2* p_context, node_t node)
{
  uint8_t i;
  if ((p_context->fsm == VERIFYING_DELIVERY || p_context->fsm == SENDING_MSG)
      && (p_context->peer.tx_options & (S2_TXOPTION_SINGLECAST_FOLLOWUP | S2_TXOPTION_FIRST_SINGLECAST_FOLLOWUP))
      && p_context->mpan)
  {
    for (i = 0; i < MOS_LIST_LENGTH; i++)
    {
      if (p_context->mos_list[i].node_id == 0)
      {
        p_context->mos_list[i].group_id = p_context->mpan->group_id;
        p_context->mos_list[i].node_id = node;
        break;
      }
    }
  }
}

static uint8_t
S2_register_nonce(struct S2* p_context, const uint8_t* buf, uint16_t len)
{
  struct SPAN  *span;

  if(!S2_verify_seq(p_context, &p_context->peer, buf[2])) {
    return 0;
  }

  span = find_span_by_node(p_context, &p_context->peer);

  if (len >= (4 + 16) && (buf[3] & SECURITY_2_NONCE_REPORT_PROPERTIES1_SOS_BIT_MASK))
  {
    memcpy(span->d.r_nonce, &buf[4], sizeof(span->d.r_nonce));
    span->state = SPAN_SOS_REMOTE_NONCE;
  }

  /*Register MOS, but only if we are expecting it */
  if ((buf[3] & SECURITY_2_NONCE_REPORT_PROPERTIES1_MOS_BIT_MASK) && (len >= 3))
  {
    S2_set_node_mos(p_context,p_context->peer.r_node);
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
  aad[i++] = (p_context->my_home_id >> 24) & 0xFF;
  aad[i++] = (p_context->my_home_id >> 16) & 0xFF;
  aad[i++] = (p_context->my_home_id >> 8)  & 0xFF;
  aad[i++] = (p_context->my_home_id >> 0)  & 0xFF;
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
  uint8_t r_nonce[16];
  uint8_t s_nonce[16];
  uint8_t i;

  hdr_len = 4;
  decrypt_len = 0;
  *plain_text = 0;
  *plain_text_len = 0;

  flags = msg[3];
  if (msg_len < (hdr_len + AUTH_TAG_LEN))
  {
    DPRINTF("====> parse_fail\n");
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
    if (!S2_verify_seq(p_context, conn, msg[2]))
    {
      DPRINTF("====> sequence_fail\n");
      return SEQUENCE_FAIL;
    }

    span = find_span_by_node(p_context, conn);
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

          next_nonce_instantiate(&span->d.rng, s_nonce, r_nonce, p_context->sg[span->class_id].nonce_key);
        }
        break;
      case S2_MSG_EXTHDR_TYPE_MGRP:
        if (ext_len != 3)
        {
          goto parse_fail;
        }
        /*Only create new MPAN if this was a single cast followup*/
        mpan = find_mpan_by_group_id(p_context, conn->r_node, ext_data[2], (conn->rx_options & S2_RXOPTION_MULTICAST) == 0);
        break;
      case S2_MSG_EXTHDR_TYPE_MOS:
        if (ext_len != 2)
        {
          goto parse_fail;
        }
        S2_set_node_mos(p_context,conn->r_node);
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
      DPRINTF("Unexpected span state %i l:%i-r:%i\n", span->state,conn->l_node, conn->r_node);
      goto auth_fail;
    }
  }

  ciphertext = &msg[hdr_len];
  ciphertext_len = msg_len - hdr_len;

  aad = &aad_buf[0];

  aad_len = S2_make_aad(p_context, conn->r_node, conn->l_node, msg, hdr_len, msg_len, aad, sizeof(aad_buf));

  if (span)
  {
    /*Single cast decryption */

    /*In this state we don't know which class_id was used to encrypt the frame, so
     * we will try de-crypting with all our classes */

    /*Check the fsm before using the workbuf */
    if (p_context->fsm == IDLE && span->state == SPAN_INSTANTIATE)
    {
      memcpy(p_context->workbuf, ciphertext, ciphertext_len);
    }

    for (i = 0; i < N_SEC_CLASS; i++)
    {
      /*Only decrypt with a key which is loaded */
      if (p_context->loaded_keys & (1 << span->class_id))
      {
        next_nonce_generate(&span->d.rng, nonce);

#ifdef DEBUGPRINT
        DPRINTF("%p Decryption class %i\n",p_context,span->class_id);
        DPRINT("Nonce \n");
        debug_print_hex(nonce,16);
        DPRINT("key \n");
        debug_print_hex(p_context->sg[span->class_id].enc_key,16);
        DPRINT("AAD \n");
        debug_print_hex(aad,aad_len);
        DPRINTF("State: %d\n",p_context->inclusion_state);
        DPRINTF("==>ReceivedKey: %X\n", mp_context->buf[SECURITY_2_NET_KEY_REP_GRANT_KEY_POS]);
        DPRINTF("==>KeyExchange: %X\n", mp_context->key_exchange);
        DPRINTF("==>KeyGranted : %X\n", mp_context->key_granted);
        DPRINTF("==>LoadedKeys: %X\n", mp_context->loaded_keys);
#endif

#if defined(ZWAVE_PSA_SECURE_VAULT) && defined(ZWAVE_PSA_AES)
       size_t out_len;
       zw_status_t status;
       uint32_t ccm_key_id = ZWAVE_CCM_TEMP_DEC_KEY_ID;
       if (p_context->is_keys_restored == false) {
         DPRINTF("==> Wrapping using temp <===\n");
         /* Import key into secure vault */
         zw_wrap_aes_key_secure_vault(&ccm_key_id, p_context->sg[span->class_id].enc_key, ZW_PSA_ALG_CCM);
       } else {
         /* Use secure vault for encryption using PSA APIs */
         ccm_key_id = convert_keyclass_to_derived_key_id(convert_key_slot_to_keyid(span->class_id), ZWAVE_KEY_TYPE_SINGLE_CAST);
         DPRINTF("==> Using Persistent:KeyId: Conversion  %lX <==\n", ccm_key_id);
       }
        status = zw_psa_aead_decrypt_ccm(ccm_key_id, nonce, ZWAVE_PSA_AES_NONCE_LENGTH, aad, aad_len,
                                ciphertext, ciphertext_len, ciphertext, ciphertext_len+ZWAVE_PSA_AES_MAC_LENGTH, &out_len);
        if (status == ZW_PSA_ERROR_INVALID_SIGNATURE) {
          decrypt_len = 0;
        } else {
          decrypt_len = out_len;
        }
        /* Remove key from vault */
        if (p_context->is_keys_restored == false) {
          zw_psa_destroy_key(ccm_key_id);
        }
#else
        decrypt_len = CCM_decrypt_and_auth(p_context->sg[span->class_id].enc_key, nonce, aad, aad_len, ciphertext,
            ciphertext_len);
#endif
        DPRINTF("decrypt_len: %i\n", decrypt_len);

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
              p_context->mpan = mpan;
              S2_fsm_post_event(p_context, GOT_ENC_MSG_MOS,&e);

              //S2_send_nonce_report(p_context, conn, SECURITY_2_NONCE_REPORT_PROPERTIES1_MOS_BIT_MASK);
            }
            else
            {
              next_mpan_state(mpan);
            }
          }
          break;
        }
      }

      if (p_context->fsm != IDLE || span->state == SPAN_NEGOTIATED)
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
      memcpy(ciphertext, p_context->workbuf, ciphertext_len);

      /*reset prng to the negotiated state with the right new test key */
      next_nonce_instantiate(&span->d.rng, s_nonce, r_nonce, p_context->sg[span->class_id].nonce_key);
    }
  }
  else
  {
    /*Multicast decryption*/
#ifdef ZWAVE_PSA_AES
    uint32_t key_id = ZWAVE_CCM_TEMP_ENC_KEY_ID;
    zw_wrap_aes_key_secure_vault(&key_id, p_context->sg[mpan->class_id].mpan_key, ZW_PSA_ALG_ECB_NO_PAD);
    /* Import key into secure vault */
    zw_psa_aes_ecb_encrypt(key_id, mpan->inner_state, nonce);
    /* Remove key from vault */
    zw_psa_destroy_key(key_id);
#else
    AES128_ECB_encrypt(mpan->inner_state, p_context->sg[mpan->class_id].mpan_key, nonce);
#endif
    next_mpan_state(mpan);

#if defined(ZWAVE_PSA_SECURE_VAULT) && defined(ZWAVE_PSA_AES)
        size_t out_len;
        key_id = ZWAVE_CCM_TEMP_DEC_KEY_ID;
        zw_status_t status;
        /* Import key into secure vault */
        zw_wrap_aes_key_secure_vault(&key_id, p_context->sg[mpan->class_id].enc_key, ZW_PSA_ALG_CCM);
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
    decrypt_len = CCM_decrypt_and_auth(p_context->sg[mpan->class_id].enc_key, nonce, aad, aad_len, ciphertext,
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
        mpan = find_mpan_by_group_id(p_context, conn->r_node, ext_data[2],1);
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

  parse_fail: DPRINT("Parse fail!\n");

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
    S2_send_nonce_report(p_context, conn,
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
  return peer->l_node == p_context->peer.l_node && peer->r_node == p_context->peer.r_node;
}

/*
 * Set the peer and message data
 */
static void
S2_set_peer(struct S2* p_context, const s2_connection_t* peer, const uint8_t* buf, uint16_t len)
{
  p_context->peer = *peer;
  p_context->buf = buf; //TODO decide if we want a local copy?
  p_context->length = len;
}

/***************** PUBLIC functions ********************/
/* Send S2 encrypted frame
 * \param dst Destination nodeid. Security scheme as 0=UNAUTH, 1=AUTH, 2=ACCES, (3=Sec0, not allowed here)
 *
 */
uint8_t
S2_send_data(struct S2* p_context, s2_connection_t* dst, const uint8_t* buf, uint16_t len)
{
  #ifdef ZW_CONTROLLER
  if (IS_LR_NODE(dst->r_node)) {
    convert_normal_to_lr_keyclass(dst);
  }
  #endif
  return S2_send_data_all_cast(p_context, dst, buf, len, SEND_MSG);
}

uint8_t
S2_is_send_data_busy(struct S2* p_context)
{
  return (p_context->fsm != IDLE) && (p_context->fsm != IS_MOS_WAIT_REPLY);
}

void
S2_init_prng(void)
{
  uint8_t entropy[32];
  const uint8_t zeros[32] = { 0 };

  S2_get_hw_random(entropy, sizeof(entropy));
  AES_CTR_DRBG_Instantiate(&s2_ctr_drbg, entropy, zeros);
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

  DPRINT("s2_init_ctx\r\n");

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
  if (class_id >= N_SEC_CLASS)
  {
    return 0;
  }
#ifdef DEBUGPRINT
  DPRINTF("Registered class %d\n",class_id);
  //print_hex(net_key,16);
#endif
  if (temp_key_expand)
  {
    tempkey_expand(key_id, net_key, p_context->sg[class_id].enc_key, p_context->sg[class_id].nonce_key, p_context->sg[class_id].mpan_key);
  }
  else
  {
    networkkey_expand(key_id, net_key, p_context->sg[class_id].enc_key, p_context->sg[class_id].nonce_key, p_context->sg[class_id].mpan_key);
#ifdef ZWAVE_PSA_SECURE_VAULT
    if (make_keys_persist_se) {
      DPRINTF("Network key buffer is not empty\n");
      uint32_t ccm_key_id = convert_keyclass_to_derived_key_id(convert_key_slot_to_keyid(class_id), ZWAVE_KEY_TYPE_SINGLE_CAST);
      ASSERT((ccm_key_id >= ZWAVE_PSA_KEY_ID_MIN) && (ccm_key_id <= ZWAVE_PSA_KEY_ID_MAX));
      DPRINTF("Importing SPAN: %lX\n", ccm_key_id);
      zw_wrap_aes_key_secure_vault(&ccm_key_id, p_context->sg[class_id].enc_key, ZW_PSA_ALG_CCM);
      DPRINTF("Imported : %lX\n", ccm_key_id);

      ccm_key_id = convert_keyclass_to_derived_key_id(convert_key_slot_to_keyid(class_id), ZWAVE_KEY_TYPE_MULTI_CAST);
      ASSERT((ccm_key_id >= ZWAVE_PSA_KEY_ID_MIN) && (ccm_key_id <= ZWAVE_PSA_KEY_ID_MAX));
      DPRINTF("Importing MPAN: %lX\n", ccm_key_id);
      zw_wrap_aes_key_secure_vault(&ccm_key_id, p_context->sg[class_id].mpan_key, ZW_PSA_ALG_CCM);
      DPRINTF("Imported : %lX\n", ccm_key_id);
      p_context->is_keys_restored = true;
    }
#endif /*#ifdef ZWAVE_PSA_SECURE_VAULT*/
  }

  p_context->loaded_keys |= 1 << class_id;
  return 1;
}

void
S2_destroy(struct S2* p_context)
{
  memset(p_context, 0, sizeof(struct S2));
#ifndef SINGLE_CONTEXT
  free(p_context);
#endif
}


void
S2_application_command_handler(struct S2* p_context, s2_connection_t* src, uint8_t* buf, uint16_t len)
{
  uint8_t *plain_text;
  uint16_t plain_text_len;
  decrypt_return_code_t rc;
  uint8_t n_commands_supported;
  const uint8_t* classes;
  event_data_t d;

  d.d.buf.buffer = buf;
  d.d.buf.len = len;
  d.con = src;

  if (buf[0] != COMMAND_CLASS_SECURITY_2)
  {
    return;
  }

  switch (buf[1])
  {
  case SECURITY_2_NONCE_GET:
    DPRINT("Got NONCE Get\r\n");
    if ((src->rx_options & S2_RXOPTION_MULTICAST) != S2_RXOPTION_MULTICAST)
    {
      if( (len >=3) && S2_verify_seq(p_context, src,buf[2]) ) {
        S2_send_nonce_report(p_context,src,SECURITY_2_NONCE_REPORT_PROPERTIES1_SOS_BIT_MASK);
      }
    }
    break;
  case SECURITY_2_NONCE_REPORT:
    DPRINTF("Got NONCE Report %u\r\n", p_context->fsm);
    S2_fsm_post_event(p_context, GOT_NONCE_RAPORT, &d);
    ;
    break;
  case SECURITY_2_MESSAGE_ENCAPSULATION:
    rc = S2_decrypt_msg(p_context, src, buf, len, &plain_text, &plain_text_len);
    if (rc == AUTH_OK)
    {
      DPRINTF("decrypt ok %i\n", rc);

      S2_fsm_post_event(p_context, GOT_ENC_MSG, &d);
      if (plain_text_len)
      {
        if (plain_text[0] == COMMAND_CLASS_SECURITY_2 &&
            !(plain_text[1] == SECURITY_2_COMMANDS_SUPPORTED_REPORT))
        {
          if(src->rx_options & S2_RXOPTION_MULTICAST) {
            //S2 encrypted multi-cast frames shouln't exist.
            return;
          }

          if (plain_text[1] == SECURITY_2_COMMANDS_SUPPORTED_GET)
          {
            p_context->u.commands_sup_report_buf[0] = COMMAND_CLASS_SECURITY_2;
            p_context->u.commands_sup_report_buf[1] = SECURITY_2_COMMANDS_SUPPORTED_REPORT;

            S2_get_commands_supported(src->l_node,src->class_id, &classes, &n_commands_supported);

            if (n_commands_supported + 2 > sizeof(p_context->u.commands_sup_report_buf))
            {
              DPRINTF("No of command classes supported are more than the buffer limit(%d)\n", sizeof(p_context->u.commands_sup_report_buf)-2);
              DPRINT("Not sending S2 Command Supported Report\n");
              return;
            }
            memcpy(&p_context->u.commands_sup_report_buf[2], classes, n_commands_supported);
            /*TODO If p_context->fsm is busy the report is not going to be sent*/
            S2_send_data(p_context, src, p_context->u.commands_sup_report_buf, n_commands_supported + 2);
          }
          /* Don't validate inclusion_peer.l_node as it may not be initialized yet due to early start */
          else
          {
            p_context->buf = plain_text;
            p_context->length = plain_text_len;
            //Default just send the command to the inclusion fsm
            s2_inclusion_post_event(p_context,src);
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
          S2_msg_received_event(p_context, src, plain_text, plain_text_len);
        }

      }
    }
    else if (rc == AUTH_FAIL)
    {
      DPRINTF("decrypt auth fail %i\n", rc);

      S2_fsm_post_event(p_context, GOT_BAD_ENC_MSG, &d);
      s2_inclusion_decryption_failure(p_context,src);
    }
    else
    {
      DPRINTF("decrypt error %i\n", rc);
    }
    break;
  default:
    /*
     * If S2 is busy, p_context->buf may be in use for sending an encrypted message.
     * KEX_FAIL is an exception. Must be passed to the inclusion fsm to abort all S2 action.
     */
    if((p_context->fsm != IDLE) && (buf[1] != KEX_FAIL)) return;

    if ((src->rx_options & S2_RXOPTION_MULTICAST) != S2_RXOPTION_MULTICAST)
    {
      p_context->buf = buf; //TODO is this a good idea?
      p_context->length = len;
      src->class_id = UNENCRYPTED_CLASS;
      s2_inclusion_post_event(p_context,src);
    }
  }

}

void
S2_timeout_notify(struct S2* p_context)
{
  S2_fsm_post_event(p_context, TIMEOUT, NULL);
}

static void
S2_post_send_done_event(struct S2* p_context, s2_tx_status_t status)
{
  s2_inclusion_send_done(p_context, (status == S2_TRANSMIT_COMPLETE_OK) || (status == S2_TRANSMIT_COMPLETE_VERIFIED));
  S2_send_done_event(p_context, status);
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
  uint8_t nr_flag;
  DPRINT("Got S2 fsm event ");
  ZW_DEBUG_SEND_NUM((uint8_t)e);
  ZW_DEBUG_SEND_NL();
  if(d && d->con) {
    ZW_DEBUG_SEND_STR("Is peer node: ");
    ZW_DEBUG_SEND_NUM(S2_is_peernode(p_context, d->con));
    ZW_DEBUG_SEND_STR(" event data: ");
    ZW_DEBUG_SEND_NUM(d->con->l_node);
    ZW_DEBUG_SEND_NUM(d->con->r_node);
  }
  ZW_DEBUG_SEND_NUM(p_context->peer.l_node);
  ZW_DEBUG_SEND_NUM(p_context->peer.r_node);
  ZW_DEBUG_SEND_NL();

  switch (p_context->fsm)
  {
  case IS_MOS_WAIT_REPLY:
  case IDLE:
    //S2_set_peer(p_context, d->con, d->buffer, d->len);
    if (e == SEND_MSG && S2_span_ok(p_context, d->con))
    {
      S2_set_peer(p_context, d->con, d->d.buf.buffer, d->d.buf.len);
      p_context->retry = 2;

      goto send_msg_state_enter;
    }
    else if (e == SEND_MSG)
    {
      p_context->fsm = WAIT_NONCE_RAPORT;
      p_context->retry = 2;
      DPRINT("WAIT_NONCE_RAPORT\r\n");

      S2_set_peer(p_context, d->con, d->d.buf.buffer, d->d.buf.len);
      S2_send_nonce_get(p_context);
      S2_set_timeout(p_context, SEND_DATA_TIMEOUT);
    }
    else if (e == GOT_NONCE_GET && (d->d.buf.len >= 3) && S2_verify_seq(p_context, d->con, d->d.buf.buffer[2]))
    {
      S2_send_nonce_report(p_context, d->con, SECURITY_2_NONCE_REPORT_PROPERTIES1_SOS_BIT_MASK);
    }
    else if (e == GOT_NONCE_RAPORT )
    {
      S2_set_peer(p_context, d->con, d->d.buf.buffer, d->d.buf.len);
      S2_register_nonce(p_context, d->d.buf.buffer, d->d.buf.len);
      emit_S2_synchronization_event(SOS_EVENT_REASON_UNANSWERED, d);
    }
    else if (e == SEND_MULTICAST)
    {
      S2_set_peer(p_context, d->con, d->d.buf.buffer, d->d.buf.len);
      //For Multicast: 8-bit group_id is stored in d->con->r_node
      p_context->mpan = find_mpan_by_group_id(p_context, 0, d->con->r_node, 1);
      p_context->fsm = SENDING_MSG;
      S2_encrypt_and_send_multi(p_context);
    }
    else if (e == SEND_DONE)
    {
      /* pass message to the inclusion FSM */
      s2_inclusion_send_done(p_context, d->d.tx.status == S2_TRANSMIT_COMPLETE_OK);
    }
    else if (e == GOT_ENC_MSG_MOS)
    {
      S2_set_timeout(p_context, 10);
      S2_set_peer(p_context,d->con,0,0);
      p_context->fsm = IS_MOS_WAIT_REPLY;
    }
    else if (e == TIMEOUT && p_context->fsm == IS_MOS_WAIT_REPLY)
    {
      p_context->mpan = 0;
      p_context->fsm = IDLE;
      S2_send_nonce_report(p_context, &p_context->peer, SECURITY_2_NONCE_REPORT_PROPERTIES1_MOS_BIT_MASK);
    }
    break;
  case WAIT_NONCE_RAPORT:
    if ((e == SEND_DONE) && (d->d.tx.status == S2_TRANSMIT_COMPLETE_OK))
    {
      S2_set_timeout(p_context, d->d.tx.time); //Just shorten timer but stay in this state
    }
    else if ((e == SEND_DONE) || (e == TIMEOUT))
    {
      p_context->fsm = IDLE;
      if (e == TIMEOUT) {
          S2_post_send_done_event(p_context, S2_TRANSMIT_COMPLETE_FAIL);
      } else {
          S2_post_send_done_event(p_context, d->d.tx.status);
      }
    }
    else if ((e == GOT_NONCE_RAPORT) && S2_is_peernode(p_context, d->con))
    {
      DPRINT("GOT_NONCE_RAPORT\r\n");
      if (S2_register_nonce(p_context, d->d.buf.buffer, d->d.buf.len) & SECURITY_2_NONCE_REPORT_PROPERTIES1_SOS_BIT_MASK)
      {
        goto send_msg_state_enter;
      }
    }
    else if ((e == GOT_NONCE_RAPORT) && !S2_is_peernode(p_context, d->con))
    {
      emit_S2_synchronization_event(SOS_EVENT_REASON_UNANSWERED, d);
    }
    break;
  case SENDING_MSG:
    if (e == SEND_DONE)
    {
      p_context->fsm = IDLE;
      S2_post_send_done_event(p_context, d->d.tx.status);
    }
    else if (e == GOT_NONCE_RAPORT && !S2_is_peernode(p_context, d->con))
    {
      emit_S2_synchronization_event(SOS_EVENT_REASON_UNANSWERED, d);
    }
    else
    {
      DPRINTF("Warning got event %i while in state %i", e, p_context->fsm);
    }

    break;
  case VERIFYING_DELIVERY:
    if (e == SEND_DONE)
    { /* shorten timer on ACK */
      if (d->d.tx.status == S2_TRANSMIT_COMPLETE_OK)
      {
        S2_set_timeout(p_context, d->d.tx.time); //Just shorten timer but stay in this state
      }
      else
      { /* bail out */
        p_context->fsm = IDLE;
        S2_post_send_done_event(p_context, d->d.tx.status);
      }
    }
    else if (e == GOT_ENC_MSG && S2_is_peernode(p_context, d->con))
    {
      if(S2_is_node_mos(p_context,d->con->r_node) && (p_context->retry > 0)) {
        p_context->length = 0;
        p_context->peer.tx_options &= ~S2_TXOPTION_VERIFY_DELIVERY;
        goto send_msg_state_enter;
      } else {
        p_context->fsm = IDLE;
        /* Stop S2 timer when we have verified delivery so ZW protocol can go back to sleep */
        S2_stop_timeout(p_context);
        S2_post_send_done_event(p_context, S2_TRANSMIT_COMPLETE_VERIFIED);
      }
    }
    /*
     * As we know there will a nonce report under way, it is better to
     * wait for the NONCE_REPORT for the sake of keeping synchronization.
     *
    else if (e == GOT_BAD_ENC_MSG && S2_is_peernode(p_context, d->con))
    {
      p_context->fsm = IDLE; //No more retries
      S2_post_send_done_event(p_context, S2_TRANSMIT_COMPLETE_FAIL);
    }*/
    else if (e == TIMEOUT)
    {
      p_context->fsm = IDLE; //The frame seems to be handled but we don't know for sure
      S2_post_send_done_event(p_context, S2_TRANSMIT_COMPLETE_OK);
    }
    else if (e == GOT_NONCE_RAPORT && S2_is_peernode(p_context, d->con))
    {
      nr_flag = S2_register_nonce(p_context, d->d.buf.buffer, d->d.buf.len);
      if (nr_flag == 0)
      {
        return;
      }
      else if (nr_flag == SECURITY_2_NONCE_REPORT_PROPERTIES1_MOS_BIT_MASK)
      {
        /* if we only get a MOS flag back, set the payload length to 0,
         * as we don't need to retransmit the payload, we should only
         * send the MPAN*/
        p_context->length = 0;
      }

      if (p_context->retry == 0)
      {
        p_context->fsm = IDLE; //No more retries
        S2_post_send_done_event(p_context, S2_TRANSMIT_COMPLETE_FAIL);
      }
      else
      {
        goto send_msg_state_enter;
        // send again
      }
    }
    else if (e == GOT_NONCE_RAPORT && !S2_is_peernode(p_context, d->con))
    {
      emit_S2_synchronization_event(SOS_EVENT_REASON_UNANSWERED, d);
    }
    else
    {
      DPRINTF("Warning got event %i while in state %i", e, p_context->fsm);
    }
    break;
  default:
    ASSERT(0);
  }

  return;
send_msg_state_enter:
  p_context->fsm = SENDING_MSG;

  if (p_context->peer.tx_options & S2_TXOPTION_VERIFY_DELIVERY)
  {
    p_context->fsm = VERIFYING_DELIVERY;
    S2_set_timeout(p_context, SEND_DATA_TIMEOUT);
  }

  S2_encrypt_and_send(p_context);
  p_context->retry--;


  return;
}

uint8_t
S2_send_data_multicast(struct S2* p_context, const s2_connection_t* con, const uint8_t* buf, uint16_t len)
{
  // No key conversion here, because we get a Group ID and we cannot know
  // which keyset to use. Send data multicast will be for Z-Wave only.
  return S2_send_data_all_cast(p_context, con, buf, len, SEND_MULTICAST);
}

uint8_t
S2_send_data_singlecast_follow_up_with_keyset(struct S2* p_context,
                                              s2_connection_t* connection,
                                              zwave_s2_keyset_t keyset,
                                              const uint8_t* payload,
                                              uint16_t payload_length)
{
  if (keyset == ZWAVE_KEYSET) {
    return S2_send_data_all_cast(p_context,
                                 connection,
                                 payload,
                                 payload_length,
                                 SEND_FOLLOW_UP);

  } else if (keyset == ZWAVE_LONG_RANGE_KEYSET) {
    convert_normal_to_lr_keyclass(connection);
    return S2_send_data_all_cast(p_context,
                                 connection,
                                 payload,
                                 payload_length,
                                 SEND_FOLLOW_UP);
  }

  // Unknown keyset, we ignore.
  return 0;
}

uint8_t S2_send_data_multicast_with_keyset(struct S2* p_context,
                                           s2_connection_t* connection,
                                           zwave_s2_keyset_t keyset,
                                           const uint8_t* payload,
                                           uint16_t payload_length)
{
  if (keyset == ZWAVE_KEYSET) {
    return S2_send_data_all_cast(p_context,
                                 connection,
                                 payload,
                                 payload_length,
                                 SEND_MULTICAST);

  } else if (keyset == ZWAVE_LONG_RANGE_KEYSET) {
    convert_normal_to_lr_keyclass(connection);
    return S2_send_data_all_cast(p_context,
                                 connection,
                                 payload,
                                 payload_length,
                                 SEND_MULTICAST);
  }

  // Unknown keyset, we ignore.
  return 0;
}

uint8_t S2_send_data_singlecast_with_keyset(struct S2* p_context,
                                            s2_connection_t* connection,
                                            zwave_s2_keyset_t keyset,
                                            const uint8_t* payload,
                                            uint16_t payload_length)
{
  if (keyset == ZWAVE_KEYSET) {
    return S2_send_data_all_cast(p_context,
                                 connection,
                                 payload,
                                 payload_length,
                                 SEND_MSG);

  } else if (keyset == ZWAVE_LONG_RANGE_KEYSET) {
    convert_normal_to_lr_keyclass(connection);
    return S2_send_data_all_cast(p_context,
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
  event_data_t e;

  if (len == 0 || len > WORKBUF_SIZE || buf == 0 || S2_is_send_data_busy(p_context))
  {
    return 0;
  }

  e.d.buf.buffer = buf;
  e.d.buf.len = len;
  e.con = con;

  if (ev == SEND_MULTICAST) {
    DPRINTF("S2 send data multi %i->[%i] class %i\n",con->l_node,con->r_node,con->class_id);
    S2_fsm_post_event(p_context, SEND_MULTICAST, &e);

  } else if (ev == SEND_MSG) {
    DPRINTF("S2 send data to %i->%i class %i\n",con->l_node,con->r_node,con->class_id);
    S2_fsm_post_event(p_context, SEND_MSG, &e);

  } else if (ev == SEND_FOLLOW_UP) {
    // Here the user has selected a MGRP Group ID for sending.
    // Make a context MPAN switch, if we can find it
    struct MPAN *new_mpan = find_mpan_by_group_id(p_context, 0, con->mgrp_group_id, 0);
    if (new_mpan) {
      p_context->mpan = new_mpan;
    }
    DPRINTF("S2 send data singlecast follow-up to %i->%i class %i, requested Group ID %i\n",
            con->l_node,
            con->r_node,
            con->class_id,
            con->mgrp_group_id);
    // Convert to a regular message event.
    S2_fsm_post_event(p_context, SEND_MSG, &e);
  }

  return 1;
}

uint8_t
S2_is_send_data_multicast_busy(struct S2* p_context)
{
  return p_context->fsm != IDLE;
}

