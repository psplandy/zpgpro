/*
 * Samsung HDMI Physical interface driver
 *
 * Copyright (C) 2010-2011 Samsung Electronics Co.Ltd
 * Author: Tomasz Stanislawski <t.stanislaws@samsung.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/err.h>

#include <media/v4l2-subdev.h>

MODULE_AUTHOR("Tomasz Stanislawski <t.stanislaws@samsung.com>");
MODULE_DESCRIPTION("Samsung HDMI Physical interface driver");
MODULE_LICENSE("GPL");

struct hdmiphy_conf {
	unsigned long pixclk;
	const u8 *data;
};

struct hdmiphy_ctx {
	struct v4l2_subdev sd;
	const struct hdmiphy_conf *conf_tab;
};

static const struct hdmiphy_conf hdmiphy_conf_s5pv210[] = {
	{ .pixclk = 27000000, .data = (u8 [32]) {
		0x01, 0x05, 0x00, 0xD8, 0x10, 0x1C, 0x30, 0x40,
		0x6B, 0x10, 0x02, 0x52, 0xDF, 0xF2, 0x54, 0x87,
		0x84, 0x00, 0x30, 0x38, 0x00, 0x08, 0x10, 0xE0,
		0x22, 0x40, 0xE3, 0x26, 0x00, 0x00, 0x00, 0x00, }
	},
	{ .pixclk = 27027000, .data = (u8 [32]) {
		0x01, 0x05, 0x00, 0xD4, 0x10, 0x9C, 0x09, 0x64,
		0x6B, 0x10, 0x02, 0x52, 0xDF, 0xF2, 0x54, 0x87,
		0x84, 0x00, 0x30, 0x38, 0x00, 0x08, 0x10, 0xE0,
		0x22, 0x40, 0xE2, 0x26, 0x00, 0x00, 0x00, 0x00, }
	},
	{ .pixclk = 74176000, .data = (u8 [32]) {
		0x01, 0x05, 0x00, 0xD8, 0x10, 0x9C, 0xEF, 0x5B,
		0x6D, 0x10, 0x01, 0x52, 0xEF, 0xF3, 0x54, 0xB9,
		0x84, 0x00, 0x30, 0x38, 0x00, 0x08, 0x10, 0xE0,
		0x22, 0x40, 0xA5, 0x26, 0x01, 0x00, 0x00, 0x00, }
	},
	{ .pixclk = 74250000, .data = (u8 [32]) {
		0x01, 0x05, 0x00, 0xD8, 0x10, 0x9C, 0xF8, 0x40,
		0x6A, 0x10, 0x01, 0x52, 0xFF, 0xF1, 0x54, 0xBA,
		0x84, 0x00, 0x10, 0x38, 0x00, 0x08, 0x10, 0xE0,
		0x22, 0x40, 0xA4, 0x26, 0x01, 0x00, 0x00, 0x00, }
	},
	{ /* end marker */ }
};

