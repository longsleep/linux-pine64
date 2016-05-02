/*
 * Copyright (C) 2015 Allwinnertech, z.q <zengqi@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
 
#include "disp_private.h"
#ifdef SUPPORT_EINK
#include"disp_eink.h"
#include"include.h"


extern struct format_manager *disp_get_format_manager(unsigned int id);
/*
static s32 clean_ring_queue(struct eink_buffer_manager* buffer_mgr)
{
	int i;
	unsigned int buf_size;

	buf_size = buffer_mgr->buf_size;

	for (i = 0; i < IMAGE_BUF_NUM; i++)
		disp_free((void*)buffer_mgr->image_slot[i].vaddr, (void*)buffer_mgr->image_slot[i].paddr, buf_size);

	return 0;
}*/


bool is_ring_queue_full(struct eink_buffer_manager* buffer_mgr)
{
	bool ret;
	unsigned int in_index, out_index;

	mutex_lock(&buffer_mgr->mlock);

	in_index = buffer_mgr->in_index;
	out_index = buffer_mgr->out_index;

	ret = ((in_index + 1)%IMAGE_BUF_NUM == out_index) ? true : false;

	mutex_unlock(&buffer_mgr->mlock);

	return ret;
}

bool is_ring_queue_empty(struct eink_buffer_manager* buffer_mgr)
{
	bool ret;
	unsigned int in_index, out_index;
	mutex_lock(&buffer_mgr->mlock);

	in_index = buffer_mgr->in_index;
	out_index = buffer_mgr->out_index;

	ret =  (in_index == (out_index+1) % IMAGE_BUF_NUM) ? true: false;

	mutex_unlock(&buffer_mgr->mlock);

	return ret;
}

struct eink_8bpp_image* get_current_image(struct eink_buffer_manager* buffer_mgr)
{
	struct eink_8bpp_image* ret;
	unsigned int out_index;

	mutex_lock(&buffer_mgr->mlock);

	out_index = buffer_mgr->out_index;

	if (out_index < IMAGE_BUF_NUM) {
		ret = &buffer_mgr->image_slot[(out_index + 1)%IMAGE_BUF_NUM];
		goto out;
	}
	else  {
		__wrn("%s: in_index larger than the max value!\n",__func__);
		ret = NULL;
		goto out;
	}

out:

	mutex_unlock(&buffer_mgr->mlock);

	return ret;
}

struct eink_8bpp_image* get_last_image(struct eink_buffer_manager* buffer_mgr)
{
	struct eink_8bpp_image* ret;
	unsigned int out_index;

	mutex_lock(&buffer_mgr->mlock);

	out_index = buffer_mgr->out_index;

	if (out_index < IMAGE_BUF_NUM) {
		ret = &buffer_mgr->image_slot[out_index];
		goto out;
	}
	else  {
		__wrn("%s: in_index larger than the max value!\n",__func__);
		ret = NULL;
		goto out;
	}

out:
	mutex_unlock(&buffer_mgr->mlock);

	return ret;
}

s32 queue_image(struct eink_buffer_manager* buffer_mgr, void * src_image, u32 mode,  struct area_info update_area)
{
	bool queue_is_full;
	int ret = 0;
	unsigned int in_index, out_index;
	unsigned int buf_size;
#ifdef SUPPORT_WB
	struct format_manager * cmgr;
	struct image_format src, dest;
#endif
	mutex_lock(&buffer_mgr->mlock);

	in_index = buffer_mgr->in_index;
	out_index = buffer_mgr->out_index;
	buf_size = buffer_mgr->buf_size;

	if(in_index >= IMAGE_BUF_NUM) {
		__wrn("in_index larger than the max value!\n");
		ret = -EINVAL;
		goto out;
	}

	queue_is_full = ((in_index + 1)%IMAGE_BUF_NUM == out_index) ? true : false;
	if ((!queue_is_full) && buffer_mgr->image_slot[in_index].state == USED) {
		/*do nothing*/
	}

#ifdef SUPPORT_WB
	cmgr = disp_get_format_manager(0);

	memset((void*)&src, 0, sizeof(struct image_format));
	src.format = DISP_FORMAT_ARGB_8888;
	src.addr1 = (unsigned int)src_image;
	src.width = buffer_mgr->width;
	src.height = buffer_mgr->height;

	memset((void*)&dest, 0, sizeof(struct image_format));
	dest.format = DISP_FORMAT_8BIT_GRAY;
	dest.addr1 = buffer_mgr->image_slot[in_index].paddr;

	if (NULL != src_image)
		cmgr->start_convert(0,&src, &dest);

#else
	if (NULL != src_image)
		memcpy((void*)buffer_mgr->image_slot[in_index].vaddr, (void*)src_image, buf_size);
#endif

	/*
	* update new 8bpp image information
	*/

	buffer_mgr->image_slot[in_index].state = USED;
	buffer_mgr->image_slot[in_index].update_mode = mode;
	buffer_mgr->image_slot[in_index].window_calc_enable = false;

	__debug("mode=0x%x, x-top=%d, y-top=%d, x-bot=%d, y-bot=%d\n", \
		mode, update_area.x_top, update_area.y_top, update_area.x_bottom, update_area.y_bottom);

	if (mode & EINK_RECT_MODE) {

		buffer_mgr->image_slot[in_index].flash_mode = LOCAL;

		if ((update_area.x_bottom== 0) && (update_area.x_top == 0) &&
				(update_area.y_bottom== 0) && (update_area.y_top == 0))
		{
			buffer_mgr->image_slot[in_index].window_calc_enable = true;
			memset(&buffer_mgr->image_slot[in_index].update_area, 0, sizeof(struct area_info));
		} else {
			memcpy(&buffer_mgr->image_slot[in_index].update_area, &update_area, sizeof(struct area_info));
		}
	}
	else {

		if (mode == EINK_INIT_MODE)
			buffer_mgr->image_slot[in_index].flash_mode = INIT;
		else
			buffer_mgr->image_slot[in_index].flash_mode = GLOBAL;

		/*set update area full screen.*/
		buffer_mgr->image_slot[in_index].update_area.x_top = 0;
		buffer_mgr->image_slot[in_index].update_area.y_top = 0;
		buffer_mgr->image_slot[in_index].update_area.x_bottom= buffer_mgr->image_slot[in_index].size.width-1;
		buffer_mgr->image_slot[in_index].update_area.y_bottom= buffer_mgr->image_slot[in_index].size.height-1;
	}

	/* if queue is full,then cover the newest image,and in_index keep the same value.*/
	if (!queue_is_full)
		buffer_mgr->in_index = (buffer_mgr->in_index+1)%IMAGE_BUF_NUM;


out:
	/*__debug("in_index:%d, out_index:%d,paddr:0x%x, calc_en=%d\n",\
		buffer_mgr->in_index, buffer_mgr->out_index, buffer_mgr->image_slot[in_index].paddr, buffer_mgr->image_slot[in_index].window_calc_enable);
	*/
	mutex_unlock(&buffer_mgr->mlock);

	return ret;
}

