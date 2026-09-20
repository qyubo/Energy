#include "bluetooth.h"
#include "energy.h"
#include "usart.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BT_MAX_LINE 1024U
#define BT_WATCHDOG_MS 6000U

static uint8_t rx_byte;
static char assembling[BT_MAX_LINE + 1U];
static char ready_line[BT_MAX_LINE + 1U];
static volatile uint8_t line_ready;
static uint16_t line_length;
static uint8_t dropping;
static char session_id[49];
static uint32_t last_seq;
static uint32_t last_seen_ms;
static char last_request[BT_MAX_LINE + 1U];
static char last_reply[384];
static char cached_reply[384];
static uint8_t reply_pending;
static uint32_t pending_seq;
static uint8_t last_reply_valid;

/* A bounded extractor for the flat ASCII JSON commands sent by the phone. */
static const char *Bluetooth_Field(const char *line, const char *name) {
    char key[32];
    (void)snprintf(key, sizeof(key), "\"%s\"", name);
    const char *p = strstr(line, key);
    if (p == NULL) return NULL;
    p += strlen(key);
    while (*p == ' ') p++;
    if (*p++ != ':') return NULL;
    while (*p == ' ') p++;
    return p;
}

static uint8_t Bluetooth_String(const char *line, const char *name, char *out, size_t capacity) {
    const char *p = Bluetooth_Field(line, name);
    if ((p == NULL) || (*p++ != '"')) return 0U;
    size_t n = 0U;
    while ((*p != '"') && (*p != '\0') && (n + 1U < capacity)) {
        if (((unsigned char)*p < 32U) || (*p == '\\')) return 0U;
        out[n++] = *p++;
    }
    if ((*p != '"') || (n == 0U)) return 0U;
    out[n] = '\0';
    return 1U;
}

static uint8_t Bluetooth_Number(const char *line, const char *name, uint32_t *out) {
    const char *p = Bluetooth_Field(line, name);
    if ((p == NULL) || (*p < '0') || (*p > '9')) return 0U;
    char *end;
    unsigned long value = strtoul(p, &end, 10);
    if ((*end != ',') && (*end != '}') && (*end != ' ')) return 0U;
    if (value > 2147483647UL) return 0U;
    *out = (uint32_t)value;
    return 1U;
}

static void Bluetooth_SendReply(uint32_t seq, uint8_t ok, const char *error) {
    EnergyState_t s = Energy_GetState();
    const char *mode = s.mode == ENERGY_MODE_SMALL_TEST ? "SMALL" :
                       s.mode == ENERGY_MODE_BIG_TEST ? "LARGE" : "NONE";
    const char *phases[] = {"IDLE", "RUNNING", "FLASHING", "COMPLETE"};
    int n = snprintf(last_reply, sizeof(last_reply),
        "{\"v\":1,\"sid\":\"%s\",\"seq\":%lu,\"ok\":%s%s%s%s,"
        "\"state\":{\"rev\":%lu,\"round\":%lu,\"mode\":\"%s\","
        "\"phase\":\"%s\",\"mask\":%u,\"target\":%u,\"flashOn\":%s,\"flashes\":%u,"
        "\"progress\":%u,\"otherTarget\":%u}}\n",
        session_id, (unsigned long)seq, ok ? "true" : "false",
        ok ? "" : ",\"error\":\"", ok ? "" : error, ok ? "" : "\"",
        (unsigned long)s.rev, (unsigned long)s.round, mode, phases[s.phase],
        s.mask, s.target, s.flash_on ? "true" : "false", s.flashes,
        s.progress, s.other_target);
    if ((n > 0) && ((size_t)n < sizeof(last_reply))) {
        last_reply_valid = 1U;
        (void)HAL_UART_Transmit(&huart1, (uint8_t *)last_reply, (uint16_t)n, 100U);
    }
}

