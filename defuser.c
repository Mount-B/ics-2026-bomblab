#include "defuser.h"

#include <inttypes.h>
#include <stdio.h>

#define MAX_FIELDS 8

/*
 * 把 values 中的 count 个整数写成一行“空格分隔的无符号十进制整数”，存入 out。
 * 成功返回 0；count 为 0（还没有实现）、snprintf 出错或结果被截断时返回非零。
 */
static int write_answer(char *out, size_t cap, const uint32_t *values, size_t count) {
    size_t used = 0;
    for (size_t i = 0; i < count; ++i) {
        int size = snprintf(out + used, cap - used, "%s%" PRIu32, i ? " " : "", values[i]);
        if (size < 0 || (size_t)size >= cap - used) return 1;
        used += (size_t)size;
    }
    return count == 0;
}

/* ------------------------------------------------------------------ */
/* 第 1 关：函数调用、参数传递、返回值                                  */
/*   answer = 2*T0 + (T1 ^ 0x746)                                      */
/*   T1 = (hi32 + 2*(c>>16)) ^ 0xbe4b                                  */
/*   T0 = 5*((uint32)c - hi32) + 0xee0c                                */
/* ------------------------------------------------------------------ */
int solve_phase_1(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    uint32_t lo = (uint32_t)challenge;
    uint32_t hi = (uint32_t)(challenge >> 32);
    uint32_t mid = (uint32_t)(challenge >> 16);
    uint32_t t1 = (hi + UINT32_C(2) * mid) ^ UINT32_C(0xbe4b);
    uint32_t t0 = UINT32_C(5) * (lo - hi) + UINT32_C(0xee0c);
    values[0] = t0 * UINT32_C(2) + (t1 ^ UINT32_C(0x746));
    return write_answer(out, cap, values, 1);
}

/* ------------------------------------------------------------------ */
/* 第 2 关：数组、循环、递推                                            */
/*   v[0] = ((c >> 46) & 31) + 2                                       */
/*   v[i] = ((((c >> (11 + 2*(i-1))) & 15) + v[i-1]) ^ i), i = 1..5     */
/* ------------------------------------------------------------------ */
int solve_phase_2(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    values[0] = (uint32_t)((challenge >> 46) & 31u) + UINT32_C(2);
    for (unsigned i = 1; i < 6; ++i) {
        uint32_t part = (uint32_t)((challenge >> (11 + 2 * (i - 1))) & 15u);
        values[i] = (part + values[i - 1]) ^ i;
    }
    return write_answer(out, cap, values, 6);
}

/* ------------------------------------------------------------------ */
/* 第 3 关：switch 分支与跳转表（0x405820）                             */
/*   x = (c>>32) ^ (c>>3)，sel = (uint32)c & 7                          */
/* ------------------------------------------------------------------ */
int solve_phase_3(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    uint32_t sel = (uint32_t)challenge & 7u;
    uint32_t x = (uint32_t)(challenge >> 32) ^ (uint32_t)(challenge >> 3);
    uint32_t r;
    switch (sel) {
        case 0: r = (x ^ UINT32_C(0xa178)) + (x >> 2); break;
        case 1: r = (UINT32_C(4) * x) ^ UINT32_C(0x67f6); break;
        case 2: r = (UINT32_C(4) * x) ^ UINT32_C(0x78af); break;
        case 3: r = (x ^ UINT32_C(0xcd)) + (x >> 2); break;
        case 4: r = UINT32_C(5) * x + UINT32_C(0xa23); break;
        case 5: r = UINT32_C(3) * x + UINT32_C(0xcf31); break;
        case 6: r = UINT32_C(5) * x + UINT32_C(0x8363); break;
        default: r = UINT32_C(3) * x + UINT32_C(0xac86); break;
    }
    values[0] = sel;
    values[1] = r;
    return write_answer(out, cap, values, 2);
}

/* ------------------------------------------------------------------ */
/* 第 4 关：递归（p4_walk 的二分查找）                                  */
/* ------------------------------------------------------------------ */
static uint32_t p4_walk(uint32_t target, uint32_t lo, uint32_t hi, uint32_t acc) {
    uint32_t mid = lo + ((hi - lo) >> 1);
    uint32_t r;
    if (mid == target) return (mid + UINT32_C(0xf3)) ^ acc;
    if (target > mid) {
        r = p4_walk(target, mid + 1, hi, acc);
        return ((r * UINT32_C(5)) ^ (mid + acc)) ^ UINT32_C(0xe);
    }
    r = p4_walk(target, lo, mid - 1, acc);
    return ((r * UINT32_C(5)) ^ (mid + acc)) ^ UINT32_C(0xc);
}

int solve_phase_4(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    uint32_t target = (uint32_t)challenge & 0x7fu;
    uint32_t acc = (uint32_t)(challenge >> 7) ^ (uint32_t)(challenge >> 32);
    values[0] = p4_walk(target, 0, UINT32_C(0x7f), acc);
    return write_answer(out, cap, values, 1);
}

/* ------------------------------------------------------------------ */
/* 第 5 关：结构体布局与链表遍历（p5_nodes @ 0x405ba0，p5_heads @ 0x405b60） */
/* ------------------------------------------------------------------ */
struct p5_node {
    int next;      /* -1 表示链尾 */
    uint32_t f8;   /* +0x8 */
    uint32_t fc;   /* +0xc */
};

