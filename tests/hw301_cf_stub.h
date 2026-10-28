/* Minimal owned CF graph for testing the bridge with no target libraries. */
#ifndef HW301_CF_STUB_H
#define HW301_CF_STUB_H
#include <assert.h>

typedef struct test_cf {
    unsigned long type;
    int refs;
    int64_t number;
    char *string;
    struct test_cf *keys[64], *values[64];
    long count;
} test_cf;
static unsigned long t_dict_type(void) { return 1; }
static unsigned long t_array_type(void) { return 2; }
static unsigned long t_string_type(void) { return 3; }
static unsigned long t_number_type(void) { return 4; }
static unsigned long t_type(void *p) { assert(p); return ((test_cf *)p)->type; }
static void *t_new(unsigned long type) {
    test_cf *p = calloc(1, sizeof *p); assert(p); p->type = type; p->refs = 1; return p;
}
static void *t_retain(void *p) { if (p) ++((test_cf *)p)->refs; return p; }
static void t_release(void *v) {
    test_cf *p = v; long i;
    if (!p || --p->refs) return;
    for (i = 0; i < p->count; ++i) { t_release(p->keys[i]); t_release(p->values[i]); }
    free(p->string); free(p);
}
static void *t_string(void *a, const char *s, uint32_t enc) {
    test_cf *p = t_new(3); (void)a; (void)enc; p->string = strdup(s); return p;
}
static const char *t_string_ptr(void *v, uint32_t enc) { (void)enc; return ((test_cf *)v)->string; }
static int t_string_get(void *v, char *out, long cap, uint32_t enc) {
    const char *p = t_string_ptr(v, enc);
    if (!p || strlen(p) >= (size_t)cap) return 0;
    strcpy(out, p); return 1;
}
static void *t_dict_new(void *a, long n, const void *k, const void *v) {
    (void)a; (void)n; (void)k; (void)v; return t_new(1);
}
static void *t_array_new(void *a, long n, const void *cb) {
    (void)a; (void)n; (void)cb; return t_new(2);
}
static void *t_get(void *v, void *key) {
    test_cf *d = v, *k = key; long i;
    assert(d && d->type == 1 && k && k->type == 3);
    for (i = 0; i < d->count; ++i)
        if (!strcmp(d->keys[i]->string, k->string)) return d->values[i];
    return NULL;
}
static void t_set(void *v, void *key, void *value) {
    test_cf *d = v, *k = key; long i;
    assert(d && d->type == 1 && value);
    for (i = 0; i < d->count; ++i)
        if (!strcmp(d->keys[i]->string, k->string)) {
            t_retain(value); t_release(d->values[i]); d->values[i] = value; return;
        }
    assert(d->count < 64);
    d->keys[d->count] = t_retain(key); d->values[d->count++] = t_retain(value);
}
static void t_remove(void *v, void *key) {
    test_cf *d = v, *k = key; long i;
    for (i = 0; i < d->count; ++i) if (!strcmp(d->keys[i]->string, k->string)) {
        t_release(d->keys[i]); t_release(d->values[i]); --d->count;
        d->keys[i] = d->keys[d->count]; d->values[i] = d->values[d->count]; return;
    }
}
static void t_set_int(void *d, void *k, int64_t n) {
    test_cf *v = t_new(4); v->number = n; t_set(d,k,v); t_release(v);
}
static int64_t t_get_int(void *d, void *k, int *err) {
    test_cf *v = t_get(d,k);
    if (err) *err = !v || v->type != 4;
    return v && v->type == 4 ? v->number : 0;
}
static void t_append(void *v, const void *value) {
    test_cf *a = v; assert(a->type == 2 && a->count < 64);
    a->values[a->count++] = t_retain((void *)value);
}
static long t_count(void *v) { assert(v && ((test_cf *)v)->type == 2); return ((test_cf *)v)->count; }
static void *t_at(void *v, long i) {
    test_cf *a = v; assert(i >= 0 && i < a->count); return a->values[i];
}
static void *t_copy(void *a, long n, void *v) {
    test_cf *src = v, *dst = t_new(src->type); long i; (void)a; (void)n;
    for (i = 0; i < src->count; ++i) {
        dst->keys[i] = t_retain(src->keys[i]); dst->values[i] = t_retain(src->values[i]);
    }
    dst->count = src->count; return dst;
}
#endif