static const struct hdmiphy_conf hdmiphy_conf_exynos4210[] = {
	{ .pixclk = 27000000, .data = (u8 [32]) {
		0x01, 0x05, 0x00, 0xD8, 0x10, 0x1C, 0x30, 0x40,
		0x6B, 0x10, 0x02, 0x51, 0xDF, 0xF2, 0x54, 0x87,
		0x84, 0x00, 0x30, 0x38, 0x00, 0x08, 0x10, 0xE0,
		0x22, 0x40, 0xE3, 0x26, 0x00, 0x00, 0x00, 0x00, }
	},
	{ .pixclk = 27027000, .data = (u8 [32]) {
		0x01, 0x05, 0x00, 0xD4, 0x10, 0x9C, 0x09, 0x64,
		0x6B, 0x10, 0x02, 0x51, 0xDF, 0xF2, 0x54, 0x87,
		0x84, 0x00, 0x30, 0x38, 0x00, 0x08, 0x10, 0xE0,
		0x22, 0x40, 0xE2, 0x26, 0x00, 0x00, 0x00, 0x00, }
	},
	{ .pixclk = 74176000, .data = (u8 [32]) {
		0x01, 0x05, 0x00, 0xD8, 0x10, 0x9C, 0xEF, 0x5B,
		0x6D, 0x10, 0x01, 0x51, 0xEF, 0xF3, 0x54, 0xB9,
		0x84, 0x00, 0x30, 0x38, 0x00, 0x08, 0x10, 0xE0,
		0x22, 0x40, 0xA5, 0x26, 0x01, 0x00, 0x00, 0x00, }
	},
	{ .pixclk = 74250000, .data = (u8 [32]) {
		0x01, 0x05, 0x00, 0xD8, 0x10, 0x9C, 0xF8, 0x40,
		0x6A, 0x10, 0x01, 0x51, 0xFF, 0xF1, 0x54, 0xBA,
		0x84, 0x00, 0x10, 0x38, 0x00, 0x08, 0x10, 0xE0,
		0x22, 0x40, 0xA4, 0x26, 0x01, 0x00, 0x00, 0x00, }
	},
	{ .pixclk = 148352000, .data = (u8 [32]) {
		0x01, 0x05, 0x00, 0xD8, 0x10, 0x9C, 0xEF, 0x5B,
		0x6D, 0x18, 0x00, 0x51, 0xEF, 0xF3, 0x54, 0xB9,
		0x84, 0x00, 0x30, 0x38, 0x00, 0x08, 0x10, 0xE0,
		0x11, 0x40, 0xA5, 0x26, 0x02, 0x00, 0x00, 0x00, }
	},
	{ .pixclk = 148500000, .data = (u8 [32]) {
		0x01, 0x05, 0x00, 0xD8, 0x10, 0x9C, 0xF8, 0x40,
		0x6A, 0x18, 0x00, 0x51, 0xFF, 0xF1, 0x54, 0xBA,
		0x84, 0x00, 0x10, 0x38, 0x00, 0x08, 0x10, 0xE0,
		0x11, 0x40, 0xA4, 0x26, 0x02, 0x00, 0x00, 0x00, }
	},
	{ /* end marker */ }
};

static const struct hdmiphy_conf hdmiphy_conf_exynos4212[] = {
	{ .pixclk = 27000000, .data = (u8 [32]) {
		0x01, 0x11, 0x2D, 0x75, 0x00, 0x01, 0x00, 0x08,
		0x82, 0x00, 0x0E, 0xD9, 0x45, 0xA0, 0x34, 0xC0,
		0x0B, 0x80, 0x12, 0x87, 0x08, 0x24, 0x24, 0x71,
		0x54, 0xE3, 0x24, 0x00, 0x00, 0x00, 0x01, 0x00, }
	},
	{ .pixclk = 27027000, .data = (u8 [32]) {
		0x01, 0x91, 0x2D, 0x72, 0x00, 0x64, 0x12, 0x08,
		0x43, 0x20, 0x0E, 0xD9, 0x45, 0xA0, 0x34, 0xC0,
		0x0B, 0x80, 0x12, 0x87, 0x08, 0x24, 0x24, 0x71,
		0x54, 0xE2, 0x24, 0x00, 0x00, 0x00, 0x01, 0x00, }
	},
	{ .pixclk = 74176000, .data = (u8 [32]) {
		0x01, 0x91, 0x3E, 0x35, 0x00, 0x5B, 0xDE, 0x08,
		0x82, 0x20, 0x73, 0xD9, 0x45, 0xA0, 0x34, 0xC0,
		0x0B, 0x80, 0x12, 0x87, 0x08, 0x24, 0x24, 0x52,
		0x54, 0xA5, 0x24, 0x01, 0x00, 0x00, 0x01, 0x00, }
	},
	{ .pixclk = 74250000, .data = (u8 [32]) {
		0x01, 0x91, 0x3E, 0x35, 0x00, 0x40, 0xF0, 0x08,
		0x82, 0x20, 0x73, 0xD9, 0x45, 0xA0, 0x34, 0xC0,
		0x0B, 0x80, 0x12, 0x87, 0x08, 0x24, 0x24, 0x52,
		0x54, 0xA4, 0x24, 0x01, 0x00, 0x00, 0x01, 0x00, }
	},
	{ .pixclk = 148500000, .data = (u8 [32]) {
		0x01, 0x91, 0x3E, 0x15, 0x00, 0x40, 0xF0, 0x08,
		0x82, 0x20, 0x73, 0xD9, 0x45, 0xA0, 0x34, 0xC0,
		0x0B, 0x80, 0x12, 0x87, 0x08, 0x24, 0x24, 0xA4,
		0x54, 0x4A, 0x25, 0x03, 0x00, 0x00, 0x01, 0x00, }
	},
	{ /* end marker */ }
};

