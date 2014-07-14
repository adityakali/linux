/*
 *  Copyright (C) 2014 Google Inc.
 *
 *  Author: Aditya Kali (adityakali@google.com)
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the Free
 *  Software Foundation, version 2 of the License.
 */

#include <linux/cgroup.h>
#include <linux/cgroup_namespace.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/nsproxy.h>
#include <linux/proc_ns.h>

static struct cgroup_namespace *alloc_cgroup_ns(void)
{
	struct cgroup_namespace *new_ns;

	new_ns = kzalloc(sizeof(struct cgroup_namespace), GFP_KERNEL);
	if (new_ns)
		atomic_set(&new_ns->count, 1);
	return new_ns;
}

void free_cgroup_ns(struct cgroup_namespace *ns)
{
	cgroup_put(ns->root_cgrp);
	put_user_ns(ns->user_ns);
	proc_free_inum(ns->proc_inum);
	kfree(ns);
}
EXPORT_SYMBOL(free_cgroup_ns);

struct cgroup_namespace *copy_cgroup_ns(unsigned long flags,
					struct user_namespace *user_ns,
					struct cgroup_namespace *old_ns)
{
	struct cgroup_namespace *new_ns = NULL;
	struct cgroup *cgrp = NULL;
	int err;

	BUG_ON(!old_ns);

	if (!(flags & CLONE_NEWCGROUP))
		return get_cgroup_ns(old_ns);

	/* Allow only sysadmin to create cgroup namespace. */
	err = -EPERM;
	if (!ns_capable(user_ns, CAP_SYS_ADMIN))
		goto err_out;

	/* CGROUPNS only virtualizes the cgroup path on the unified hierarchy.
	 */
	cgrp = get_task_cgroup(current);

	err = -ENOMEM;
	new_ns = alloc_cgroup_ns();
	if (!new_ns)
		goto err_out;

	err = proc_alloc_inum(&new_ns->proc_inum);
	if (err)
		goto err_out;

	new_ns->user_ns = get_user_ns(user_ns);
	new_ns->root_cgrp = cgrp;

	return new_ns;

err_out:
	if (cgrp)
		cgroup_put(cgrp);
	kfree(new_ns);
	return ERR_PTR(err);
}

static int cgroupns_install(struct nsproxy *nsproxy, void *ns)
{
	pr_info("setns not supported for cgroup namespace");
	return -EINVAL;
}

static void *cgroupns_get(struct task_struct *task)
{
	struct cgroup_namespace *ns = NULL;
	struct nsproxy *nsproxy;

	task_lock(task);
	nsproxy = task->nsproxy;
	if (nsproxy) {
		ns = nsproxy->cgroup_ns;
		get_cgroup_ns(ns);
	}
	task_unlock(task);

	return ns;
}

static void cgroupns_put(void *ns)
{
	put_cgroup_ns(ns);
}

static unsigned int cgroupns_inum(void *ns)
{
	struct cgroup_namespace *cgroup_ns = ns;

	return cgroup_ns->proc_inum;
}

const struct proc_ns_operations cgroupns_operations = {
	.name		= "cgroup",
	.type		= CLONE_NEWCGROUP,
	.get		= cgroupns_get,
	.put		= cgroupns_put,
	.install	= cgroupns_install,
	.inum		= cgroupns_inum,
};

static __init int cgroup_namespaces_init(void)
{
	return 0;
}
subsys_initcall(cgroup_namespaces_init);
