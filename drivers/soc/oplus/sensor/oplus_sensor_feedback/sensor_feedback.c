// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2018-2020 Oplus. All rights reserved.
 */


#define pr_fmt(fmt) "<sensor_feedback>" fmt

#include <linux/init.h>
#include <linux/module.h>

#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/atomic.h>
#include <linux/kthread.h>
#include <linux/soc/qcom/smem.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/err.h>
#include <linux/time64.h>

#include "sensor_feedback.h"
#ifdef CONFIG_OPLUS_FEATURE_FEEDBACK
#include <soc/oplus/system/kernel_fb.h>
#endif

#define SENSOR_DEVICE_TYPE      "10002"
#define SENSOR_POWER_TYPE       "10003"
#define SENSOR_STABILITY_TYPE   "10004"
#define SENSOR_PFMC_TYPE        "10005"
#define SENSOR_MEMORY_TYPE      "10006"


#define SENSOR_DEBUG_DEVICE_TYPE      "20002"
#define SENSOR_DEBUG_POWER_TYPE       "20003"
#define SENSOR_DEBUG_STABILITY_TYPE   "20004"
#define SENSOR_DEBUG_PFMC_TYPE        "20005"
#define SENSOR_DEBUG_MEMORY_TYPE      "20006"


struct msm_rpmh_master_stats {
	uint32_t version_id;
	uint32_t counts;
	uint64_t last_entered;
	uint64_t last_exited;
	uint64_t accumulated_duration;
};


static struct sensor_fb_cxt *g_sensor_fb_cxt = NULL;

/*fb_field :maxlen 19*/
struct sensor_fb_conf g_fb_conf[] = {
	{PS_INIT_FAIL_ID, "device_ps_init_fail", SENSOR_DEVICE_TYPE},
	{PS_I2C_ERR_ID, "device_ps_i2c_err", SENSOR_DEVICE_TYPE},
	{PS_ALLOC_FAIL_ID, "device_ps_alloc_fail", SENSOR_DEVICE_TYPE},
	{PS_ESD_REST_ID, "device_ps_esd_reset", SENSOR_DEVICE_TYPE},
	{PS_NO_INTERRUPT_ID, "device_ps_no_irq", SENSOR_DEVICE_TYPE},
	{PS_FIRST_REPORT_DELAY_COUNT_ID, "device_ps_rpt_delay", SENSOR_DEVICE_TYPE},
	{PS_ORIGIN_DATA_TO_ZERO_ID, "device_ps_to_zero", SENSOR_DEVICE_TYPE},

	{ALS_INIT_FAIL_ID, "device_als_init_fail", SENSOR_DEVICE_TYPE},
	{ALS_I2C_ERR_ID, "device_als_i2c_err", SENSOR_DEVICE_TYPE},
	{ALS_ALLOC_FAIL_ID, "device_als_alloc_fail", SENSOR_DEVICE_TYPE},
	{ALS_ESD_REST_ID, "device_als_esd_reset", SENSOR_DEVICE_TYPE},
	{ALS_NO_INTERRUPT_ID, "device_als_no_irq", SENSOR_DEVICE_TYPE},
	{ALS_FIRST_REPORT_DELAY_COUNT_ID, "device_als_rpt_delay", SENSOR_DEVICE_TYPE},
	{ALS_ORIGIN_DATA_TO_ZERO_ID, "device_als_to_zero", SENSOR_DEVICE_TYPE},

	{ACCEL_INIT_FAIL_ID, "device_acc_init_fail", SENSOR_DEVICE_TYPE},
	{ACCEL_I2C_ERR_ID, "device_acc_i2c_err", SENSOR_DEVICE_TYPE},
	{ACCEL_ALLOC_FAIL_ID, "device_acc_alloc_fail", SENSOR_DEVICE_TYPE},
	{ACCEL_ESD_REST_ID, "device_acc_esd_reset", SENSOR_DEVICE_TYPE},
	{ACCEL_NO_INTERRUPT_ID, "device_acc_no_irq", SENSOR_DEVICE_TYPE},
	{ACCEL_FIRST_REPORT_DELAY_COUNT_ID, "device_acc_rpt_delay", SENSOR_DEVICE_TYPE},
	{ACCEL_ORIGIN_DATA_TO_ZERO_ID, "device_acc_to_zero", SENSOR_DEVICE_TYPE},