static const struct hdmiphy_conf hdmiphy_conf_exynos4412[] = {
	{ .pixclk = 27000000, .data = (u8 [32]) {
		0x01, 0x11, 0x2D, 0x75, 0x40, 0x01, 0x00, 0x08,
		0x82, 0x00, 0x0E, 0xD9, 0x45, 0xA0, 0xAC, 0x80,
		0x08, 0x80, 0x11, 0x84, 0x02, 0x22, 0x44, 0x86,
		0x54, 0xE4, 0x24, 0x00, 0x00, 0x00, 0x01, 0x00, }
	},
	{ .pixclk = 27027000, .data = (u8 [32]) {
		0x01, 0x91, 0x2D, 0x72, 0x40, 0x64, 0x12, 0x08,
		0x43, 0x20, 0x0E, 0xD9, 0x45, 0xA0, 0xAC, 0x80,
		0x08, 0x80, 0x11, 0x84, 0x02, 0x22, 0x44, 0x86,
		0x54, 0xE3, 0x24, 0x00, 0x00, 0x00, 0x01, 0x00, }
	},
	{ .pixclk = 74176000, .data = (u8 [32]) {
		0x01, 0x91, 0x1F, 0x10, 0x40, 0x5B, 0xEF, 0x08,
		0x81, 0x20, 0xB9, 0xD8, 0x45, 0xA0, 0xAC, 0x80,
		0x08, 0x80, 0x11, 0x84, 0x02, 0x22, 0x44, 0x86,
		0x54, 0xA6, 0x24, 0x01, 0x00, 0x00, 0x01, 0x00, }
	},
	{ .pixclk = 74250000, .data = (u8 [32]) {
		0x01, 0x91, 0x1F, 0x10, 0x40, 0x40, 0xF8, 0x08,
		0x81, 0x20, 0xBA, 0xD8, 0x45, 0xA0, 0xAC, 0x80,
		0x08, 0x80, 0x11, 0x84, 0x02, 0x22, 0x44, 0x86,
		0x54, 0xA5, 0x24, 0x01, 0x00, 0x00, 0x01, 0x00, }
	},
	{ .pixclk = 148500000, .data = (u8 [32]) {
		0x01, 0x91, 0x1F, 0x00, 0x40, 0x40, 0xF8, 0x08,
		0x81, 0x20, 0xBA, 0xD8, 0x45, 0xA0, 0xAC, 0x80,
		0x08, 0x80, 0x11, 0x84, 0x02, 0x22, 0x44, 0x86,
		0x54, 0x4B, 0x25, 0x03, 0x00, 0x00, 0x01, 0x00, }
	},
	{ /* end marker */ }
};

static inline struct hdmiphy_ctx *sd_to_ctx(struct v4l2_subdev *sd)
{
	return container_of(sd, struct hdmiphy_ctx, sd);
}

static const u8 *hdmiphy_find_conf(unsigned long pixclk,
		const struct hdmiphy_conf *conf)
{
	for (; conf->pixclk; ++conf)
		if (conf->pixclk == pixclk)
			return conf->data;
	return NULL;
}

static int hdmiphy_s_power(struct v4l2_subdev *sd, int on)
{
	/* to be implemented */
	return 0;
}