static const struct p5_node p5_nodes[] = {
    {7, 337, 9088},   /* 0x405ba0 */
    {0, 2439, 12993}, /* 0x405bb0 */
    {1, 73, 46315},   /* 0x405bc0 */
    {2, 2914, 39157}, /* 0x405bd0 */
    {5, 1842, 24181}, /* 0x405be0 */
    {9, 1664, 37241}, /* 0x405bf0 */
    {3, 2576, 15531}, /* 0x405c00 */
    {8, 1354, 33826}, /* 0x405c10 */
    {10, 1090, 631},  /* 0x405c20 */
    {-1, 568, 36377}, /* 0x405c30 */
    {11, 2080, 55427},/* 0x405c40 */
    {4, 817, 18508},  /* 0x405c50 */
};

static const int p5_heads[8] = {6, 3, 2, 1, 0, 7, 8, 10};

int solve_phase_5(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    unsigned idx = (unsigned)(challenge & 7u);
    unsigned skip = (unsigned)((challenge >> 3) & 3u);
    int n = p5_heads[idx];
    uint32_t acc = (uint32_t)(challenge >> 6) ^ (uint32_t)(challenge >> 32);
    acc ^= p5_nodes[n].fc;
    acc ^= UINT32_C(0x49f1);
    acc += p5_nodes[n].f8;
    uint32_t last = p5_nodes[n].f8;
    for (unsigned i = 0; i < skip; ++i) {
        n = p5_nodes[n].next;
        acc ^= p5_nodes[n].fc;
        acc += p5_nodes[n].f8;
        last = p5_nodes[n].f8;
    }
    values[0] = acc;
    values[1] = last;
    return write_answer(out, cap, values, 2);
}

/* ------------------------------------------------------------------ */
/* 第 6 关：函数指针表与间接调用（p6_ops @ 0x405e00）                   */
/*   表内容：op0, op2, op5, op5, op3, op1, op4, op3                     */
/* ------------------------------------------------------------------ */
static uint32_t p6_op_0(uint32_t x) { return (x ^ UINT32_C(0x3abc)) + (x << 7); }
static uint32_t p6_op_1(uint32_t x) { return ((x >> 3) ^ x) + UINT32_C(0xfe9d); }
static uint32_t p6_op_2(uint32_t x) { return ((x << 7) + UINT32_C(0xa0c1)) ^ (UINT32_C(5) * x); }
static uint32_t p6_op_3(uint32_t x) { return (x + UINT32_C(0xdb07)) ^ (x >> 2); }
static uint32_t p6_op_4(uint32_t x) { return (x >> 2) | (x << 30); }
static uint32_t p6_op_5(uint32_t x) { return ((x << 5) | (x >> 27)) ^ UINT32_C(0x9782); }

static uint32_t p6_apply(unsigned slot, uint32_t x) {
    switch (slot) {
        case 0: return p6_op_0(x);
        case 1: return p6_op_2(x);
        case 2: return p6_op_5(x);
        case 3: return p6_op_5(x);
        case 4: return p6_op_3(x);
        case 5: return p6_op_1(x);
        case 6: return p6_op_4(x);
        default: return p6_op_3(x);
    }
}

int solve_phase_6(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    uint32_t x = (uint32_t)(challenge >> 6) ^ (uint32_t)(challenge >> 32);
    uint32_t y = p6_apply((unsigned)(challenge & 7u), x) ^ UINT32_C(0x2acb);
    uint32_t z = p6_apply((unsigned)((challenge & 0x38u) >> 3), y);
    values[0] = z + UINT32_C(0xa9fb);
    return write_answer(out, cap, values, 1);
}

/* ------------------------------------------------------------------ */
/* Honor：secret_gate 要求整行长度在 [36,163]，缓冲区（strcpy 整行）中      */
/* 偏移 32..35 处必须是 "OPEN"；偏移 36 之后是三个无符号整数              */
/*   A = (uint32)context & 3                                           */
/*   B = secret_mix(mix ^ ctx) ^ ((A * 0xe9d3) ^ (uint32)(context>>16)) */
/*   C = B ^ (0x6b8f + ctx + A + ((mix) >> 1))                          */
/*   其中 mix = (uint32)(challenge>>32) ^ (uint32)challenge              */
/*        ctx = (uint32)(context>>32)  ^ (uint32)context                 */
/* ------------------------------------------------------------------ */
int solve_secret(uint64_t challenge, uint64_t context, char *out, size_t cap) {
    uint32_t sel = (uint32_t)context & 3u;
    uint32_t mix = (uint32_t)(challenge >> 32) ^ (uint32_t)challenge;
    uint32_t ctx = (uint32_t)(context >> 32) ^ (uint32_t)context;
    uint32_t key = (sel * UINT32_C(0xe9d3)) ^ (uint32_t)(context >> 16);
    uint32_t arg = mix ^ ctx;
    uint32_t rolled = (arg >> 1) | (arg << 31);          /* ror 1 */
    uint32_t b = (rolled ^ UINT32_C(0x825c)) ^ key;
    uint32_t c = b ^ (UINT32_C(0x6b8f) + ctx + sel + (mix >> 1));
    int n = snprintf(out, cap, "%sOPEN %" PRIu32 " %" PRIu32 " %" PRIu32,
                     "0123456789abcdef0123456789abcdef", sel, b, c);
    return n < 0 || (size_t)n >= cap;
}
