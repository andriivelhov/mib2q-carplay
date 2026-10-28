/* alt111_video queue: trailing AUD, overflow skip-to-IDR without a consumer restart. */
#include "alt111.h"
#include <stdio.h>
#include <string.h>

static int failures;
#define CHECK(c) do { if (!(c)) { ++failures; printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); } } while (0)

static const uint8_t avcc[] = {
    1, 0x42, 0x00, 0x1f, 0xff, 0xe1,
    0, 4, 0x67, 0x42, 0x00, 0x1f,      /* SPS */
    1, 0, 2, 0x68, 0xce                 /* PPS */
};
static const uint8_t idr_au[] = { 0, 0, 0, 3, 0x65, 0x88, 0x84 };
static const uint8_t p_au[] = { 0, 0, 0, 3, 0x41, 0x9a, 0x02 };
static const uint8_t aud[] = { 0, 0, 0, 1, 9, 0xf0 };

/* Write out the head chunk completely, like the tee worker does. */
static int drain_one(struct alt111_video *v, int *was_idr, int *ends_with_aud)
{
    const uint8_t *b; size_t n; struct alt111_output_ticket t;
    if (alt111_video_peek(v, &b, &n, &t) != ALT111_OK) return 0;
    *was_idr = (int)t.idr;
    *ends_with_aud = n >= sizeof(aud) && !memcmp(b + n - sizeof(aud), aud, sizeof(aud));
    CHECK(alt111_video_advance(v, &t, n) == ALT111_OK);
    return 1;
}

int main(void)
{
    struct alt111_video v;
    uint64_t stream, consumer;
    unsigned i;
    int idr, has_aud, rc;
    const uint8_t *b; size_t n; struct alt111_output_ticket t;

    alt111_video_init(&v);
    stream = alt111_video_begin(&v, 1);
    CHECK(alt111_video_config(&v, stream, avcc, sizeof(avcc)) == ALT111_OK);
    CHECK(alt111_video_attach(&v) == ALT111_OK);
    consumer = v.consumer;

    /* Every forwarded picture ends with an access unit delimiter. */
    CHECK(alt111_video_submit(&v, stream, idr_au, sizeof(idr_au), 1) == ALT111_OK);
    CHECK(alt111_video_submit(&v, stream, p_au, sizeof(p_au), 1) == ALT111_OK);
    CHECK(drain_one(&v, &idr, &has_aud) && idr && has_aud);
    CHECK(drain_one(&v, &idr, &has_aud) && !idr && has_aud);
    CHECK(v.consumer_primed);

    /* Fill the queue, start writing the head chunk, then overflow. */
    for (i = 0; i < ALT111_QUEUE_CAP; ++i)
        CHECK(alt111_video_submit(&v, stream, p_au, sizeof(p_au), 1) == ALT111_OK);
    CHECK(alt111_video_peek(&v, &b, &n, &t) == ALT111_OK);
    CHECK(alt111_video_advance(&v, &t, 2) == ALT111_OK);   /* head partly sent */
    rc = alt111_video_submit(&v, stream, p_au, sizeof(p_au), 1);
    CHECK(rc == ALT111_SKIP_TO_IDR);
    CHECK(v.attached && v.consumer == consumer);           /* no consumer restart */
    CHECK(v.count == 1 && v.queue[v.head].offset == 2);    /* partial head kept */
    CHECK(v.awaiting_idr);

    /* Pictures that depend on dropped ones are not forwarded. */
    CHECK(alt111_video_submit(&v, stream, p_au, sizeof(p_au), 1) == ALT111_WAIT);
    CHECK(v.count == 1);

    /* The next IDR resumes forwarding on the same consumer. */
    CHECK(alt111_video_submit(&v, stream, idr_au, sizeof(idr_au), 1) == ALT111_OK);
    CHECK(!v.awaiting_idr && v.count == 2);
    CHECK(alt111_video_peek(&v, &b, &n, &t) == ALT111_OK);
    CHECK(alt111_video_advance(&v, &t, n) == ALT111_OK);   /* finish partial head */
    CHECK(drain_one(&v, &idr, &has_aud) && idr && has_aud);
    CHECK(v.count == 0 && v.queued_bytes == 0);

    /* An overflowing IDR replaces the backlog directly. */
    for (i = 0; i < ALT111_QUEUE_CAP; ++i)
        CHECK(alt111_video_submit(&v, stream, p_au, sizeof(p_au), 1) == ALT111_OK);
    CHECK(alt111_video_submit(&v, stream, idr_au, sizeof(idr_au), 1) == ALT111_OK);
    CHECK(v.count == 1 && !v.awaiting_idr && v.consumer == consumer);
    CHECK(drain_one(&v, &idr, &has_aud) && idr);

    /* Detach clears the skip state. */
    for (i = 0; i <= ALT111_QUEUE_CAP; ++i)
        (void)alt111_video_submit(&v, stream, p_au, sizeof(p_au), 1);
    CHECK(v.awaiting_idr);
    alt111_video_detach(&v);
    CHECK(!v.awaiting_idr && v.count == 0 && v.queued_bytes == 0);

    alt111_video_destroy(&v);
    if (failures) { printf("alt111_video_test: %d failure(s)\n", failures); return 1; }
    printf("alt111_video_test: OK\n");
    return 0;
}
