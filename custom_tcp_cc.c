#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/tcp.h>
#include <linux/spinlock.h>
#include <linux/atomic.h>
#include <linux/string.h>

// Thread-safe global statistics
static spinlock_t stats_lock;
static atomic64_t total_connections = ATOMIC64_INIT(0);
static atomic64_t total_packets = ATOMIC64_INIT(0);

// Scaling constant
static u32 cubic_c = 410;

// Multiplicative decrease factor (beta * 1024)
static u32 beta = 717;

// Per-connection state
struct custom_cc {
    u32 last_max_cwnd;
    u32 epoch_start;
    u32 origin_point;
    u32 K;
};

// Simple cubic root approximation
static u32 cubic_root(u64 a)
{
    return int_sqrt(a);
}

// Congestion avoidance logic
static void custom_cong_avoid(struct sock *sk, u32 ack, u32 acked)
{
    struct tcp_sock *tp = tcp_sk(sk);
    struct custom_cc *ca = inet_csk_ca(sk);

    // Track total packets globally
    atomic64_add(acked, &total_packets);

    // Slow start region
    if (tp->snd_cwnd <= tp->snd_ssthresh) {
        tcp_slow_start(tp, acked);
        return;
    }

    // Initialize epoch if first time
    if (!ca->epoch_start)
        ca->epoch_start = tcp_jiffies32;

    u32 t = tcp_jiffies32 - ca->epoch_start;
    u32 target = ca->origin_point + cubic_root(t);

    if (target > tp->snd_cwnd)
        tp->snd_cwnd++;
}

// Multiplicative decrease when loss occurs
static u32 custom_ssthresh(struct sock *sk)
{
    const struct tcp_sock *tp = tcp_sk(sk);

    return max(tp->snd_cwnd * beta / 1024U, 2U);
}

// Initialize per-socket congestion state
static void custom_init(struct sock *sk)
{
    struct custom_cc *ca = inet_csk_ca(sk);

    // Clear memory
    memset(ca, 0, sizeof(*ca));

    // Set origin point
    ca->origin_point = tcp_sk(sk)->snd_cwnd;

    // Track total connections
    atomic64_inc(&total_connections);
}

// Called when socket is cloned (forked)
static void custom_clone(struct sock *sk, struct sock *newsk)
{
    struct custom_cc *old_ca = inet_csk_ca(sk);
    struct custom_cc *new_ca = inet_csk_ca(newsk);

    // Copy per-socket state
    memcpy(new_ca, old_ca, sizeof(*new_ca));

    atomic64_inc(&total_connections);
}

// Called when socket is released
static void custom_release(struct sock *sk)
{
    struct custom_cc *ca = inet_csk_ca(sk);

    // Zero memory before kernel frees it
    memset(ca, 0, sizeof(*ca));

    atomic64_dec(&total_connections);
}

// Main TCP congestion ops struct
static struct tcp_congestion_ops custom_cc __read_mostly = {
    .init        = custom_init,
    .clone       = custom_clone,
    .release     = custom_release,
    .ssthresh    = custom_ssthresh,
    .cong_avoid  = custom_cong_avoid,
    .name        = "custom_cubic",
    .owner       = THIS_MODULE,
    .ca_priv_size = sizeof(struct custom_cc),
};

// Module load
static int __init custom_register(void)
{
    spin_lock_init(&stats_lock);

    printk(KERN_INFO "Custom TCP CC Loaded\n");
    return tcp_register_congestion_control(&custom_cc);
}

// Module unload
static void __exit custom_unregister(void)
{
    tcp_unregister_congestion_control(&custom_cc);

    printk(KERN_INFO "Custom TCP CC Unloaded\n");
    printk(KERN_INFO "Total Connections Seen: %lld\n",
           atomic64_read(&total_connections));
    printk(KERN_INFO "Total Packets Processed: %lld\n",
           atomic64_read(&total_packets));
}

module_init(custom_register);
module_exit(custom_unregister);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jagannath Srivatsa");
MODULE_DESCRIPTION(" Custom TCP Congestion Control");