	{GYRO_INIT_FAIL_ID, "device_gyro_init_fail", SENSOR_DEVICE_TYPE},
	{GYRO_I2C_ERR_ID, "device_gyro_i2c_err", SENSOR_DEVICE_TYPE},
	{GYRO_ALLOC_FAIL_ID, "device_gyro_alloc_fail", SENSOR_DEVICE_TYPE},
	{GYRO_ESD_REST_ID, "device_gyro_esd_reset", SENSOR_DEVICE_TYPE},
	{GYRO_NO_INTERRUPT_ID, "device_gyro_no_irq", SENSOR_DEVICE_TYPE},
	{GYRO_FIRST_REPORT_DELAY_COUNT_ID, "device_gyro_rpt_delay", SENSOR_DEVICE_TYPE},
	{GYRO_ORIGIN_DATA_TO_ZERO_ID, "device_gyro_to_zero", SENSOR_DEVICE_TYPE},

	{MAG_INIT_FAIL_ID, "device_mag_init_fail", SENSOR_DEVICE_TYPE},
	{MAG_I2C_ERR_ID, "device_mag_i2c_err", SENSOR_DEVICE_TYPE},
	{MAG_ALLOC_FAIL_ID, "device_mag_alloc_fail", SENSOR_DEVICE_TYPE},
	{MAG_ESD_REST_ID, "device_mag_esd_reset", SENSOR_DEVICE_TYPE},
	{MAG_NO_INTERRUPT_ID, "device_mag_no_irq", SENSOR_DEVICE_TYPE},
	{MAG_FIRST_REPORT_DELAY_COUNT_ID, "device_mag_rpt_delay", SENSOR_DEVICE_TYPE},
	{MAG_ORIGIN_DATA_TO_ZERO_ID, "device_mag_to_zero", SENSOR_DEVICE_TYPE},

	{POWER_SENSOR_INFO_ID, "debug_power_sns_info", SENSOR_DEBUG_POWER_TYPE},
	{POWER_ACCEL_INFO_ID, "debug_power_acc_info", SENSOR_DEBUG_POWER_TYPE},
	{POWER_GYRO_INFO_ID, "debug_power_gyro_info", SENSOR_DEBUG_POWER_TYPE},
	{POWER_MAG_INFO_ID, "debug_power_mag_info", SENSOR_DEBUG_POWER_TYPE},
	{POWER_PROXIMITY_INFO_ID, "debug_power_prox_info", SENSOR_DEBUG_POWER_TYPE},
	{POWER_LIGHT_INFO_ID, "debug_power_light_info", SENSOR_DEBUG_POWER_TYPE},
	{POWER_WISE_LIGHT_INFO_ID, "debug_power_wiseligt_info", SENSOR_DEBUG_POWER_TYPE},
	{POWER_WAKE_UP_RATE_ID, "debug_power_wakeup_rate", SENSOR_DEBUG_POWER_TYPE},
	{POWER_ADSP_SLEEP_RATIO_ID, "power_adsp_sleep_ratio", SENSOR_POWER_TYPE},

	{ALAILABLE_SENSOR_LIST_ID, "available_sensor_list", SENSOR_DEBUG_DEVICE_TYPE},
	{ALAILABLE_SENSOR_NAME_ID, "available_sensor_name", SENSOR_DEBUG_DEVICE_TYPE}

};


#define MSM_ARCH_TIMER_FREQ 19200000
static inline u64 get_time_in_msec(u64 counter)
{
	do_div(counter, (MSM_ARCH_TIMER_FREQ / MSEC_PER_SEC));
	return counter;
}

static struct timespec oplus_current_kernel_time(void)
{
	struct timespec64 ts64;
	ktime_get_coarse_real_ts64(&ts64);
	return timespec64_to_timespec(ts64);
}

static unsigned int BKDRHash(char *str, unsigned int len)
{
	unsigned int seed = 131;
	// 31 131 1313 13131 131313 etc..
	unsigned int hash = 0;
	unsigned int i    = 0;

	if (str == NULL) {
		return 0;
	}

	for (i = 0; i < len; str++, i++) {
		hash = (hash * seed) + (*str);
	}

	return hash;
}



#define CRASH_CAUSE_BUF_LEN 256U
char sensor_crash_buf[CRASH_CAUSE_BUF_LEN] = {0x00};