static void Bluetooth_HandleLine(const char *line) {
    char sid[49], cmd[16], mode[12];
    uint32_t version, seq, rev, target;
    if (!Bluetooth_Number(line, "v", &version) || (version != 1U) ||
        !Bluetooth_String(line, "sid", sid, sizeof(sid)) ||
        (strlen(sid) < 8U) || !Bluetooth_Number(line, "seq", &seq) || (seq == 0U) ||
        !Bluetooth_String(line, "cmd", cmd, sizeof(cmd))) return;
    for (const char *p = sid; *p != '\0'; p++) {
        if (!(('a' <= *p && *p <= 'z') || ('A' <= *p && *p <= 'Z') ||
              ('0' <= *p && *p <= '9') || (*p == '_') || (*p == '-'))) return;
    }

    if ((strcmp(cmd, "HELLO") == 0) && (strcmp(sid, session_id) != 0)) {
        /* A reconnect supersedes any unconfirmed reply from the old session. */
        reply_pending = 0U;
        strcpy(session_id, sid);
        last_seq = 0U;
        last_reply_valid = 0U;
        Energy_SetMode(ENERGY_MODE_OFF);
    }
    if (strcmp(sid, session_id) != 0) {
        /* Echo the rejected session so the phone can associate the error. */
        char old_sid[49];
        strcpy(old_sid, session_id);
        strcpy(session_id, sid);
        Bluetooth_SendReply(seq, 0U, "SESSION");
        strcpy(session_id, old_sid);
        return;
    }
    if (seq == last_seq) {
        if (last_reply_valid && strcmp(line, last_request) == 0)
            (void)HAL_UART_Transmit(&huart1, (uint8_t *)cached_reply, (uint16_t)strlen(cached_reply), 100U);
        else Bluetooth_SendReply(seq, 0U, "SEQ_CONFLICT");
        last_seen_ms = HAL_GetTick();
        return;
    }
    if (seq < last_seq) { Bluetooth_SendReply(seq, 0U, "OLD_SEQ"); return; }
    if (reply_pending != 0U) return;

    const char *error = NULL;
    EnergyState_t s = Energy_GetState();
    if ((strcmp(cmd, "START") == 0) || (strcmp(cmd, "HIT") == 0) || (strcmp(cmd, "MISS") == 0)) {
        if (!Bluetooth_Number(line, "rev", &rev) || (rev != s.rev)) error = "STALE";
    }
    if (error == NULL && strcmp(cmd, "START") == 0) {
        if (!Bluetooth_String(line, "mode", mode, sizeof(mode)) ||
            (strcmp(mode, "SMALL") != 0 && strcmp(mode, "LARGE") != 0))
            error = "MODE";
        else if (s.phase != 0U) error = "BUSY";
        else Energy_SetMode(strcmp(mode, "SMALL") == 0 ? ENERGY_MODE_SMALL_TEST : ENERGY_MODE_BIG_TEST);
    } else if (error == NULL && ((strcmp(cmd, "HIT") == 0) || (strcmp(cmd, "MISS") == 0))) {
        if (s.phase != 1U) error = "PHASE";
        else if (!Bluetooth_Number(line, "target", &target) || (target != s.target)) error = "STALE";
        else Energy_ReportResult(strcmp(cmd, "HIT") == 0 ? ENERGY_RESULT_HIT : ENERGY_RESULT_MISS);
    } else if (error == NULL && strcmp(cmd, "STOP") == 0) {
        Energy_SetMode(ENERGY_MODE_OFF);
    } else if (error == NULL && strcmp(cmd, "STATUS") != 0 && strcmp(cmd, "HELLO") != 0) {
        error = "BAD_FRAME";
    }
    last_seq = seq;
    strcpy(last_request, line);
    last_seen_ms = HAL_GetTick();
    if (error != NULL) {
        Bluetooth_SendReply(seq, 0U, error);
        strcpy(cached_reply, last_reply);
    } else if (Energy_IsSettled() != 0U) {
        Bluetooth_SendReply(seq, 1U, NULL);
        strcpy(cached_reply, last_reply);
    }
    else { reply_pending = 1U; pending_seq = seq; }
}

uint8_t Bluetooth_Init(void) {
    last_seen_ms = HAL_GetTick();
    return HAL_UART_Receive_IT(&huart1, &rx_byte, 1U) == HAL_OK;
}

void Bluetooth_Task(void) {
    if (line_ready != 0U) {
        Bluetooth_HandleLine(ready_line);
        line_ready = 0U;
    }
    if (reply_pending != 0U && Energy_IsSettled() != 0U) {
        reply_pending = 0U;
        Bluetooth_SendReply(pending_seq, 1U, NULL);
        strcpy(cached_reply, last_reply);
    }
    if ((Energy_GetState().phase != 0U) &&
        ((uint32_t)(HAL_GetTick() - last_seen_ms) > BT_WATCHDOG_MS)) {
        Energy_SetMode(ENERGY_MODE_OFF);
        last_seen_ms = HAL_GetTick();
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance != USART1) return;
    if (rx_byte == '\n') {
        if ((dropping == 0U) && (line_length != 0U) && (line_ready == 0U)) {
            assembling[line_length] = '\0';
            memcpy(ready_line, assembling, line_length + 1U);
            line_ready = 1U;
        }
        line_length = 0U;
        dropping = 0U;
    } else if (rx_byte != '\r' && dropping == 0U) {
        if ((rx_byte < 32U) || (rx_byte > 126U) || (line_length >= BT_MAX_LINE)) {
            dropping = 1U;
            line_length = 0U;
        } else assembling[line_length++] = (char)rx_byte;
    }
    (void)HAL_UART_Receive_IT(&huart1, &rx_byte, 1U);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) (void)HAL_UART_Receive_IT(&huart1, &rx_byte, 1U);
}
