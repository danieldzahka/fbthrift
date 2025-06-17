#include "psp_ynl.h"

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <ynl.h>
#include <psp-user.h>

typedef uint32_t u32;

struct psp_ynl {
	struct ynl_sock *ys;
	int psp_dev_id;
	u32 restore_ver_ena;
};

static int psp_dev_set_ena(struct ynl_sock *ys, u32 dev_id, u32 versions)
{
	struct psp_dev_set_req *sreq;
	struct psp_dev_set_rsp *srsp;

	sreq = psp_dev_set_req_alloc();

	psp_dev_set_req_set_id(sreq, dev_id);
	psp_dev_set_req_set_psp_versions_ena(sreq, versions);

	srsp = psp_dev_set(ys, sreq);
	psp_dev_set_req_free(sreq);
	if (!srsp)
		return -1;

	psp_dev_set_rsp_free(srsp);
	return 0;
}

static int psp_ynl_init(bool enable_psp, struct psp_ynl *pynl)
{
	struct psp_dev_get_list *dev_list;
	struct ynl_error yerr;
	struct ynl_sock *ys;
	int first_id = 0;
	u32 ver_ena, ver_cap, ver_want;

	ys = ynl_sock_create(&ynl_psp_family, &yerr);
	if (!ys) {
		fprintf(stderr, "YNL: %s\n", yerr.msg);
		return -1;
	}

	dev_list = psp_dev_get_dump(ys);
	if (ynl_dump_empty(dev_list)) {
		if (ys->err.code)
			goto err_close;
		printf("No PSP devices\n");
		return -1;
	}

	ynl_dump_foreach(dev_list, d)
	{
		if (!first_id) {
			first_id = d->id;
			ver_ena = d->psp_versions_ena;
			ver_cap = d->psp_versions_cap;
		} else {
			fprintf(stderr, "Multiple PSP devices found\n");
			goto err_close_silent;
		}
	}
	psp_dev_get_list_free(dev_list);

	ver_want = enable_psp ? ver_cap : 0;
	if (ver_ena != ver_want) {
		if (psp_dev_set_ena(ys, first_id, ver_want))
			goto err_close;
	}

	pynl->ys = ys;
	pynl->psp_dev_id = first_id;
	pynl->restore_ver_ena = ver_ena;

	return 0;

err_close:
	fprintf(stderr, "YNL: %s\n", ys->err.msg);
err_close_silent:
	ynl_sock_destroy(ys);
	return -1;
}

static void psp_ynl_uninit(struct psp_ynl *pynl)
{
	ynl_sock_destroy(pynl->ys);
}

struct psp_ynl *psp_ynl_new()
{
	struct psp_ynl *pynl = calloc(1, sizeof(*pynl));
	int rc;

	rc = psp_ynl_init(true, pynl);
	if (rc) {
		free(pynl);
		return NULL;
	}

	return pynl;
}

void psp_ynl_free(struct psp_ynl *pynl)
{
	if (!pynl)
		return;

	psp_ynl_uninit(pynl);
	free(pynl);
}

int psp_ynl_rx_spi_alloc(struct psp_ynl *pynl, int sock, u8 version,
			 struct psp_key_parsed *spi_key)
{
	struct psp_rx_assoc_rsp *rsp;
	struct psp_rx_assoc_req *req;

	req = psp_rx_assoc_req_alloc();
	psp_rx_assoc_req_set_dev_id(req, pynl->psp_dev_id);
	psp_rx_assoc_req_set_sock_fd(req, sock);
	psp_rx_assoc_req_set_version(req, version);

	rsp = psp_rx_assoc(pynl->ys, req);
	psp_rx_assoc_req_free(req);

	if (!rsp)
		return -1;

	spi_key->spi = rsp->rx_key.spi;
	memcpy(spi_key->key, rsp->rx_key.key, rsp->rx_key._len.key);

	psp_rx_assoc_rsp_free(rsp);
	return 0;
}

int psp_ynl_tx_spi_set(struct psp_ynl *pynl, int sock, u8 version,
		       struct psp_key_parsed *spi_key)
{
	struct psp_tx_assoc_rsp *tsp;
	struct psp_tx_assoc_req *teq;
	int key_len;

	key_len = (version == PSP_V0 || version == PSP_V2) ? PSP_V0_KEY :
							     PSP_V1_KEY;
	teq = psp_tx_assoc_req_alloc();

	psp_tx_assoc_req_set_dev_id(teq, pynl->psp_dev_id);
	psp_tx_assoc_req_set_sock_fd(teq, sock);
	psp_tx_assoc_req_set_version(teq, version);
	psp_tx_assoc_req_set_tx_key_spi(teq, spi_key->spi);
	psp_tx_assoc_req_set_tx_key_key(teq, spi_key->key, key_len);

	tsp = psp_tx_assoc(pynl->ys, teq);
	psp_tx_assoc_req_free(teq);
	if (!tsp) {
		perror("ERROR: failed to Tx assoc");
		return -1;
	}
	psp_tx_assoc_rsp_free(tsp);

	return 0;
}
