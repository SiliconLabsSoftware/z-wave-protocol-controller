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
#include <stdlib.h>
#include <unistd.h>

#include "sl_log.h"
#include "uic_main.h"
#include "uic_main_externals.h"
#include "zwave_controller.h"
#include "zwave_network_management_fixt.h"
#include "zwave_rx_fixt.h"
#include "zwave_s2_fixt.h"
#include "zwave_tx_fixt.h"
#include "zwave_transports_fixt.h"
#include "process.h"


typedef void (*blocked_cb_t)(void);

static int pipefd[2];

PROCESS(block_process, "Block process");

void testlib_block(blocked_cb_t cb)
{
  const int result __attribute__((unused)) = write(pipefd[1], &cb, sizeof(cb));
}

void block_poll()
{
  blocked_cb_t cb;
  const int result __attribute__((unused)) = read(pipefd[0], &cb, sizeof(cb));
  if (cb)
    cb();
}

PROCESS_THREAD(block_process, ev, data)
{
  (void)(data);
  PROCESS_POLLHANDLER(block_poll());

  PROCESS_BEGIN();

  while (1) {
    PROCESS_WAIT_EVENT();
  }

  PROCESS_END();
}

typedef uint32_t (*get_tick_t)(void);
get_tick_t get_tick_fcn;

void testlib_set_emulated_tick_mode(get_tick_t get_tick)
{
  get_tick_fcn = get_tick;
}

typedef void (*post_init_cb_t)(void);

static post_init_cb_t post_init_cb;

void testlib_register_post_init_cb(post_init_cb_t cb)
{
  post_init_cb = cb;
}

sl_status_t post_init(void)
{
  const int result __attribute__((unused)) = pipe(pipefd);
  uic_main_ext_register_rfd(pipefd[0], NULL, &block_process);

  process_start(&block_process, NULL);

  if (post_init_cb)
    post_init_cb();

  return SL_STATUS_OK;
}

static uic_fixt_setup_step_t uic_fixt_setup_steps_list[] = {
  /* The Z-Wave API depends on the configuration component. */
  {&zwave_rx_fixt_setup, "Z-Wave RX"},
  /** Start Z-Wave TX */
  {&zwave_tx_fixt_setup, "Z-Wave TX"},
  /** Let the Network management initialize its state machine
      and cache our NodeID/ HomeID from the Z-Wave API */
  {&zwave_network_management_fixt_setup, "Z-Wave Network Management"},
  /** Start Z-Wave S2 */
  {&zwave_s2_fixt_setup, "Z-Wave S2"},
  /** Initializes all Z-Wave transports */
  {&zwave_transports_init, "Z-Wave Transports"},
  {&post_init, "post_init"},
  {NULL, "Terminator"}};

/** Final tear-down steps.
 *
 * @ingroup uic_fixt_int
 *
 * Functions to take down components and free allocated resources. The
 * function are executed in order, after all contiki processes have
 * been stopped.
 */
static uic_fixt_shutdown_step_t uic_fixt_shutdown_steps_list[]
  = {{&zwave_network_management_fixt_teardown, "Z-Wave Network Management"},
     {&zwave_rx_fixt_teardown, "Z-Wave RX"},
     {NULL, "Terminator"}};

int main(int argc, char **argv)
{
  const zwave_rx_config_t rx_config = {
    .serial_port               = argv[1],
    .serial_log_file           = argv[2],
    .zwave_rf_region           = argv[4],
    .zwave_normal_tx_power_dbm = 0,
    .zwave_measured_0dbm_power = 0,
    .zwave_max_lr_tx_power_dbm = 0,
  };
  zwave_rx_set_config(&rx_config);

  const zwave_controller_config_t controller_config = {
    .zpc_basic_device_type    = 2,
    .zpc_generic_device_type  = 1,
    .zpc_specific_device_type = 0,
  };
  zwave_controller_set_config(&controller_config);

  sl_log_init(argv[3]);

  return uic_main(uic_fixt_setup_steps_list,
                  uic_fixt_shutdown_steps_list,
                  argc,
                  argv,
                  CMAKE_PROJECT_VERSION);
}