void set_sensor_crash_cause(char *str)
{
	int len = 0;
	unsigned char payload[1024] = {0x00};
	unsigned int hashid = 0;
	char *detailData = "adsp_slpi";
	memset((void *)sensor_crash_buf, 0, sizeof(sensor_crash_buf));

	if ((strncmp(str, "slpi", strlen("slpi")) == 0)
		|| (strncmp(str, "adsp", strlen("adsp")) == 0)) {
		len = snprintf(sensor_crash_buf, sizeof(sensor_crash_buf), "%s", str);
		hashid = BKDRHash(sensor_crash_buf, strlen(sensor_crash_buf));
		len = snprintf(payload, sizeof(payload),
				"NULL$$EventField@@%u$$FieldData@@%s$$detailData@@%s",  hashid,
				sensor_crash_buf,
				detailData);
		pr_info("payload= %s, sensor_crash_buf=%s\n", payload, sensor_crash_buf);

	} else {
		pr_info("is not sensor crash =%s\n", str);
	}

#ifdef CONFIG_OPLUS_FEATURE_FEEDBACK
	oplus_kevent_fb(FB_SENSOR, SENSOR_STABILITY_TYPE, payload);
#endif
}
EXPORT_SYMBOL(set_sensor_crash_cause);

static ssize_t adsp_notify_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct sensor_fb_cxt *sensor_fb_cxt = g_sensor_fb_cxt;
	uint16_t adsp_event_counts = 0;
	spin_lock(&sensor_fb_cxt->rw_lock);
	adsp_event_counts = sensor_fb_cxt->adsp_event_counts;
	spin_unlock(&sensor_fb_cxt->rw_lock);
	pr_info(" adsp_value = %d\n", adsp_event_counts);

	return snprintf(buf, PAGE_SIZE, "%d\n", adsp_event_counts);
}

static ssize_t adsp_notify_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct sensor_fb_cxt *sensor_fb_cxt = g_sensor_fb_cxt;
	uint16_t adsp_event_counts = 0;
	uint16_t node_type = 0;
	int err = 0;

	err = sscanf(buf, "%hu %hu", &node_type, &adsp_event_counts);

	if (err < 0) {
		pr_err("adsp_notify_store error: err = %d\n", err);
		return err;
	}

	spin_lock(&sensor_fb_cxt->rw_lock);
	sensor_fb_cxt->adsp_event_counts = adsp_event_counts;
	sensor_fb_cxt->node_type = node_type;
	spin_unlock(&sensor_fb_cxt->rw_lock);
	pr_info("adsp_notify_store adsp_value = %d, node_type=%d\n", adsp_event_counts,
		node_type);

	set_bit(THREAD_WAKEUP, (unsigned long *)&sensor_fb_cxt->wakeup_flag);
	/*wake_up_interruptible(&sensor_fb_cxt->wq);*/
	wake_up(&sensor_fb_cxt->wq);

	return count;
}


static ssize_t hal_info_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return 0;
}

static ssize_t hal_info_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	pr_info("hal_info_store count = %d\n", count);
	return count;
}

static ssize_t test_id_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	pr_info("test_id_show\n");
	return 0;
}

static ssize_t test_id_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct sensor_fb_cxt *sensor_fb_cxt = g_sensor_fb_cxt;
	uint16_t adsp_event_counts = 0;
	uint16_t node_type = 0;
	uint16_t event_id = 0;
	uint16_t event_data = 0;
	int err = 0;

	err = sscanf(buf, "%hu %hu %hu %hu", &node_type, &adsp_event_counts, &event_id,
			&event_data);

	if (err < 0) {
		pr_err("test_id_store error: err = %d\n", err);
		return err;
	}

	spin_lock(&sensor_fb_cxt->rw_lock);
	sensor_fb_cxt->adsp_event_counts = adsp_event_counts;
	sensor_fb_cxt->node_type = node_type;
	spin_unlock(&sensor_fb_cxt->rw_lock);

	sensor_fb_cxt->fb_smem.event[0].event_id = event_id;
	sensor_fb_cxt->fb_smem.event[0].count = event_data;

	pr_info("test_id_store adsp_value = %d, node_type=%d \n", adsp_event_counts,
		node_type);
	pr_info("test_id_store event_id = %d, event_data=%d \n", event_id, event_data);


	set_bit(THREAD_WAKEUP, (unsigned long *)&sensor_fb_cxt->wakeup_flag);
	/*wake_up_interruptible(&sensor_fb_cxt->wq);*/
	wake_up(&sensor_fb_cxt->wq);

	return count;
}

static ssize_t sensor_list_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct sensor_fb_cxt *sensor_fb_cxt = g_sensor_fb_cxt;
	uint16_t sensor_list[2] = {0x00};
	spin_lock(&sensor_fb_cxt->rw_lock);
	sensor_list[0] = sensor_fb_cxt->sensor_list[0];
	sensor_list[1] = sensor_fb_cxt->sensor_list[1];
	spin_unlock(&sensor_fb_cxt->rw_lock);
	pr_info("phy = 0x%x, virt = 0x%x\n", sensor_list[0], sensor_list[1]);

	return snprintf(buf, PAGE_SIZE, "phy = 0x%x, virt = 0x%x\n", sensor_list[0],
			sensor_list[1]);

}


