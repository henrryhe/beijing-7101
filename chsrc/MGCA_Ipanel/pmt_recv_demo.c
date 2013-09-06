
/*recv pmt demo*/

static int pmt_filter_index = -1;
static unsigned char pmt_version = 0xff, pmt_recv_flag = 0;
/*the function must be non blocking*/
static INT32_T pmt_recv_notify(VOID* tag, BYTE_T* data, INT32_T len)
{
	if (tag && data && (tag == (void*)1000) && (data[0] == 0x02) && (len > 8)) {
		if (!pmt_recv_flag) {
			//recv new pmt section
			//
			pmt_version = data[5] & 0x3E;
			pmt_recv_flag = 1;
		}
		else if (pmt_version != (data[5] & 0x3E)) {
			//update new pmt section
			//
			pmt_version = data[5] & 0x3E;
		}
	}
	return 0;
}

static void add_pmt_filter(unsigned short srv_id, unsigned short pid)
{
	INT32_T nRet = -1;
	IPANEL_FILTER_INFO info[1] = {0};
	info->pid = pid;
	info->coef[0] = 0x02;
	info->coef[3] = (srv_id >> 8) & 0xff;
	info->coef[4] = srv_id & 0xff;
	info->mask[0] = 0xff;
	info->mask[3] = 0xff;
	info->mask[4] = 0xff;
	info->depth = 5;
	info->func = pmt_recv_notify;
	info->tag = (void*)1000;
	if (pmt_filter_index == -1) {
		pmt_filter_index = ipanel_add_filter(info);
	}
	else if (pmt_filter_index >= 0) {
		nRet = ipanel_modify_filter(pmt_filter_index, info);
	}
}

static void remove_pmt_filter(void)
{
	INT32_T nRet = -1;
	if (pmt_filter_index >= 0) {
		nRet = ipanel_remove_filter(pmt_filter_index);
		if (nRet >= 0) {
			pmt_filter_index = -1;
			pmt_recv_flag = 0;
		}
	}
}

INT32_T ipanel_cam_srv_ctrl(IPANEL_CAM_ECM_CTRL_e act, IPANEL_CAM_SRV_INFO *psrv);
{
	if (act == IPANEL_CAM_ADD_SRV) {
		if (psrv) {
			add_pmt_filter(psrv->service_id, psrv->pmt_pid);
		}
	}
	else if (act == IPANEL_CAM_DEL_SRV) {
		remove_pmt_filter();
	}
	...
	return 0;
}
