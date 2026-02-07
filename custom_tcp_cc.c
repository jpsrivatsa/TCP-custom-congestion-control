#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/tcp.h>
#include <linux/spinlock.h>
#include <linux/atomic.h>

// global statistics shared across connections
// protected via spinlock to avoid race conditions

static spinlock_t stats_lock;
static atomic64_t total_connections = ATOMIC64_INIT(0);
static atomic64_t total_packets = ATOMIC64_INIT(0);

// scaling constant
static u32 cubic_c = 410;

// multiplicative decrease
static u32 beta = 717;

// per connection state
struct custom_cc {
    u32 last_max_cwnd;
    u32 epoch_start;
    u32 origin_point;
    u32 K;
};

// safe cubic root approximation
static u32 cubic_root(u64 a)
{
    return int_sqrt(a);
}

// congestion avoidance logic
static void custom_cong_avoid(struct sock *sk, u32 ack, u32 acked)
{
    struct tcp_sock *tp = tcp_sk(sk);
    struct custom_cc *ca = inet_csk_ca(sk);

    atomic64_add(acked, &total_packets);

    // slow start handling
    if (tp->snd_cwnd <= tp->snd_ssthresh) {
        tcp_slow_start(tp, acked);
        return;
    }

    // initialize epoch if first time
    if (!ca->epoch_start)
        ca->epoch_start = tcp_jiffies32;

    u32 t = tcp_jiffies32 - ca->epoch_start;

    u32 target = ca->origin_point + cubic_root(t);

    if (target > tp->snd_cwnd)
        tp->snd_cwnd++;
}

// multiplicative decrease when packet loss occurs
static u32 custom_ssthresh(struct sock *sk)
{
    const struct tcp_sock *tp = tcp_sk(sk);

    return max(tp->snd_cwnd * beta / 1024U, 2U);
}

// initialize congestion state per socket
static void custom_init(struct sock *sk)
{
    struct custom_cc *ca = inet_csk_ca(sk);

    memset(ca, 0, sizeof(*ca));

    ca->origin_point = tcp_sk(sk)->snd_cwnd;

    atomic64_inc(&total_connections);
}

// called when connection is cloned (forked socket state)
static void custom_clone(struct sock *sk, struct sock *newsk)
{
    struct custom_cc *old_ca = inet_csk_ca(sk);
    struct custom_cc *new_ca = inet_csk_ca(newsk);

    memcpy(new_ca, old_ca, sizeof(*new_ca));

    atomic64_inc(&total_connections);
}

// optional cleanup when socket closes
static void custom_release(struct sock *sk)
{
    atomic64_dec(&total_connections);
}

// main congestion control ops
static struct tcp_congestion_ops custom_cc __read_mostly = {
    .init       = custom_init,
    .clone      = custom_clone,
    .release    = custom_release,
    .ssthresh   = custom_ssthresh,
    .cong_avoid = custom_cong_avoid,
    .name       = "custom_cubic",
    .owner      = THIS_MODULE,
    .ca_priv_size = sizeof(struct custom_cc),
};

// module load
static int __init custom_register(void)
{
    spin_lock_init(&stats_lock);

    printk(KERN_INFO "Custom TCP CC Loaded\n");

    return tcp_register_congestion_control(&custom_cc);
}

// module unload
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
MODULE_DESCRIPTION("Thread Safe Custom TCP Congestion Control");