static ssize_t sensor_list_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	pr_info("sensor_list_store\n");
	return count;
}



DEVICE_ATTR(adsp_notify, 0644, adsp_notify_show, adsp_notify_store);
DEVICE_ATTR(hal_info, 0644, hal_info_show, hal_info_store);
DEVICE_ATTR(test_id, 0644, test_id_show, test_id_store);
DEVICE_ATTR(sensor_list, 0644, sensor_list_show, sensor_list_store);



static struct attribute *sensor_feedback_attributes[] = {
	&dev_attr_adsp_notify.attr,
	&dev_attr_hal_info.attr,
	&dev_attr_test_id.attr,
	&dev_attr_sensor_list.attr,
	NULL
};



static struct attribute_group sensor_feedback_attribute_group = {
	.attrs = sensor_feedback_attributes
};

#define SMEM_SENSOR_FEEDBACK (128)
static int read_data_from_share_mem(struct sensor_fb_cxt *sensor_fb_cxt)
{
	int ret = 0;
	size_t smem_size = 0;
	void *smem_addr = NULL;
	struct fb_event_smem *fb_event = NULL;
	smem_addr = qcom_smem_get(QCOM_SMEM_HOST_ANY,
			SMEM_SENSOR_FEEDBACK,
			&smem_size);

	if (IS_ERR(smem_addr)) {
		pr_err("unable to acquire smem SMEM_SENSOR_FEEDBACK entry\n");
		return -1;
	}

	fb_event = (struct fb_event_smem *)smem_addr;

	if (fb_event == ERR_PTR(-EPROBE_DEFER)) {
		fb_event = NULL;
		return -2;
	}

	memcpy((void *)&sensor_fb_cxt->fb_smem, (void *)fb_event, smem_size);
	return ret;
}


static int find_event_id(int16_t event_id)
{
	int len = sizeof(g_fb_conf) / sizeof(g_fb_conf[0]);
	int ret = -1;
	int index = 0;

	for (index = 0; index < len; index++) {
		if (g_fb_conf[index].event_id == event_id) {
			ret = index;
		}
	}

	return ret;
}


int procce_special_event_id(unsigned short event_id, int count,
	struct sensor_fb_cxt *sensor_fb_cxt)
{
	int ret = 0;

	if (event_id == ALAILABLE_SENSOR_LIST_ID) {
		sensor_fb_cxt->sensor_list[0] = (uint32_t)
			sensor_fb_cxt->fb_smem.event[count].buff[0];
		sensor_fb_cxt->sensor_list[1] = (uint32_t)
			sensor_fb_cxt->fb_smem.event[count].buff[1];
		pr_info("sensor_list virt_sns = 0x%x, phy_sns = 0x%x\n",
			sensor_fb_cxt->sensor_list[0], sensor_fb_cxt->sensor_list[1]);
		ret = 1;

	} else if (event_id == ALAILABLE_SENSOR_NAME_ID) {
		sensor_fb_cxt->gyro_name_index = (uint32_t)
			sensor_fb_cxt->fb_smem.event[count].buff[0];
		pr_info("gyro_name_index = 0x%x\n", sensor_fb_cxt->gyro_name_index);
		ret = 1;
	}

	return ret;
}


static int parse_shr_info(struct sensor_fb_cxt *sensor_fb_cxt)
{
	int ret = 0;
	int count = 0;
	uint16_t event_id = 0;
	int index = 0;
	unsigned char payload[1024] = {0x00};
	int fb_len = 0;
	unsigned char detail_buff[128] = {0x00};

	for (count = 0; count < sensor_fb_cxt->adsp_event_counts; count ++) {
		event_id = sensor_fb_cxt->fb_smem.event[count].event_id;
		pr_info("event_id =%d, count =%d\n", event_id, count);

		index = find_event_id(event_id);

		if (index == -1) {
			pr_info("event_id =%d, count =%d\n", event_id, count);
			continue;
		}

		ret = procce_special_event_id(event_id, count, sensor_fb_cxt);

		if (ret == 1) {
			continue;
		}

		memset(payload, 0, sizeof(payload));
		memset(detail_buff, 0, sizeof(detail_buff));
		snprintf(detail_buff, sizeof(detail_buff), "%d %d %d",
			sensor_fb_cxt->fb_smem.event[count].buff[0],
			sensor_fb_cxt->fb_smem.event[count].buff[1],
			sensor_fb_cxt->fb_smem.event[count].buff[2]);
		fb_len += scnprintf(payload, sizeof(payload),
				"NULL$$EventField@@%s$$FieldData@@%d$$detailData@@%s",
				g_fb_conf[index].fb_field,
				sensor_fb_cxt->fb_smem.event[count].count,
				detail_buff);
		pr_info("payload =%s\n", payload);
#ifdef CONFIG_OPLUS_FEATURE_FEEDBACK
		oplus_kevent_fb(FB_SENSOR, g_fb_conf[index].fb_event_id, payload);
#endif
	}

	return ret;
}

