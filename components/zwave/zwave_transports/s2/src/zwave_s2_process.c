/******************************************************************************
 * # License
 * <b>Copyright 2021 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/
#include "zwave_controller_transport.h"
#include "zwave_controller_transport_internal.h"
#include "log.h"
#include "timer.hpp"
#include "zwave_controller.h"
#include "zwave_s2_internal.h"
#include "zwave_s2_keystore_int.h"
#include "zwave_s2_network.h"
#include "zwave_s2_transport.h"

#define LOG_TAG "zwave_s2_process"

static struct timer_handle_t s2_timer;
static struct timer_handle_t s2_inclusion_timer;

static void s2_inclusion_timer_callback(void *ptr)
{
    sl_log_debug(LOG_TAG, "S2 inclusion timer has now expired\n");
    zwave_s2_transport_lock();
    s2_inclusion_notify_timeout(s2_ctx);
    zwave_s2_transport_unlock();
}

static void s2_timer_callback(void *ptr)
{
    sl_log_debug(LOG_TAG, "S2 send data timer has now expired\n");
    zwave_s2_transport_lock();
    S2_timeout_notify(s2_ctx);
    zwave_s2_transport_unlock();
}

uint8_t s2_inclusion_set_timeout(struct S2 *ctxt, uint32_t timeout)
{
    (void)ctxt;
    sl_log_debug(LOG_TAG, "Setting S2 Inclusion timeout to: %i ms\n", timeout * 10);
    timer_set(&s2_inclusion_timer, timeout * 10, s2_inclusion_timer_callback, NULL);
    return 0;
}

void s2_inclusion_stop_timeout(void)
{
    timer_stop(&s2_inclusion_timer);
}

void S2_stop_timeout(struct S2 *ctxt)
{
    (void)ctxt;
    timer_stop(&s2_timer);
}

void S2_set_timeout(struct S2 *ctxt, uint32_t interval)
{
    (void)ctxt;
    sl_log_debug(LOG_TAG, "Setting S2 Send Data timeout to: %i ms\n", interval);
    timer_set(&s2_timer, interval, s2_timer_callback, NULL);
}

static void zwave_s2_on_network_address_update(zwave_home_id_t home_id, zwave_node_id_t node_id)
{
    (void)node_id;
    zwave_s2_refresh_home_id(home_id);
}

static void zwave_s2_on_new_network_entered(zwave_home_id_t home_id, zwave_node_id_t node_id, zwave_keyset_t granted_keys, zwave_kex_fail_type_t kex_fail_type)
{
    (void)node_id;
    (void)granted_keys;
    (void)kex_fail_type;
    zwave_s2_refresh_home_id(home_id);
}

static void zwave_s2_init()
{
    static zwave_controller_callbacks_t callbacks = {
      .on_network_address_update  = zwave_s2_on_network_address_update,
      .on_new_network_entered     = zwave_s2_on_new_network_entered,
      .on_multicast_group_deleted = zwave_s2_on_on_multicast_group_deleted,
    };

    zwave_controller_register_callbacks(&callbacks);

    static zwave_controller_transport_t transport = {
      .priority          = 2,
      .command_class     = COMMAND_CLASS_SECURITY_2,
      .version           = COMMAND_CLASS_SECURITY_2_VERSION,
      .send_data         = zwave_s2_send_data,
      .abort_send_data   = zwave_s2_abort_send_data,
      .on_frame_received = zwave_s2_on_frame_received,
      .is_busy           = zwave_s2_transport_is_busy,
    };
    zwave_controller_transport_register(&transport);

    zwave_s2_keystore_init();
    zwave_s2_network_init();
    zwave_s2_transport_init();
}

void zwave_s2_process_init(void)
{
    timer_stop(&s2_timer);
    timer_stop(&s2_inclusion_timer);
    zwave_s2_init();
}