static int hdmiphy_s_dv_timings(struct v4l2_subdev *sd,
	struct v4l2_dv_timings *timings)
{
	const u8 *data;
	u8 buffer[32];
	int ret;
	struct hdmiphy_ctx *ctx = sd_to_ctx(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct device *dev = &client->dev;
	unsigned long pixclk = timings->bt.pixelclock;

	dev_info(dev, "s_dv_timings\n");
	if ((timings->bt.flags & V4L2_DV_FL_REDUCED_FPS) && pixclk == 74250000)
		pixclk = 74176000;
	data = hdmiphy_find_conf(pixclk, ctx->conf_tab);
	if (!data) {
		dev_err(dev, "format not supported\n");
		return -EINVAL;
	}

	/* storing configuration to the device */
	memcpy(buffer, data, 32);
	ret = i2c_master_send(client, buffer, 32);
	if (ret != 32) {
		dev_err(dev, "failed to configure HDMIPHY via I2C\n");
		return -EIO;
	}

	return 0;
}

static int hdmiphy_dv_timings_cap(struct v4l2_subdev *sd,
	struct v4l2_dv_timings_cap *cap)
{
	if (cap->pad != 0)
		return -EINVAL;

	cap->type = V4L2_DV_BT_656_1120;
	/* The phy only determines the pixelclock, leave the other values
	 * at 0 to signify that we have no information for them. */
	cap->bt.min_pixelclock = 27000000;
	cap->bt.max_pixelclock = 148500000;
	return 0;
}

static int hdmiphy_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct device *dev = &client->dev;
	u8 buffer[2];
	int ret;

	dev_info(dev, "s_stream(%d)\n", enable);
	/* going to/from configuration from/to operation mode */
	buffer[0] = 0x1f;
	buffer[1] = enable ? 0x80 : 0x00;

	ret = i2c_master_send(client, buffer, 2);
	if (ret != 2) {
		dev_err(dev, "stream (%d) failed\n", enable);
		return -EIO;
	}
	return 0;
}

static const struct v4l2_subdev_core_ops hdmiphy_core_ops = {
	.s_power =  hdmiphy_s_power,
};

static const struct v4l2_subdev_video_ops hdmiphy_video_ops = {
	.s_dv_timings = hdmiphy_s_dv_timings,
	.s_stream =  hdmiphy_s_stream,
};

static const struct v4l2_subdev_pad_ops hdmiphy_pad_ops = {
	.dv_timings_cap = hdmiphy_dv_timings_cap,
};

static const struct v4l2_subdev_ops hdmiphy_ops = {
	.core = &hdmiphy_core_ops,
	.video = &hdmiphy_video_ops,
	.pad = &hdmiphy_pad_ops,
};

static int hdmiphy_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct hdmiphy_ctx *ctx;

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	ctx->conf_tab = (struct hdmiphy_conf *)id->driver_data;
	v4l2_i2c_subdev_init(&ctx->sd, client, &hdmiphy_ops);

	dev_info(&client->dev, "probe successful\n");
	return 0;
}

static int hdmiphy_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct hdmiphy_ctx *ctx = sd_to_ctx(sd);

	kfree(ctx);
	dev_info(&client->dev, "remove successful\n");

	return 0;
}

static const struct i2c_device_id hdmiphy_id[] = {
	{ "hdmiphy", (unsigned long)hdmiphy_conf_exynos4210 },
	{ "hdmiphy-s5pv210", (unsigned long)hdmiphy_conf_s5pv210 },
	{ "hdmiphy-exynos4210", (unsigned long)hdmiphy_conf_exynos4210 },
	{ "hdmiphy-exynos4212", (unsigned long)hdmiphy_conf_exynos4212 },
	{ "hdmiphy-exynos4412", (unsigned long)hdmiphy_conf_exynos4412 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, hdmiphy_id);

static struct i2c_driver hdmiphy_driver = {
	.driver = {
		.name	= "s5p-hdmiphy",
	},
	.probe		= hdmiphy_probe,
	.remove		= hdmiphy_remove,
	.id_table = hdmiphy_id,
};

module_i2c_driver(hdmiphy_driver);