int read_sleep_ratio(struct sensor_fb_cxt *sensor_fb_cxt, int status)
{
	struct msm_rpmh_master_stats *record = NULL;
	uint64_t accumulated_duration = 0;
	uint64_t last_accumulated_duration = 0;
	//int master_id = 2;// 2:adsp, 3:slpi
	//int smem_id = 606;//606:adsp, 613:adsp_island
	int master_id = 0;
	int smem_id = 0;
	uint64_t sleep_time = 0;
	char payload[1024] = {0x00};
	int count = 1;
	char *adsp_sleep_ratio_fied = "power_adsp_sleep_ratio";

	spin_lock(&sensor_fb_cxt->rw_lock);
	sleep_time = sensor_fb_cxt->sleep_time;
	master_id = sensor_fb_cxt->master_id;
	smem_id = sensor_fb_cxt->smem_id;
	spin_unlock(&sensor_fb_cxt->rw_lock);

	record = (struct msm_rpmh_master_stats *) qcom_smem_get(
			master_id,
			smem_id, NULL);

	if (IS_ERR_OR_NULL(record)) {
		return -1;
	}

	spin_lock(&sensor_fb_cxt->rw_lock);

	if (status == 1) {
		accumulated_duration = record->accumulated_duration;
		accumulated_duration = get_time_in_msec(accumulated_duration);//ms
		last_accumulated_duration = get_time_in_msec(
				sensor_fb_cxt->last_accumulated_duration);
		accumulated_duration -= last_accumulated_duration;
		sensor_fb_cxt->sleep_ratio = (accumulated_duration * 100) / sleep_time;
		pr_info("sleep_ratio =%d, accumulated_duration=%d, sleep_time=%d\n",
			sensor_fb_cxt->sleep_ratio, accumulated_duration, sleep_time);

		if ((sleep_time > (2 * 3600 * 1000)) && (sensor_fb_cxt->sleep_ratio < 10)) {
			scnprintf(payload, sizeof(payload),
				"NULL$$EventField@@%s$$FieldData@@%d$$detailData@@%llu",
				adsp_sleep_ratio_fied,
				count,
				sensor_fb_cxt->sleep_ratio);
#ifdef CONFIG_OPLUS_FEATURE_FEEDBACK
			oplus_kevent_fb(FB_SENSOR, SENSOR_POWER_TYPE, payload);
#endif
		}

	} else {
		sensor_fb_cxt->last_accumulated_duration = record->accumulated_duration;
	}

	spin_unlock(&sensor_fb_cxt->rw_lock);

	return 0;
}

static int sensor_report_thread(void *arg)
{
	int ret = 0;
	struct sensor_fb_cxt *sensor_fb_cxt = (struct sensor_fb_cxt *)arg;
	uint16_t node_type = 0;
	pr_info("sensor_feedback: sensor_report_thread step1!\n");

	while (!kthread_should_stop()) {
		wait_event_interruptible(sensor_fb_cxt->wq, test_bit(THREAD_WAKEUP,
				(unsigned long *)&sensor_fb_cxt->wakeup_flag));

		clear_bit(THREAD_WAKEUP, (unsigned long *)&sensor_fb_cxt->wakeup_flag);
		set_bit(THREAD_SLEEP, (unsigned long *)&sensor_fb_cxt->wakeup_flag);
		spin_lock(&sensor_fb_cxt->rw_lock);
		node_type = sensor_fb_cxt->node_type;
		spin_unlock(&sensor_fb_cxt->rw_lock);

		if (node_type == 0) {
			ret = read_data_from_share_mem(sensor_fb_cxt);

		} else if (node_type == 2) { //sleep ratio wakeup
			ret = read_sleep_ratio(sensor_fb_cxt, 1);

		} else if (node_type == 3) { //power done
			ret = read_sleep_ratio(sensor_fb_cxt, 0);

		} else {
			pr_info("sensor_feedback test from node\n");
		}

		ret = parse_shr_info(sensor_fb_cxt);
		spin_lock(&sensor_fb_cxt->rw_lock);
		memset((void *)&sensor_fb_cxt->fb_smem, 0, sizeof(struct fb_event_smem));
		sensor_fb_cxt->adsp_event_counts = 0;
		spin_unlock(&sensor_fb_cxt->rw_lock);
	}

	pr_info("sensor_feedback ret =%s\n", ret);
	return ret;
}

