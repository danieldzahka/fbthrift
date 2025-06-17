#ifndef PSP_YNL_H
#define PSP_YNL_H

#ifdef __cplusplus

#include <cstdint>
#include <memory>
extern "C" {
#else

#include <stdint.h>
#endif

typedef uint8_t u8;
typedef uint32_t __be32;

#define PSP_V0_KEY 16
#define PSP_V1_KEY 32
#define PSP_MAX_KEY 32

enum {
	PSP_V0 = 0,
	PSP_V1,
	PSP_V2,
	PSP_V3,
};

struct psp_key_parsed {
	__be32 spi;
	u8 key[PSP_MAX_KEY];
};

struct psp_ynl;

struct psp_ynl *psp_ynl_new();
void psp_ynl_free(struct psp_ynl *pynl);
int psp_ynl_rx_spi_alloc(struct psp_ynl *pynl, int sock, u8 version,
			 struct psp_key_parsed *spi_key);
int psp_ynl_tx_spi_set(struct psp_ynl *pynl, int sock, u8 version,
		       struct psp_key_parsed *spi_key);

#ifdef __cplusplus
}

using PSPYnl = std::unique_ptr<struct psp_ynl, decltype(&psp_ynl_free)>;
inline std::unique_ptr<struct psp_ynl, decltype(&psp_ynl_free)> psp_ynl_create()
{
	return { psp_ynl_new(), psp_ynl_free };
}

#endif
#endif /* PSP_YNL_H */
