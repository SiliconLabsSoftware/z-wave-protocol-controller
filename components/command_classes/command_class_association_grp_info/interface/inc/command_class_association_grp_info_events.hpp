/******************************************************************************
 * # License
 * <b>Copyright 2025 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/

#ifndef COMMAND_CLASS_ASSOCIATION_GRP_INFO_EVENTS_H
#define COMMAND_CLASS_ASSOCIATION_GRP_INFO_EVENTS_H

#include <stdint.h>

// NOTE: The *_GET_* events below are request/response queries served by AGI.
// Callers must invoke them through component_connector::fire_event_async<Payload, Result>(...).get()
// from a thread that is NOT the component_connector worker thread, otherwise the
// blocking .get() will deadlock the single-threaded worker.
enum class command_class_association_grp_info_events_t : uint32_t {
    COMMAND_CLASS_ASSOCIATION_GRP_INFO_BASE_EVENT = (89 << 8),
    COMMAND_CLASS_ASSOCIATION_GRP_INFO_GROUP_NAME_GET,
    COMMAND_CLASS_ASSOCIATION_GRP_INFO_GROUP_INFO_GET,
    COMMAND_CLASS_ASSOCIATION_GRP_INFO_GROUP_COMMAND_LIST_GET,
    COMMAND_CLASS_ASSOCIATION_GRP_INFO_GROUP_NAME_REPORT,
    COMMAND_CLASS_ASSOCIATION_GRP_INFO_GROUP_INFO_REPORT,
    COMMAND_CLASS_ASSOCIATION_GRP_INFO_GROUP_COMMAND_LIST_REPORT,
    COMMAND_CLASS_ASSOCIATION_GRP_INFO_ADD_LIFELINE_NODE,
    COMMAND_CLASS_ASSOCIATION_GRP_INFO_REMOVE_LIFELINE_NODE,
    COMMAND_CLASS_ASSOCIATION_GRP_INFO_CHECK_COMMAND_IN_GROUP_LIST,
    COMMAND_CLASS_ASSOCIATION_GRP_INFO_CHECK_COMMAND_IN_GROUP_LIST_RESULT,
    COMMAND_CLASS_ASSOCIATION_GRP_INFO_GET_LIFELINE_DESTINATIONS,
    COMMAND_CLASS_ASSOCIATION_GRP_INFO_GET_SUPPORTED_GROUPINGS_COUNT,
    COMMAND_CLASS_ASSOCIATION_GRP_INFO_GET_GROUP_MAX_NODES,
    COMMAND_CLASS_ASSOCIATION_GRP_INFO_GET_GROUP_DESTINATIONS,
    COMMAND_CLASS_ASSOCIATION_GRP_INFO_ADD_LIFELINE_COMMAND,
    COMMAND_CLASS_ASSOCIATION_GRP_INFO_REMOVE_LIFELINE_COMMAND,
    COMMAND_CLASS_ASSOCIATION_GRP_INFO_GET_LIFELINE_COMMAND_LIST,
};

#endif  // COMMAND_CLASS_ASSOCIATION_GRP_INFO_EVENTS_H
