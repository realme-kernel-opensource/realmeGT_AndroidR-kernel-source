/**************************************************************
 * Copyright (c)  2008- 2030  Oplus Mobile communication Corp.ltd
 *
 * File       : touchpanel_event_notify.h
 * Description: source file for Touch event notify
 * Version   : 1.0
 * Date        : 2020-09-14
 * Author    : Qicai.Gu@Bsp.Group.Tp
 * TAG         : BSP.TP.Function
 ****************************************************************/
#ifndef _TOUCHPANEL_EVENTNOTIFY_H
#define _TOUCHPANEL_EVENTNOTIFY_H

#define EVENT_ACTION_FOR_FINGPRINT 0x01

struct touchpanel_event {
    int touchpanel_id;
    struct timeval time;
    int x;
    int y;
    int fid;       /* Finger ID */
    char type;     /* 'D' - Down, 'M' - Move, 'U' - Up, */
    int touch_state;
    int area_rate;
};

#define EVENT_TYPE_DOWN    'D'
#define EVENT_TYPE_MOVE    'M'
#define EVENT_TYPE_UP      'U'

/* caller API */
int touchpanel_event_register_notifier(struct notifier_block *nb);
int touchpanel_event_unregister_notifier(struct notifier_block *nb);

/* callee API */
void touchpanel_event_call_notifier(unsigned long action, void *data);

#endif
