/**
 * @file fsm_header.h
 */
#pragma once

/** States */
enum PICKUP_STATES {
  PICKUP_STATE_LISTENING = 0,
  PICKUP_STATE_SEEING = 1,
  PICKUP_STATE_SELECTING_OBJECT = 2,
  PICKUP_STATE_SELECTED_PROCESSING = 3,
  PICKUP_STATE_PLANNING = 4,
  PICKUP_STATE_EXECUTING = 5,
  PICKUP_STATE_NOTIFYING = 6,
  PICKUP_STATE_DONE = 7,
  PICKUP_STATE_GOING_BACK = 8,
  PICKUP_STATE_NONE = 9
};

/** Actions */
enum PICKUP_ACTIONS {
  
  PICKUP_ACTION_NONE = 0,
  PICKUP_ACTION_PERCEPTION_GRAB_IMG = 1,
  PICKUP_ACTION_CLIENT_CHOOSE_IMG = 2,
  PICKUP_ACTION_PERCEPTION_SEND = 3,
  PICKUP_ACTION_PLAN_PLAN = 4,
  PICKUP_ACTION_PLAN_EXECUTE = 5,
  PICKUP_ACTION_PLAN_GO_BACK = 6,
  PICKUP_ACTION_CLIENT_NOTIFY = 7,
  PICKUP_ACTION_PLAN_RESET = 8
  };

/** Msg Status: Notify the FSA of the result or consequence of something */
enum PICKUP_MSG_STATUS_TYPES {

  PICKUP_MSG_STATUS_PERCEPTION_IMG_YES = 1,
  PICKUP_MSG_STATUS_PERCEPTION_IMG_NO = 2,
  PICKUP_MSG_STATUS_CLIENT_CHOOSE_YES = 3,
  PICKUP_MSG_STATUS_CLIENT_CHOOSE_NO = 4,
  PICKUP_MSG_STATUS_PERCEPTION_SENT_YES = 5,
  PICKUP_MSG_STATUS_PERCEPTION_SENT_NO = 6,
  PICKUP_MSG_STATUS_PLAN_PLANNED_YES = 7,
  PICKUP_MSG_STATUS_PLAN_PLANNED_NO = 8,
  PICKUP_MSG_STATUS_PLAN_EXECUTED_YES = 9,
  PICKUP_MSG_STATUS_PLAN_EXECUTED_NO = 10,
  PICKUP_MSG_STATUS_CLIENT_NOTIFIED = 11,
  PICKUP_MSG_STATUS_PLAN_GO_BACK_YES = 12
};

enum PICKUP_MSG_COMMAND_TYPES {
  PICKUP_MSG_CMD_SERVER_CONNECTED = 12,
  PICKUP_MSG_CMD_SERVER_START = 13,
  PICKUP_MSG_CMD_SERVER_RESET = 14,
  PICKUP_MSG_CMD_PERCEPTION_GRAB_IMG = 15,
  PICKUP_MSG_CMD_CLIENT_CHOOSE_IMG = 16,
  PICKUP_MSG_CMD_PERCEPTION_SEND = 17,
  PICKUP_MSG_CMD_PLAN_PLAN = 18,
  PICKUP_MSG_CMD_PLAN_EXECUTE = 19,
  PICKUP_MSG_CMD_CLIENT_NOTIFY = 20,
  PICKUP_MSG_CMD_PLAN_RESET = 21,
  PICKUP_MSG_CMD_SERVER_GO_BACK = 22,
  PICKUP_MSG_CMD_PLAN_GO_BACK = 23
};

enum PICKUP_MSG_NONE_TYPES {
  PICKUP_MSG_TEXT = 21
};
