/***************************************************************
** Copyright (C),  2020,  OPLUS Mobile Comm Corp.,  Ltd
** VENDOR_EDIT
** File : oplus_onscreenfingerprint.h
** Description : oplus onscreenfingerprint feature
** Version : 1.0
** Date : 2020/04/15
** Author : Qianxu@MM.Display.LCD Driver
**
** ------------------------------- Revision History: -----------
**  <author>        <data>        <version >        <desc>
**   Qianxu         2020/04/15        1.0           Build this moudle
******************************************************************/
#ifndef _OPLUS_ONSCREENFINGERPRINT_H_
#define _OPLUS_ONSCREENFINGERPRINT_H_

#include <drm/drm_crtc.h>
#include "dsi_panel.h"
#include "dsi_defs.h"
#include "dsi_parser.h"
#include "sde_encoder_phys.h"


#define FFL_FP_LEVEL 150


int oplus_get_panel_brightness(void);

int dsi_panel_parse_oplus_fod_config(struct dsi_panel *panel);

int dsi_panel_parse_oplus_config(struct dsi_panel *panel);

int dsi_panel_parse_oplus_mode_config(struct dsi_display_mode *mode,
				     struct dsi_parser_utils *utils);

bool sde_crtc_get_dimlayer_mode(struct drm_crtc_state *crtc_state);

bool sde_crtc_get_fingerprint_mode(struct drm_crtc_state *crtc_state);

bool sde_crtc_get_fingerprint_pressed(struct drm_crtc_state *crtc_state);

int sde_crtc_set_onscreenfinger_defer_sync(struct drm_crtc_state *crtc_state,
		bool defer_sync);

int sde_crtc_config_fingerprint_dim_layer(struct drm_crtc_state *crtc_state,
		int stage);

bool _sde_encoder_setup_dither_for_onscreenfingerprint(struct sde_encoder_phys *phys,
		void *dither_cfg, int len, struct sde_hw_pingpong *hw_pp);

int sde_plane_check_fingerprint_layer(const struct drm_plane_state *drm_state);

#endif /*_OPLUS_ONSCREENFINGERPRINT_H_*/
