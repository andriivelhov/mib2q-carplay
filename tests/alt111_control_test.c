/*
 * altscreen_hook control core: session start sequence and the suggestUI reshow
 * (issue #35: the app taking over the cluster mid-session needs a new showUI,
 * and with ETA ON a changed URL first).
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "alt111.h"

#include <stdio.h>
#include <string.h>

static int failures;
#define CHECK(c, msg) do { if (!(c)) { printf("FAIL: %s\n", msg); failures++; } } while (0)

static uint64_t now_ms = 1000;
static struct alt111_command last;

/* Next command, acknowledged at once; NONE when the controller waits. */
static enum alt111_command_type step(struct alt111_control *c, int success)
{
    struct alt111_command cmd;
    if (alt111_control_next(c, now_ms, &cmd) != ALT111_OK) return ALT111_CMD_NONE;
    alt111_control_complete(c, cmd.session, cmd.request, success, now_ms + 10);
    now_ms += 20;
    last = cmd;
    return cmd.type;
}

static int flip_show(struct alt111_control *c)
{
    return step(c, 1) == ALT111_CMD_SHOW && last.flip;
}

static int plain_show(struct alt111_control *c)
{
    return step(c, 1) == ALT111_CMD_SHOW && !last.flip;
}

static void settle(struct alt111_control *c)
{
    int n;
    for (n = 0; n < 8 && step(c, 1) != ALT111_CMD_NONE; ++n) {}
}

static void url_tests(void)
{
    char out[128];

    CHECK(alt111_url_eta_off("maps:/car/instrumentcluster?showETA=yes&showSpeedLimit=no&showCompass=no",
                             out, sizeof(out)) == 1 &&
          strcmp(out, "maps:/car/instrumentcluster?showETA=no&showSpeedLimit=no&showCompass=no") == 0,
          "url: first item");
    CHECK(alt111_url_eta_off("maps:/car/instrumentcluster/map?showSpeedLimit=user&showCompass=user&showETA=yes&maneuverLayout=",
                             out, sizeof(out)) == 1 &&
          strcmp(out, "maps:/car/instrumentcluster/map?showSpeedLimit=user&showCompass=user&showETA=no&maneuverLayout=") == 0,
          "url: middle item");
    CHECK(alt111_url_eta_off("maps:/car/instrumentcluster?showSpeedLimit=yes&showETA=yes", out, sizeof(out)) == 1 &&
          strcmp(out, "maps:/car/instrumentcluster?showSpeedLimit=yes&showETA=no") == 0,
          "url: last item, other yes items kept");

    memcpy(out, "untouched", sizeof("untouched"));
    CHECK(alt111_url_eta_off("maps:/car/instrumentcluster?showETA=no&showSpeedLimit=yes", out, sizeof(out)) == 0,
          "url: ETA off");
    CHECK(alt111_url_eta_off("maps:/car/instrumentcluster?showETA=yesno", out, sizeof(out)) == 0,
          "url: value must be exact");
    CHECK(alt111_url_eta_off("maps:/car/instrumentcluster?xshowETA=yes", out, sizeof(out)) == 0,
          "url: key must be exact");
    CHECK(alt111_url_eta_off("maps:/car/instrumentcluster", out, sizeof(out)) == 0, "url: no query");
    CHECK(alt111_url_eta_off("maps:/car&showETA=yes", out, sizeof(out)) == 0, "url: no '?'");
    CHECK(alt111_url_eta_off("maps:/x?showETA=yes", out, 18) == 0, "url: out too short");
    CHECK(strcmp(out, "untouched") == 0, "url: out untouched when not flipped");
    CHECK(alt111_url_eta_off("maps:/x?showETA=yes", out, 19) == 1 &&
          strcmp(out, "maps:/x?showETA=no") == 0, "url: out exactly big enough");
}

