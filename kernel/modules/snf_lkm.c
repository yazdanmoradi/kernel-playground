/*
 * M12 — MAC Address Filter
 * Basic level: log the source MAC address of incoming frames.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/netfilter_ipv6.h>

#include <linux/skbuff.h>
#include <linux/if_ether.h>
#include <linux/netdevice.h>
#include <linux/atomic.h>

static atomic64_t packet_counter = ATOMIC64_INIT(0);

static unsigned int mac_filter_hook(void *priv,
                                    struct sk_buff *skb,
                                    const struct nf_hook_state *state)
{
    struct ethhdr *eth;
    unsigned long long count;

    if (!skb)
        return NF_ACCEPT;

    if (!skb_mac_header_was_set(skb))
        return NF_ACCEPT;

    eth = eth_hdr(skb);
    if (!eth)
        return NF_ACCEPT;

    count = atomic64_inc_return(&packet_counter);

    pr_info("[M12-basic] packet=%llu interface=%s src_mac=%pM dst_mac=%pM\n",
            count,
            state->in ? state->in->name : "unknown",
            eth->h_source,
            eth->h_dest);

    return NF_ACCEPT;
}

static struct nf_hook_ops nfho_ipv4 = {
    .hook     = mac_filter_hook,
    .hooknum  = NF_INET_PRE_ROUTING,
    .pf       = PF_INET,
    .priority = NF_IP_PRI_FIRST,
};

static struct nf_hook_ops nfho_ipv6 = {
    .hook     = mac_filter_hook,
    .hooknum  = NF_INET_PRE_ROUTING,
    .pf       = PF_INET6,
    .priority = NF_IP6_PRI_FIRST,
};

static int __init m12_init(void)
{
    int ret;

    pr_info("[M12-basic] MAC Address Filter module loading\n");

    ret = nf_register_net_hook(&init_net, &nfho_ipv4);
    if (ret) {
        pr_err("[M12-basic] failed to register IPv4 hook\n");
        return ret;
    }

    ret = nf_register_net_hook(&init_net, &nfho_ipv6);
    if (ret) {
        pr_err("[M12-basic] failed to register IPv6 hook\n");
        nf_unregister_net_hook(&init_net, &nfho_ipv4);
        return ret;
    }

    pr_info("[M12-basic] IPv4 and IPv6 hooks registered successfully\n");

    return 0;
}

static void __exit m12_exit(void)
{
    nf_unregister_net_hook(&init_net, &nfho_ipv4);
    nf_unregister_net_hook(&init_net, &nfho_ipv6);

    pr_info("[M12-basic] module unloaded, total packets logged=%lld\n",
            atomic64_read(&packet_counter));
}

module_init(m12_init);
module_exit(m12_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yazdan Moradi");
MODULE_DESCRIPTION("M12 MAC Address Filter - Basic: log source MAC address of incoming frames");
MODULE_VERSION("1.0");