//#ifdef CONFIG_FB
#if defined(CONFIG_DRM_MSM)
static int fb_notifier_callback(struct notifier_block *nb,
	unsigned long event, void *data)
{
	int blank;
	struct msm_drm_notifier *evdata = data;
	struct sensor_fb_cxt *sns_cxt = container_of(nb, struct sensor_fb_cxt,
			fb_notif);
	struct timespec now_time;

	if (!evdata || (evdata->id != 0)) {
		return 0;
	}

	//if(event == MSM_DRM_EARLY_EVENT_BLANK || event == MSM_DRM_EVENT_BLANK)
	if (event == MSM_DRM_EARLY_EVENT_BLANK) {
		blank = *(int *)(evdata->data);

		if (blank == MSM_DRM_BLANK_UNBLANK) { //resume
			now_time = oplus_current_kernel_time();
			spin_lock(&sns_cxt->rw_lock);
			sns_cxt->end_time = (now_time.tv_sec * 1000 + now_time.tv_nsec / 1000000);
			sns_cxt->sleep_time = (sns_cxt->end_time - sns_cxt->start_time);
			sns_cxt->node_type = 2; //adsp sleep ratio type
			spin_unlock(&sns_cxt->rw_lock);

			set_bit(THREAD_WAKEUP, (unsigned long *)&sns_cxt->wakeup_flag);
			/*wake_up_interruptible(&sensor_fb_cxt->wq);*/
			wake_up(&sns_cxt->wq);

			pr_info("%s: ap_sleep time: %ld ms\n", __func__, sns_cxt->sleep_time);

		} else if (blank == MSM_DRM_BLANK_POWERDOWN) { //suspend
			now_time = oplus_current_kernel_time();
			sns_cxt->start_time = (now_time.tv_sec * 1000 + now_time.tv_nsec / 1000000);

			spin_lock(&sns_cxt->rw_lock);
			sns_cxt->node_type = 3; //adsp sleep ratio type
			spin_unlock(&sns_cxt->rw_lock);
			set_bit(THREAD_WAKEUP, (unsigned long *)&sns_cxt->wakeup_flag);
			/*wake_up_interruptible(&sensor_fb_cxt->wq);*/
			wake_up(&sns_cxt->wq);

		} else {
			pr_info("%s: receives wrong data EARLY_BLANK:%d\n", __func__, blank);
		}
	}

	return 0;
}
#elif defined(CONFIG_FB)
static int fb_notifier_callback(struct notifier_block *nb,
	unsigned long event, void *data)
{
	int blank;
	struct fb_event *evdata = data;
	struct sensor_fb_cxt *sns_cxt = container_of(nb, struct sensor_fb_cxt,
			fb_notif);
	struct timespec now_time;

	if (evdata && evdata->data) {
		//if(event == FB_EARLY_EVENT_BLANK || event == FB_EVENT_BLANK)
		if (event == FB_EVENT_BLANK) {
			blank = *(int *)evdata->data;

			if (blank == FB_BLANK_UNBLANK) { //resume
				now_time = oplus_current_kernel_time();
				sns_cxt->end_time = (now_time.tv_sec * 1000 + now_time.tv_nsec / 1000000);
				sns_cxt->sleep_time = (sns_cxt->end_time - sns_cxt->start_time);
				pr_info("%s: ap_sleep time: %ld ms\n", __func__, sns_cxt->sleep_time);

			} else if (blank == FB_BLANK_POWERDOWN) { //suspend
				now_time = oplus_current_kernel_time();
				sns_cxt->start_time = (now_time.tv_sec * 1000 + now_time.tv_nsec / 1000000);

			} else {
				pr_info("%s: receives wrong data EARLY_BLANK:%d\n", __func__, blank);
			}
		}
	}

	return 0;
}
#endif /* CONFIG_DRM_MSM */
//#endif /* CONFIG_FB */

char *gyro_name_arr[] = {
	"null",
	"bmi16x",
	"lsm6ds3c"
};