int main(void)
{
    struct alt111_control c;
    struct alt111_command cmd;
    uint64_t s;

    url_tests();

    CHECK(alt111_control_init(&c, 3) == ALT111_OK, "init");
    s = alt111_control_begin(&c);
    CHECK(alt111_control_reshow(&c, s, 0) == ALT111_WAIT, "reshow before projection waits");
    alt111_control_intent(&c, 1, 1);

    /* Session start: SHOW, VIEW, KEYFRAME, then idle. */
    CHECK(step(&c, 1) == ALT111_CMD_SHOW, "start: SHOW");
    CHECK(step(&c, 1) == ALT111_CMD_VIEW, "start: VIEW");
    CHECK(step(&c, 1) == ALT111_CMD_KEYFRAME, "start: KEYFRAME");
    CHECK(step(&c, 1) == ALT111_CMD_NONE, "start: idle");

    /* suggestUI: the same sequence again, with no STOP first. */
    CHECK(alt111_control_reshow(&c, s, 0) == ALT111_OK, "reshow accepted");
    CHECK(plain_show(&c), "reshow: SHOW");
    CHECK(step(&c, 1) == ALT111_CMD_VIEW, "reshow: VIEW");
    CHECK(step(&c, 1) == ALT111_CMD_KEYFRAME, "reshow: KEYFRAME");
    CHECK(step(&c, 1) == ALT111_CMD_NONE, "reshow: idle");

    /* A burst coalesces into one SHOW. */
    alt111_control_reshow(&c, s, 0);
    alt111_control_reshow(&c, s, 0);
    CHECK(step(&c, 1) == ALT111_CMD_SHOW, "burst: one SHOW");
    CHECK(step(&c, 1) == ALT111_CMD_VIEW, "burst: VIEW, no second SHOW");
    settle(&c);

    /* ETA ON: the flipped SHOW, at once the real one, then VIEW and one KEYFRAME. */
    CHECK(alt111_control_reshow(&c, s, 1) == ALT111_OK, "flip accepted");
    CHECK(flip_show(&c), "flip: flipped SHOW");
    CHECK(plain_show(&c), "flip: real SHOW next");
    CHECK(step(&c, 1) == ALT111_CMD_VIEW, "flip: VIEW");
    CHECK(step(&c, 1) == ALT111_CMD_KEYFRAME, "flip: KEYFRAME");
    CHECK(step(&c, 1) == ALT111_CMD_NONE, "flip: idle");

    /* A burst gives one flipped SHOW, also while it is in flight. */
    alt111_control_reshow(&c, s, 1);
    alt111_control_reshow(&c, s, 1);
    CHECK(alt111_control_next(&c, now_ms, &cmd) == ALT111_OK && cmd.type == ALT111_CMD_SHOW && cmd.flip,
          "flip burst: flipped SHOW in flight");
    alt111_control_reshow(&c, s, 1);
    alt111_control_complete(&c, cmd.session, cmd.request, 1, now_ms + 10);
    now_ms += 20;
    CHECK(plain_show(&c), "flip burst: real SHOW, no second flip");
    settle(&c);

    /* A real SHOW in flight: the flip follows it, then the real SHOW again. */
    alt111_control_reshow(&c, s, 0);
    CHECK(alt111_control_next(&c, now_ms, &cmd) == ALT111_OK && cmd.type == ALT111_CMD_SHOW && !cmd.flip,
          "SHOW in flight");
    alt111_control_reshow(&c, s, 1);
    alt111_control_complete(&c, cmd.session, cmd.request, 1, now_ms + 10);
    now_ms += 20;
    CHECK(flip_show(&c), "after SHOW in flight: flipped SHOW");
    CHECK(plain_show(&c), "after SHOW in flight: real SHOW");
    settle(&c);
    CHECK(c.shown_ack && !c.flip && c.acknowledged_view == 1, "flip settled");

    /* A failed flipped SHOW may still have been applied: the real SHOW follows
     * after the retry delay, never a second flip. */
    alt111_control_reshow(&c, s, 1);
    CHECK(step(&c, 0) == ALT111_CMD_SHOW && last.flip, "flip fails");
    CHECK(step(&c, 1) == ALT111_CMD_NONE, "flip failed: retry delay");
    now_ms += ALT111_RETRY_MS;
    CHECK(plain_show(&c), "flip failed: real SHOW, not flipped again");
    settle(&c);
    CHECK(c.shown_ack && !c.flip, "flip failure settled");

    /* A flipped SHOW that times out counts as failed the same way. */
    alt111_control_reshow(&c, s, 1);
    CHECK(alt111_control_next(&c, now_ms, &cmd) == ALT111_OK && cmd.flip, "flip in flight");
    now_ms += ALT111_COMMAND_TIMEOUT_MS;
    CHECK(alt111_control_next(&c, now_ms, &cmd) == ALT111_WAIT, "flip timed out: retry delay");
    now_ms += ALT111_RETRY_MS;
    CHECK(plain_show(&c), "flip timed out: real SHOW");
    settle(&c);

    /* Projection off drops a due flip; the next SHOW carries the real URL. */
    alt111_control_reshow(&c, s, 1);
    alt111_control_intent(&c, 0, 1);
    CHECK(step(&c, 1) == ALT111_CMD_STOP, "off: STOP");
    alt111_control_intent(&c, 1, 1);
    CHECK(plain_show(&c), "on again: real SHOW, no flip");
    settle(&c);

    /* A VIEW in flight completes first; SHOW follows. */
    alt111_control_intent(&c, 1, 0);
    CHECK(alt111_control_next(&c, now_ms, &cmd) == ALT111_OK && cmd.type == ALT111_CMD_VIEW,
          "view change: VIEW in flight");
    CHECK(alt111_control_reshow(&c, s, 0) == ALT111_OK, "reshow during VIEW");
    CHECK(alt111_control_complete(&c, cmd.session, cmd.request, 1, now_ms + 10) == ALT111_OK,
          "in-flight VIEW still completes");
    now_ms += 20;
    CHECK(step(&c, 1) == ALT111_CMD_SHOW, "SHOW after the VIEW");
    settle(&c);
    CHECK(c.acknowledged_view == 0, "desired view acknowledged after reshow");

    /* Retries exhausted on a VIEW: a reshow starts a fresh transaction. */
    alt111_control_intent(&c, 1, 2);
    CHECK(step(&c, 0) == ALT111_CMD_VIEW, "VIEW fails 1");
    now_ms += ALT111_RETRY_MS;
    CHECK(step(&c, 0) == ALT111_CMD_VIEW, "VIEW fails 2");
    now_ms += ALT111_RETRY_MS;
    CHECK(step(&c, 0) == ALT111_CMD_VIEW, "VIEW fails 3");
    now_ms += ALT111_RETRY_MS;
    CHECK(step(&c, 1) == ALT111_CMD_NONE, "exhausted: idle");
    CHECK(alt111_control_reshow(&c, s, 0) == ALT111_OK, "reshow after exhaustion");
    CHECK(step(&c, 1) == ALT111_CMD_SHOW, "exhausted: SHOW again");
    settle(&c);

    /* Reacquire (STOP then SHOW) already ends in a SHOW: reshow adds nothing. */
    CHECK(alt111_control_reacquire(&c, s) == ALT111_OK, "reacquire");
    CHECK(alt111_control_reshow(&c, s, 0) == ALT111_OK, "reshow during reacquire");
    CHECK(step(&c, 1) == ALT111_CMD_STOP, "reacquire: STOP");
    CHECK(step(&c, 1) == ALT111_CMD_SHOW, "reacquire: SHOW");
    CHECK(step(&c, 1) == ALT111_CMD_VIEW, "reacquire: VIEW, no extra SHOW");
    settle(&c);

    /* ...but a flip during a reacquire still goes before its real SHOW. */
    CHECK(alt111_control_reacquire(&c, s) == ALT111_OK, "reacquire 2");
    CHECK(alt111_control_reshow(&c, s, 1) == ALT111_OK, "flip during reacquire");
    CHECK(step(&c, 1) == ALT111_CMD_STOP, "reacquire 2: STOP");
    CHECK(flip_show(&c), "reacquire 2: flipped SHOW");
    CHECK(plain_show(&c), "reacquire 2: real SHOW");
    settle(&c);
    CHECK(!c.reacquiring && c.shown_ack, "reacquire 2 settled");

    /* Stale or ended sessions are refused; teardown drops a due flip. */
    CHECK(alt111_control_reshow(&c, s + 1, 1) == ALT111_STALE, "other session stale");
    CHECK(alt111_control_reshow(&c, s, 1) == ALT111_OK, "flip due at teardown");
    alt111_control_end(&c, s);
    CHECK(alt111_control_reshow(&c, s, 1) == ALT111_STALE, "ended session stale");
    s = alt111_control_begin(&c);
    CHECK(plain_show(&c), "new session: real SHOW, the old flip dropped");

    if (failures) return 1;
    printf("OK\n");
    return 0;
}
