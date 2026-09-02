/**
 * ui.c — THE INPUT LAYER.  ****  YOU WRITE THE BOTTOM HALF  ****
 *
 * Every action here does the same three things and nothing else:
 *
 *      ask the user  ->  validate  ->  write ONE field of the model
 *
 * None of them decide anything. Deciding what a lamp should do is the job of
 * applyRules() in house.c. Keeping those apart is what makes either of them
 * testable — and it is worth marks.
 *
 *  THE FIVE FUNCTIONS ARE IN THE ORDER YOU SHOULD WRITE THEM.
 *  Work straight down the file. Each one is marked [ N / 5 ].
 *
 *      [ 1 / 5 ]  setOccupancy()     FR-08   easiest — start here
 *      [ 2 / 5 ]  setTemperature()   FR-09   ask, range-check, store
 *      [ 3 / 5 ]  switchDevice()     FR-07   a switch with 3 cases
 *      [ 4 / 5 ]  houseReport()      FR-11   counters and bars
 *      [ 5 / 5 ]  runAutomation()    FR-10   the trace — hardest, do it last
 *
 * The top half is GIVEN: the menu, the input helpers, and pickRoom(), which
 * is your validation template. Copy its shape.
 *
 * Smart Home Console · Day 03 midterm — G9
 * Student: <YOUR NAME HERE>
 */
#include <stdio.h>
#include "ui.h"
#include "house.h"
#include "render.h"
#include "platform.h"

/* Format one status byte from the most-significant bit to the least-significant bit. */
#define BIT_FMT "%u%u%u%u%u%u%u%u"
#define BIT_VAL(value) \
    ((unsigned)READ_BIT((value), 7U)), ((unsigned)READ_BIT((value), 6U)), \
    ((unsigned)READ_BIT((value), 5U)), ((unsigned)READ_BIT((value), 4U)), \
    ((unsigned)READ_BIT((value), 3U)), ((unsigned)READ_BIT((value), 2U)), \
    ((unsigned)READ_BIT((value), 1U)), ((unsigned)READ_BIT((value), 0U))

/* ============================================================================
 *                            GIVEN — read these
 * ========================================================================== */

void printMenu(void)
{
    /* two fixed-width columns, so the numbers line up under each other */
    printf("\n  %s1%s) %-26s%s2%s) %s\n", CC(C_SEL), CC(C_RESET),
           "Show the house", CC(C_SEL), CC(C_RESET), "Switch lamp/fan/auto");
    printf("  %s3%s) %-26s%s4%s) %s\n", CC(C_SEL), CC(C_RESET),
           "Someone enters/leaves", CC(C_SEL), CC(C_RESET), "Set room temperature");
    printf("  %s5%s) %-26s%s6%s) %s\n", CC(C_SEL), CC(C_RESET),
           "Run automation", CC(C_SEL), CC(C_RESET), "House report");
    printf("  %s7%s) %-26s%s0%s) %s\n", CC(C_SEL), CC(C_RESET),
           "Auto demo (scripted)", CC(C_SEL), CC(C_RESET), "Exit");
    printf("\n  Select > ");
    fflush(stdout);
}

/* Read one whole line, then parse an int out of it. Returns 0 on EOF or on
 * junk like "hello". Reading a LINE at a time means the buffer is never left
 * dirty, so the "press Enter" pause below actually waits. */
int readInt(int *out)
{
    char buf[64];
    if (fgets(buf, (int)sizeof buf, stdin) == NULL) { return 0; }
    return sscanf(buf, "%d", out) == 1;
}

void pauseKey(void)
{
    char buf[64];
    if (g_plain) { return; }
    printf("\n%s  -- press Enter --%s", CC(C_DIM), CC(C_RESET));
    fflush(stdout);
    if (fgets(buf, (int)sizeof buf, stdin) == NULL) { /* EOF: carry on */ }
}

void printBinary(uint8_t value)
{
    printf("0b");
    for (int8_t bit = 7; bit >= 0; bit--) {
        putchar(READ_BIT(value, bit) ? '1' : '0');
    }
}

