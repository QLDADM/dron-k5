/* RID-K5: łącze z komputerem (opis w link.h). */
#include <string.h>

#include "app/link.h"
#include "app/scan.h"
#include "app/store.h"
#include "bsp/dp32g030/dma.h"
#include "driver/uart.h"
#include "external/printf/printf.h"

static uint16_t readIndex;
static char     cmd[32];
static uint8_t  cmdLen;

static void SendLine(const char *line)
{
	UART_Send(line, strlen(line));
}

void LINK_ReportWindow(void)
{
	const ScanBand_t      *b = &SCAN_BANDS[gScanLastBand];
	const ScanBandState_t *s = &gScanState[gScanLastBand];
	if (s->windows < 2)
		return;  // poziom szumu jeszcze się uczy
	char line[40];
	sprintf(line, "DD,%s,%u,%u,%d,%u\r\n", b->name, s->score, s->distinct, SCAN_RssiToDbm(s->peak), s->alarm);
	SendLine(line);
}

static void DumpLog(void)
{
	char line[56];
	StoreEvent_t e;
	for (uint8_t i = 0; STORE_GetEvent(i, &e); i++) {
		sprintf(line, "DL,%u,%s,%d,%u,%lu\r\n", i, e.band < SCAN_BAND_COUNT ? SCAN_BANDS[e.band].name : "?",
		        e.peakDbm, e.durationS, (unsigned long)e.time);
		SendLine(line);
	}
	SendLine("DL,END\r\n");
}

static void HandleCommand(void)
{
	cmd[cmdLen] = 0;
	if (!strncmp(cmd, "TIME ", 5)) {
		uint32_t t = 0;
		for (const char *p = cmd + 5; *p >= '0' && *p <= '9'; p++)
			t = t * 10 + (*p - '0');
		if (t > 1700000000u)
			gUnixBase = t - gUptimeS;
	} else if (!strcmp(cmd, "LOG")) {
		DumpLog();
	} else if (!strcmp(cmd, "ID")) {
		SendLine("DI,RID-K5," VERSION_STRING "\r\n");
	}
}

void LINK_Poll(void)
{
	const uint16_t dmaIndex = DMA_CH0->ST & 0xFFFU;
	while (readIndex != dmaIndex) {
		const uint8_t c = UART_DMA_Buffer[readIndex];
		readIndex = (readIndex + 1) % sizeof(UART_DMA_Buffer);
		if (c == '\n' || c == '\r') {
			if (cmdLen)
				HandleCommand();
			cmdLen = 0;
		} else if (c >= ' ' && c < 127 && cmdLen < sizeof(cmd) - 1) {
			cmd[cmdLen++] = c;
		} else {
			cmdLen = 0;  // dane binarne (np. zapytanie CHIRP) - ignorujemy
		}
	}
}