static ssize_t gyro_name_read_proc(struct file *file, char __user *buf,
	size_t count, loff_t *off)
{
	char page[128] = {0};
	int len = 0;
	struct sensor_fb_cxt *sensor_fb_cxt = (struct sensor_fb_cxt *)PDE_DATA(
			file_inode(file));
	int index = 0;
	int arr_len = sizeof(gyro_name_arr) / sizeof(gyro_name_arr[0]);
	index = sensor_fb_cxt->gyro_name_index;

	if (index > arr_len) {
		index = 0;
	}

	pr_info("arr_len =%d, index=%d\n", arr_len, index);
	len = snprintf(page, sizeof(page), "%s", gyro_name_arr[index]);
	len = simple_read_from_buffer(buf, count, off, page, strlen(page));
	pr_info("gyro_name = %s", gyro_name_arr[index]);
	return len;
}

static struct file_operations gyro_name_fops = {
	.owner = THIS_MODULE,
	.read = gyro_name_read_proc,
};
static ssize_t sensor_list_read_proc(struct file *file, char __user *buf,
	size_t count, loff_t *off)
{
	char page[128] = {0};
	int len = 0;
	struct sensor_fb_cxt *sensor_fb_cxt = (struct sensor_fb_cxt *)PDE_DATA(
			file_inode(file));

	len = snprintf(page, sizeof(page), "phy = 0x%x, virt = 0x%x\n",
			sensor_fb_cxt->sensor_list[0], sensor_fb_cxt->sensor_list[1]);
	len = simple_read_from_buffer(buf, count, off, page, strlen(page));
	pr_info("phy = 0x%x, virt = 0x%x, len=%d \n", sensor_fb_cxt->sensor_list[0],
		sensor_fb_cxt->sensor_list[1],
		len);
	return len;
}

static struct file_operations sensor_list_fops = {
	.owner = THIS_MODULE,
	.read = sensor_list_read_proc,
};


static ssize_t crash_reason_read_proc(struct file *file, char __user *buf,
	size_t count, loff_t *off)
{
	char page[512] = {0x00};
	int len = 0;

	len = snprintf(page, sizeof(page), "%s", sensor_crash_buf);
	len = simple_read_from_buffer(buf, count, off, page, strlen(page));

	return len;
}

static struct file_operations crash_reason_fops = {
	.owner = THIS_MODULE,
	.read = crash_reason_read_proc,
};


static int sensor_feedback_parse_dts(struct platform_device *pdev)
{
	struct device_node *node = pdev->dev.of_node;
	struct sensor_fb_cxt *sensor_fb_cxt = platform_get_drvdata(pdev);
	int rc = 0;
	int master_id = 0;
	int smem_id = 0;

	rc = of_property_read_u32(node, "master-id", &master_id);

	if (rc) {
		pr_err("parse master_id failed rc=%d\n", rc);
		rc = -1;
		return rc;
	}

	rc = of_property_read_u32(node, "smem-id", &smem_id);

	if (rc) {
		pr_err("parse smem_id failed rc=%d\n", rc);
		rc = -2;
		return rc;
	}

	sensor_fb_cxt->master_id = master_id;
	sensor_fb_cxt->smem_id   = smem_id;
	pr_info("parse dts smem_id=%d, master_id=%d\n", smem_id, master_id);

	return rc;
}

