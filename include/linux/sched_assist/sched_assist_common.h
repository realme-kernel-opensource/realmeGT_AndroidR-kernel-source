/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2020 Oplus. All rights reserved.
 */


#ifndef _OPLUS_SCHED_COMMON_H_
#define _OPLUS_SCHED_COMMON_H_

#define ux_err(fmt, ...) \
		printk_deferred(KERN_ERR "[SCHED_ASSIST_ERR][%s]"fmt, __func__, ##__VA_ARGS__)
#define ux_warn(fmt, ...) \
		printk_deferred(KERN_WARNING "[SCHED_ASSIST_WARN][%s]"fmt, __func__, ##__VA_ARGS__)
#define ux_debug(fmt, ...) \
		printk_deferred(KERN_INFO "[SCHED_ASSIST_INFO][%s]"fmt, __func__, ##__VA_ARGS__)

#define UX_MSG_LEN 64
#define UX_DEPTH_MAX 5

/* define for sched assist thread type, keep same as the define in java file */
#define SA_OPT_CLEAR     (0)
#define SA_TYPE_LIGHT    (1 << 0)
#define SA_TYPE_HEAVY    (1 << 1)
#define SA_TYPE_ANIMATOR (1 << 2)
#define SA_TYPE_LISTPICK (1 << 3)
#define SA_OPT_SET       (1 << 7)
#define SA_TYPE_INHERIT  (1 << 8)
#define SA_TYPE_ONCE_UX  (1 << 9)
#define SA_TYPE_ID_CAMERA_PROVIDER  (1 << 10)
#define SA_TYPE_ID_ALLOCATOR_SER    (1 << 11)



#define SCHED_ASSIST_UX_MASK (0xFF)

/* define for sched assist scene type, keep same as the define in java file */
#define SA_SCENE_OPT_CLEAR  (0)
#define SA_LAUNCH           (1 << 0)
#define SA_SLIDE            (1 << 1)
#define SA_CAMERA           (1 << 2)
#define SA_SCENE_OPT_SET    (1 << 7)

enum UX_STATE_TYPE
{
	UX_STATE_INVALID = 0,
	UX_STATE_NONE,
	UX_STATE_SCHED_ASSIST,
	UX_STATE_INHERIT,
	MAX_UX_STATE_TYPE,
};

enum INHERIT_UX_TYPE
{
	INHERIT_UX_BINDER = 0,
	INHERIT_UX_RWSEM,
	INHERIT_UX_MUTEX,
	INHERIT_UX_SEM,
	INHERIT_UX_FUTEX,
	INHERIT_UX_MAX,
};

struct rq;
extern int ux_prefer_cpu[];

extern void ux_init_rq_data(struct rq *rq);
extern void ux_init_cpu_data(void);

extern bool test_list_pick_ux(struct task_struct *task);
extern void enqueue_ux_thread(struct rq *rq, struct task_struct *p);
extern void dequeue_ux_thread(struct rq *rq, struct task_struct *p);
extern void pick_ux_thread(struct rq *rq, struct task_struct **p, struct sched_entity **se);

extern void inherit_ux_dequeue(struct task_struct *task, int type);
extern void inherit_ux_dequeue_refs(struct task_struct *task, int type, int value);
extern void inherit_ux_enqueue(struct task_struct *task, int type, int depth);
extern void inherit_ux_inc(struct task_struct *task, int type);
extern void inherit_ux_sub(struct task_struct *task, int type, int value);

extern void set_inherit_ux(struct task_struct *task, int type, int depth, int inherit_val);
extern void reset_inherit_ux(struct task_struct *inherit_task, struct task_struct *ux_task, int reset_type);
extern void unset_inherit_ux(struct task_struct *task, int type);
extern void unset_inherit_ux_value(struct task_struct *task, int type, int value);
extern void inc_inherit_ux_refs(struct task_struct *task, int type);

extern bool test_task_ux(struct task_struct *task);
extern bool test_task_ux_depth(int ux_depth);
extern bool test_inherit_ux(struct task_struct *task, int type);
extern bool test_set_inherit_ux(struct task_struct *task);
extern int get_ux_state_type(struct task_struct *task);

extern bool test_ux_task_cpu(int cpu);
extern bool test_ux_prefer_cpu(struct task_struct *task, int cpu);
extern void find_ux_task_cpu(struct task_struct *task, int *target_cpu);
static inline void find_slide_boost_task_cpu(struct task_struct *task, int *target_cpu) {}

extern void place_entity_adjust_ux_task(struct cfs_rq *cfs_rq, struct sched_entity *se, int initial);
extern bool should_ux_task_skip_further_check(struct sched_entity *se);
extern bool should_ux_preempt_wakeup(struct task_struct *wake_task, struct task_struct *curr_task);
extern bool should_ux_task_skip_cpu(struct task_struct *task, unsigned int cpu);
extern void set_ux_task_to_prefer_cpu(struct task_struct *task, int *orig_target_cpu);
extern int set_ux_task_cpu_common_by_prio(struct task_struct *task, int *target_cpu, bool boost, bool prefer_idle, unsigned int type);
extern bool ux_skip_sync_wakeup(struct task_struct *task, int *sync);

extern bool test_task_identify_ux(struct task_struct *task, int id_type_ux);
#ifdef CONFIG_SCHED_WALT
extern bool sched_assist_task_misfit(struct task_struct *task, int cpu, int flag);
#else
static inline bool sched_assist_task_misfit(struct task_struct *task, int cpu, int flag) { return false; }
#endif
static inline bool is_heavy_ux_task(struct task_struct *task)
{
	return task->ux_state == 2;
}

static inline void set_once_ux(struct task_struct *task)
{
	task->ux_state |= SA_TYPE_ONCE_UX;
}

static inline bool sched_assist_scene(unsigned int scene)
{
	if (unlikely(!sysctl_sched_assist_enabled))
		return false;

	switch (scene) {
		case SA_LAUNCH:
			return sysctl_sched_assist_scene & SA_LAUNCH;
		case SA_SLIDE:
			return sysctl_slide_boost_enabled;
		case SA_CAMERA:
			return sysctl_sched_assist_scene & SA_CAMERA;
		default:
			return false;
	}
}
#endif