/* THE VALIDATION TEMPLATE. Ask for a room index, reject anything that is not
 * 0..ROOM_COUNT-1, and return 255 to mean "bad input, do nothing".
 * Every one of your functions below starts with this same shape. */
uint8_t pickRoom(void)
{
    int n = -1;

    printf("  Room (");
    for (uint8_t i = 0U; i < ROOM_COUNT; i++) {
        printf("%s%u%s=%s ", CC(C_SEL), i, CC(C_RESET), houseRoom(i)->name);
    }
    printf("): ");

    if (!readInt(&n) || n < 0 || n >= (int)ROOM_COUNT) {
        statusSet(C_ALARM, "No such room.");
        return 255U;
    }
    return (uint8_t)n;
}

/* ============================================================================
 *                              YOUR WORK
 *
 * Useful calls you already have:
 *   pickRoom()                    -> room index, or 255 on bad input
 *   houseRoom(i)                  -> Room_t * for room i
 *   tempC(r->adc)                 -> that room's temperature in C
 *   statusSet(COLOUR, "fmt", ...) -> the message line under the schematic
 *                                    (works exactly like printf)
 *   render(i)                     -> redraw, with room i's border glowing;
 *                                    pass -1 to highlight nothing
 *   pauseKey()                    -> "press Enter", so output can be read
 *   printBinary(byte)             -> prints 0b00001011
 *   drawBar(v, full, width, col)  -> the bar renderer, from render.h
 *
 * Colours for statusSet: C_OK (green), C_ALARM (red), C_WARM (amber),
 * C_COOL (blue), C_LAMP (yellow), C_FAN (cyan), C_DIM (grey), C_AUTO, C_MAN.
 * ========================================================================== */


/* ============================================================================
 *  [ 1 / 5 ]   YOUR WORK HERE   setOccupancy()                     FR-08
 * --------------------------------------------------------------------------
 *  REQUIRES : house.c [ 1 / 6 ] houseInit(), so the rooms have names.
 *  GIVES    : menu 3 works  "people: yes/no" flips on the schematic.
 *  USES     : pickRoom(), houseRoom(), TOGGLE_BIT, BIT_OCCUPIED,
 *             READ_BIT, statusSet(), render(), pauseKey()
 *  CHECK    : menu 3, pick Kitchen, its people line flips. The LAMP must
 *             NOT move  only the sensor bit changed.
 * ==========================================================================
 *
 * Somebody walks in or out. The shortest function in the file — do it first
 * to get the shape into your fingers.
 *
 *   1. i = pickRoom(); if it is 255, return immediately
 *   2. TOGGLE_BIT the OCCUPIED flag of that room
 *   3. statusSet() a message saying who is in the room now
 *   4. render((int)i) then pauseKey()
 *
 * NOTE: this changes the SENSOR bit and nothing else. The lamp does not move
 */

void setOccupancy(void)
{
    uint8_t idx = pickRoom();
    if (idx == 255U) return;

    TOGGLE_BIT(houseRoom(idx)->status, BIT_OCCUPIED);

    if (READ_BIT(houseRoom(idx)->status, BIT_OCCUPIED)) {
        statusSet(C_OK, "Person entered room");
    } else {
        statusSet(C_DIM, "Person left room");
    }

    render((int)idx);
    pauseKey();
}

void setTemperature(void)
{
    uint8_t idx = pickRoom();
    if (idx == 255U) return;

    int val = -1;
    printf(" Enter new ADC (0..1023) > ");
    if (!readInt(&val) || val < 0 || val > 1023) {
        statusSet(C_ALARM, "Invalid ADC value.");
        return;
    }

    uint16_t old_adc = houseRoom(idx)->adc;
    houseRoom(idx)->adc = (uint16_t)val;

    char msg[128];
    snprintf(msg, sizeof(msg), "%s: ADC %u -> %u (%u C)", houseRoom(idx)->name, old_adc, houseRoom(idx)->adc, tempC(houseRoom(idx)->adc));
    statusSet(C_OK, msg);

    render((int)idx);
    pauseKey();
}

