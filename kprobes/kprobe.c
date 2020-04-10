#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <net/icmp.h>
#include <linux/skbuff.h>

#define MAX_SYMBOL_LEN	64
static char sym_name[MAX_SYMBOL_LEN] = "icmp_rcv";
struct kprobe icmp_kprobe = {
	.symbol_name = sym_name,
	.offset = 0xc5,
};

static void icmp_send_post(struct kprobe *p, struct pt_regs *regs, unsigned long flags)
{
	struct icmphdr *icmph;
	struct sk_buff *skb = (struct sk_buff *)regs->di;

	pr_info("<%s> pre_handler: p->addr = 0x%p, ip = %lx, flags = 0x%lx\n",
			p->symbol_name, p->addr, regs->ip, regs->flags);

	icmph = icmp_hdr(skb);
	if (icmph->type > NR_ICMP_TYPES)
		pr_err("Unknown ICMP message\n");
	else
		pr_info("ICMP_type = 0x%x\n", icmph->type && 0xff);
}

static int __init kprobe_init(void)
{
	int ret;

	icmp_kprobe.post_handler = icmp_send_post;

	ret = register_kprobe(&icmp_kprobe);
	if (ret < 0) {
		pr_err("register_kprobe failed, returned %d\n", ret);
		return -1;
	}

	pr_debug("Planted kprobe at %s: %p\n", icmp_kprobe.symbol_name,
			 icmp_kprobe.addr);

	return 0;
}

static void __exit kprobe_exit(void)
{
	unregister_kprobe(&icmp_kprobe);
	pr_info("kprobe at %p unregistered\n", icmp_kprobe.addr);
}

module_init(kprobe_init);
module_exit(kprobe_exit);

MODULE_LICENSE("GPL");