static int sensor_feedback_probe(struct platform_device *pdev)
{
	int err = 0;
	struct sensor_fb_cxt *sensor_fb_cxt = NULL;
	struct proc_dir_entry *pentry;

	sensor_fb_cxt = kzalloc(sizeof(struct sensor_fb_cxt), GFP_KERNEL);

	if (sensor_fb_cxt == NULL) {
		pr_err("kzalloc g_sensor_fb_cxt failed\n");
		err = -ENOMEM;
		goto alloc_sensor_fb_failed;
	}

	g_sensor_fb_cxt = sensor_fb_cxt;
	platform_set_drvdata(pdev, sensor_fb_cxt);

	err = sensor_feedback_parse_dts(pdev);

	if (err) {
		pr_err("sensor_feedback_parse_dts failed\n");
		err = -ENOMEM;
		goto alloc_sensor_fb_failed;
	}

	spin_lock_init(&sensor_fb_cxt->rw_lock);
	init_waitqueue_head(&sensor_fb_cxt->wq);

	sensor_fb_cxt->sensor_fb_dev = pdev;
	err = sysfs_create_group(&sensor_fb_cxt->sensor_fb_dev->dev.kobj,
			&sensor_feedback_attribute_group);

	if (err < 0) {
		pr_err("unable to create sensor_feedback_attribute_group file err=%d\n", err);
		goto sysfs_create_failed;
	}

	sensor_fb_cxt->proc_sns =  proc_mkdir("sns_debug", NULL);

	if (!sensor_fb_cxt->proc_sns) {
		pr_err("can't create sns_debug proc\n");
		err = -EFAULT;
		goto sysfs_create_failed;
	}

	pentry = proc_create_data("gyro_name", 0666, sensor_fb_cxt->proc_sns,
			&gyro_name_fops, sensor_fb_cxt);

	if (!pentry) {
		pr_err("create gyro_name proc failed.\n");
		err = -EFAULT;
		goto sysfs_create_failed;
	}

	pentry = proc_create_data("sensor_list", 0666, sensor_fb_cxt->proc_sns,
			&sensor_list_fops, sensor_fb_cxt);

	if (!pentry) {
		pr_err("create sensor_list proc failed.\n");
		err = -EFAULT;
		goto sysfs_create_failed;
	}

	pentry = proc_create_data("crash_reason", 0666, sensor_fb_cxt->proc_sns,
			&crash_reason_fops, sensor_fb_cxt);

	if (!pentry) {
		pr_err("create crash_reason failed.\n");
		err = -EFAULT;
		goto sysfs_create_failed;
	}


	kobject_uevent(&sensor_fb_cxt->sensor_fb_dev->dev.kobj, KOBJ_ADD);

#if defined(CONFIG_DRM_MSM)
	sensor_fb_cxt->fb_notif.notifier_call = fb_notifier_callback;
	err = msm_drm_register_client(&sensor_fb_cxt->fb_notif);

	if (err) {
		pr_err("Unable to register fb_notifier: %d\n", err);
	}

#elif defined(CONFIG_FB)
	sensor_fb_cxt->fb_notif.notifier_call = fb_notifier_callback;
	err = fb_register_client(&sensor_fb_cxt->fb_notif);

	if (err) {
		pr_err("Unable to register fb_notifier: %d\n", err);
	}

#endif/*CONFIG_FB*/


	init_waitqueue_head(&sensor_fb_cxt->wq);

	set_bit(THREAD_SLEEP, (unsigned long *)&sensor_fb_cxt->wakeup_flag);

	sensor_fb_cxt->report_task = kthread_create(sensor_report_thread,
			(void *)sensor_fb_cxt,
			"sensor_feedback_task");

	if (IS_ERR(sensor_fb_cxt->report_task)) {
		err = PTR_ERR(sensor_fb_cxt->report_task);
		goto create_task_failed;
	}


	wake_up_process(sensor_fb_cxt->report_task);

	pr_info("sensor_feedback_init success\n");
	return 0;
create_task_failed:
	sysfs_remove_group(&sensor_fb_cxt->sensor_fb_dev->dev.kobj,
		&sensor_feedback_attribute_group);
sysfs_create_failed:
	kfree(sensor_fb_cxt);
	g_sensor_fb_cxt = NULL;
alloc_sensor_fb_failed:
	return err;
}


static int sensor_feedback_remove(struct platform_device *pdev)
{
	struct sensor_fb_cxt *sensor_fb_cxt = g_sensor_fb_cxt;
	sysfs_remove_group(&sensor_fb_cxt->sensor_fb_dev->dev.kobj,
		&sensor_feedback_attribute_group);
	kfree(sensor_fb_cxt);
	g_sensor_fb_cxt = NULL;
	return 0;
}

static const struct of_device_id of_drv_match[] = {
	{ .compatible = "oplus,sensor-feedback"},
	{},
};
MODULE_DEVICE_TABLE(of, of_drv_match);

static struct platform_driver _driver = {
	.probe      = sensor_feedback_probe,
	.remove     = sensor_feedback_remove,
	.driver     = {
		.name       = "sensor_feedback",
		.of_match_table = of_drv_match,
	},
};

static int __init sensor_feedback_init(void)
{
	pr_info("sensor_feedback_init call\n");

	platform_driver_register(&_driver);
	return 0;
}

/*
static int __exit sensor_feedback_exit(void)
{
	pr_info("sensor_feedback_exit call\n");

	platform_driver_unregister(&_driver);
	return 0;
}*/


core_initcall(sensor_feedback_init);

//module_init(sensor_feedback_init);
//module_exit(sensor_feedback_exit);


MODULE_AUTHOR("JangHua.Tang");
MODULE_LICENSE("GPL v2");

