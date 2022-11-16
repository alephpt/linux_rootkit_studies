#include <linux/cred.h>
#include <linux/ftrace.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/version.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("P3R5157");
MODULE_DESCRIPTION("PRNG Character Device Hook");
MODULE_VERSION("0.03");


	// // // // // // // //
	//  Type Definitions //
	// // // // // // // //

#if LINUX_VERSION_CODE < KERNEL_VERSION(5,11,0)
#define FTRACE_OPS_FL_RECURSION FTRACE_OPS_FL_RECURSION_SAFE
#define ftrace_regs pt_regs
static __always_inline struct pt_regs *ftrace_get_regs(struct ftrace_regs *fregs) {
	return fregs;
}
#endif

#if defined(CONFIG_X86_64) && (LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0))
#define SYSNAME(name) ("__x64_" name)
#else
#define SYSNAME(name) (name)
#endif

#define HOOK(_handle, _hook, _origin)		\
	{					\
		.name = SYSNAME(_handle),	\
		.function = (_hook),		\
		.original = (_origin)		\
	}

struct hook_t {
	const char *name;
	void *function;
	void *original;
	unsigned long address;
	struct ftrace_ops ops;
};


	// // // // // // // //
	//  Hook Definitions //
	// // // // // // // //

static asmlinkage ssize_t (*real_random_read)(struct file *file, char __user *buf, size_t bytes_n, loff_t *ppos);
static asmlinkage ssize_t (*real_urandom_read)(struct file *file, char __user *buf, size_t bytes_n, loff_t *ppos);

static asmlinkage ssize_t random_read_hook (struct file *file, char __user *buf, size_t bytes_n, loff_t *ppos) {
	int bytes_r = real_random_read(file, buf, bytes_n, ppos);

	printk(KERN_DEBUG "CharDevRK: /dev/random read %d bytes.\n", bytes_r); 
	
	return bytes_r;
}

static asmlinkage ssize_t urandom_read_hook (struct file *file, char __user *buf, size_t bytes_n, loff_t *ppos) {
	int bytes_r = real_urandom_read(file, buf, bytes_n, ppos);

	printk(KERN_DEBUG "CharDevRK: /dev/urandom read %d bytes.\n", bytes_r);

	return bytes_r;
}


	// // // // // // // //
	// Utility Functions //
	// // // // // // // //

static void notrace hook_thunk (unsigned long ip, unsigned long pip,
				struct ftrace_ops *ops,	struct ftrace_regs *fregs) {
	struct pt_regs *regs = ftrace_get_regs(fregs);
	struct hook_t *hook = container_of(ops, struct hook_t, ops);

	if (!within_module(pip, THIS_MODULE)) { regs->ip = (unsigned long) hook->function; } 

	return;
}

void kill_hooks (struct hook_t *hooks, size_t count) {
	for (size_t i = 0; i < count; i++) {
		int unreg = unregister_ftrace_function(&hooks[i].ops);
		int filter = ftrace_set_filter_ip(&hooks[i].ops, hooks[i].address, 1, 0);

		if (unreg) { pr_debug("CharDevRK: kill_hook unregister failed: %d\n", unreg); }
		if (filter) { pr_debug("CharDevRK: kill_hook set filter failed: %d\n", filter); }
	}

	return;
}

int initialize_hooks (struct hook_t *hooks, size_t count) {
	int errors;
	size_t i;

	for (i = 0; i < count; i++) {
		#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,7,0)
		struct kprobe kp = {
			.symbol_name = hooks[i].name
		};

		if (register_kprobe(&kp) < 0) { errors = 0; }

		hooks[i].address = (unsigned long)kp.addr;

		unregister_kprobe(&kp);
		#else
		hooks[i].address = kallsyms_lookup_name(hooks[i].name);
		#endif

		if (!hooks[i].address) {
			pr_debug("CharDevRK: init_hooks %s address failed.\n", hooks[i].name);
			errors = -ENOENT;
			return -ENOENT;
		}

		*((unsigned long*) hooks[i].original) = hooks[i].address;
		hooks[i].ops.func = hook_thunk;
		hooks[i].ops.flags = FTRACE_OPS_FL_SAVE_REGS |
				     FTRACE_OPS_FL_RECURSION |
				     FTRACE_OPS_FL_IPMODIFY;

		errors = ftrace_set_filter_ip(&hooks[i].ops, hooks[i].address, 0, 0);
		if (errors) {
			pr_debug("CharDevRK: init_hooks filter ip failed: %d\n", errors);
			goto kill;
		}

		errors = register_ftrace_function(&hooks[i].ops);

		if (errors) {
			pr_debug("CharDevRK: init_hooks register failed %d\n", errors);
			goto kill;
		}

		return 0;
	}

kill:
	kill_hooks(hooks, i);
	return errors;
}


	
	// // // // // //
	//  Set Hooks  //
	// // // // // //

static struct hook_t hooks[] = {
	HOOK("random_read", random_read_hook, &real_random_read), 
	HOOK("urandom_read", urandom_read_hook, &real_urandom_read),
};

static int __init prng_hook_init (void) {
	int err = initialize_hooks(hooks, ARRAY_SIZE(hooks));

	if (err) { return err; }

	printk(KERN_INFO "CharDevRK: PRNG Hook Initialized.");
	return 0;
}

static void __exit prng_hook_exit (void) {
	kill_hooks(hooks, ARRAY_SIZE(hooks));

	printk(KERN_INFO "CharDevRK: PRNG Hook Uninitialized.");

	return;
}


module_init(prng_hook_init);
module_exit(prng_hook_exit);