void switchDevice(void)
{
    uint8_t idx = pickRoom();
    if (idx == 255U) return;

    Room_t *r = houseRoom(idx);

    printf("\n 1) Toggle Lamp\n 2) Toggle Fan\n 3) Toggle Auto\n Choice > ");
    int choice = -1;
    readInt(&choice);

    if (choice == 1) {
        TOGGLE_BIT(r->status, BIT_LAMP);
        CLR_BIT(r->status, BIT_AUTO);
        statusSet(C_OK, "Lamp toggled (AUTO disabled)");
    } else if (choice == 2) {
        TOGGLE_BIT(r->status, BIT_FAN);
        CLR_BIT(r->status, BIT_AUTO);
        statusSet(C_OK, "Fan toggled (AUTO disabled)");
    } else if (choice == 3) {
        TOGGLE_BIT(r->status, BIT_AUTO);
        statusSet(C_OK, "Auto toggled");
    } else {
        statusSet(C_DIM, "Nothing switched.");
    }

    render((int)idx);
    pauseKey();
}

void houseReport(void)
{
    uint8_t lamps = countRoomsWith(BIT_LAMP);
    uint8_t fans = countRoomsWith(BIT_FAN);
    uint8_t people = countRoomsWith(BIT_OCCUPIED);
    uint8_t alarms = countRoomsWith(BIT_ALARM);

    printf(" Lamps ON   : ");
    drawBar(lamps, ROOM_COUNT, REPORT_BAR_W, C_LAMP);
    printf(" Fans ON    : ");
    drawBar(fans, ROOM_COUNT, REPORT_BAR_W, C_FAN);
    printf(" Occupied   : ");
    drawBar(people, ROOM_COUNT, REPORT_BAR_W, C_OK);
    printf(" Alarms     : ");
    drawBar(alarms, ROOM_COUNT, REPORT_BAR_W, C_ALARM);

    uint8_t hottest_idx = 0;
    uint8_t coldest_idx = 0;
    for (uint8_t i = 1; i < ROOM_COUNT; i++) {
        if (houseRoom(i)->adc > houseRoom(hottest_idx)->adc) {
            hottest_idx = i;
        }
        if (houseRoom(i)->adc < houseRoom(coldest_idx)->adc) {
            coldest_idx = i;
        }
    }

    printf("\n Hottest room : %s (%u C)\n", houseRoom(hottest_idx)->name, tempC(houseRoom(hottest_idx)->adc));
    printf(" Coldest room : %s (%u C)\n", houseRoom(coldest_idx)->name, tempC(houseRoom(coldest_idx)->adc));

    uint32_t total_adc = sumAdc(houseRooms(), ROOM_COUNT);
    uint16_t avg_temp = tempC((uint16_t)(total_adc / ROOM_COUNT));
    printf(" Average temp : %u C\n\n", avg_temp);

    pauseKey();
}

void runAutomation(void)
{
    char trace[ROOM_COUNT][96];
    uint8_t changed = 0;

    for (uint8_t i = 0; i < ROOM_COUNT; i++) {
        Room_t *r = houseRoom(i);
        uint16_t t = tempC(r->adc);
        if (!READ_BIT(r->status, BIT_AUTO)) {
            snprintf(trace[i], sizeof(trace[i]), " %-10s %3u C  skipped (MANUAL)", r->name, t);
        } else {
            uint8_t before = r->status;
            uint8_t ch = applyRules(r);
            if (ch) {
                changed++;
                snprintf(trace[i], sizeof(trace[i]), " %-10s %3u C  0b" BIT_FMT " -> 0b" BIT_FMT " *",
                         r->name, t, BIT_VAL(before), BIT_VAL(r->status));
            } else {
                snprintf(trace[i], sizeof(trace[i]), " %-10s %3u C  0b" BIT_FMT " -> 0b" BIT_FMT,
                         r->name, t, BIT_VAL(before), BIT_VAL(r->status));
            }
        }
    }

    for (uint8_t i = 0; i < ROOM_COUNT; i++) {
        puts(trace[i]);
    }

    printf("\n%u room(s) changed.\n", changed);
    pauseKey();
}