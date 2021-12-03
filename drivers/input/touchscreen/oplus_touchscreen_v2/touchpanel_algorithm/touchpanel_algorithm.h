/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2018-2020 Oplus. All rights reserved.
 */

#ifndef _TOUCHPANEL_ALGORITHM_H_
#define _TOUCHPANEL_ALGORITHM_H_

#include <linux/uaccess.h>
#include "../touchpanel_common.h"

#define INVALID_POINT   (0xFFFF)
#define KALMAN_LEVEL    (1024)
#define VARIABLE_GAIN   (1024)
#define PROC_MAX_BUF    (255)
#define TIME_ONE_SECOND (1000)
#define KALMAN_GAME_BIT BIT(16)
#define TIME_MAX        (32768)

struct algo_point {
	uint16_t x;
	uint16_t y;
	uint16_t z;
};

typedef enum {
	NORMAL          = 0,
	TOP_START       = BIT(0),
	TOP_END         = BIT(1),
	BOTTOM_START    = BIT(2),
	BOTTOM_END      = BIT(3),
	LEFT_START      = BIT(4),
	LEFT_END        = BIT(5),
	RIGHT_START     = BIT(6),
	RIGHT_END       = BIT(7),
	CLICK_UP        = BIT(8),
} STRETCH_TOUCH_STATUS;


struct algo_point_buf {
	uint16_t touch_time;
	uint16_t delta_time;
	STRETCH_TOUCH_STATUS status;
	struct algo_point down_point;
	struct algo_point kal_p_last;
	struct algo_point kal_x_last;
};

typedef enum {
	BOTTOM_SIDE     = BIT(0),
	LEFT_SIDE       = BIT(1),
	RIGHT_SIDE      = BIT(2),
} STRETCH_PHONE_DIRECTION;

struct algo_stretch_info {
	uint32_t top_frame;
	uint32_t top_stretch_time;
	uint32_t top_open_dir;
	uint32_t top_start;
	uint32_t top_end;

	uint32_t bottom_frame;
	uint32_t bottom_stretch_time;
	uint32_t bottom_open_dir;
	uint32_t bottom_start;
	uint32_t bottom_end;

	uint32_t left_frame;
	uint32_t left_stretch_time;
	uint32_t left_open_dir;
	uint32_t left_start;
	uint32_t left_end;

	uint32_t right_frame;
	uint32_t right_stretch_time;
	uint32_t right_open_dir;
	uint32_t right_start;
	uint32_t right_end;
};

struct algo_kalman_info {
	uint32_t kalman_dis;
	uint32_t min_dis;
	uint32_t Q_Min;
	uint32_t Q_Max;
	uint32_t enable_bit;
};

struct algo_end_stretch {
	uint32_t end_area;
	uint32_t limit_time;
	uint32_t limit_speed;
	uint32_t stretch_ratio;
};

struct touch_algorithm_info {
	struct algo_point_buf *point_buf;
	struct algo_stretch_info stretch_info;
	struct algo_kalman_info kalman_info;
	struct algo_kalman_info charger_kalman_info;
	struct algo_kalman_info *kalman_info_used;
	STRETCH_PHONE_DIRECTION phone_direction;
	struct algo_end_stretch end_stretch;
	struct algo_point *last_points;
	struct algo_point *last_last_points;
	uint8_t *touch_status;
	uint8_t *last_status;
	ktime_t last_time;
	bool enable_kalman;
};

void switch_kalman_fun(struct touchpanel_data *ts, bool game_switch);

int touch_algorithm_handle(struct touchpanel_data *ts, int obj_attention,
			   struct point_info *points);

void oplus_touch_algorithm_init(struct touchpanel_data *ts);

void set_algorithm_direction(struct touchpanel_data *ts, int dir);

#endif  /*_TOUCHPANEL_ALGORITHM_H_*/