s32 dequeue_image(struct eink_buffer_manager* buffer_mgr)
{
	bool queue_is_empty;
	int ret = 0;
	unsigned int in_index, out_index;

	mutex_lock(&buffer_mgr->mlock);
	in_index = buffer_mgr->in_index;
	out_index = buffer_mgr->out_index;

	if(out_index >= IMAGE_BUF_NUM) {
		__wrn("%s: out_index larger than the max value!\n",__func__);
		ret = -EINVAL;
		goto out;
	}

	queue_is_empty = (in_index == (out_index+1) % IMAGE_BUF_NUM) ? true : false;
	if (queue_is_empty) {
		ret = -EBUSY;
		__debug("queue is empty!\n");
		goto out;
	}

	/*set the 8pp image information to initial state.*/
	buffer_mgr->image_slot[out_index].state = FREE;
	buffer_mgr->image_slot[out_index].flash_mode = GLOBAL;
	buffer_mgr->image_slot[out_index].update_mode = EINK_INIT_MODE;
	buffer_mgr->image_slot[out_index].window_calc_enable = true;
	buffer_mgr->out_index = (buffer_mgr->out_index+1)%IMAGE_BUF_NUM;


	//__debug("in_index:%d, out_index:%d\n",buffer_mgr->in_index, buffer_mgr->out_index);
out:
	mutex_unlock(&buffer_mgr->mlock);

	return ret;
}

s32 ring_buffer_manager_init(struct disp_eink_manager* eink_manager)
{
	int i;
	int ret = 0;
	struct eink_buffer_manager* buffer_mgr = NULL;

	buffer_mgr = (struct eink_buffer_manager*) disp_sys_malloc(sizeof(struct eink_buffer_manager));
	if (!buffer_mgr) {
		__wrn("malloc eink buffer manager memory fail!\n");
		ret = -ENOMEM;
		goto buffer_mgr_err;
	}

	/*init is 1,ring buffer is empty. out_index is 0,point last image.*/
	memset((void*)(buffer_mgr), 0, sizeof(struct eink_buffer_manager));
	buffer_mgr->width = eink_manager->private_data->param.timing.width;
	buffer_mgr->height = eink_manager->private_data->param.timing.height;
	buffer_mgr->buf_size = buffer_mgr->width * buffer_mgr->height;
	buffer_mgr->in_index = 1;
	buffer_mgr->out_index = 0;
	mutex_init(&buffer_mgr->mlock);

	for (i = 0; i < IMAGE_BUF_NUM; i++) {
		buffer_mgr->image_slot[i].size.width = buffer_mgr->width;
		buffer_mgr->image_slot[i].size.height = buffer_mgr->height;
		buffer_mgr->image_slot[i].size.align = 4; //fix it
		buffer_mgr->image_slot[i].update_mode = EINK_INIT_MODE;
		buffer_mgr->image_slot[i].vaddr = (void*)disp_malloc(buffer_mgr->buf_size, &buffer_mgr->image_slot[i].paddr);
		if (!buffer_mgr->image_slot[i].vaddr) {
			__wrn("malloc image buffer memory fail!\n");
			ret = -ENOMEM;
			goto image_buffer_err;
		}
		__debug("image paddr%d=0x%x" ,i, buffer_mgr->image_slot[i].paddr);

		/*init mode need buf data 0xff*/

		memset((void*)buffer_mgr->image_slot[i].vaddr, 0xff, buffer_mgr->buf_size);
	}

	buffer_mgr->is_full = is_ring_queue_full;
	buffer_mgr->is_empty = is_ring_queue_empty;
	buffer_mgr->get_last_image = get_last_image;
	buffer_mgr->get_current_image = get_current_image;
	buffer_mgr->queue_image = queue_image;
	buffer_mgr->dequeue_image = dequeue_image;

	eink_manager->buffer_mgr = buffer_mgr;

	return ret;

image_buffer_err:
	for (i = 0; i < IMAGE_BUF_NUM; i++) {
		if (buffer_mgr->image_slot[i].vaddr)
			disp_free(buffer_mgr->image_slot[i].vaddr, (void*) buffer_mgr->image_slot[i].paddr,
				buffer_mgr->buf_size);
	}

buffer_mgr_err:
	disp_sys_free(buffer_mgr);

	return ret;

}
#endif
