#ifndef _LINUX_CGROUP_NAMESPACE_H
#define _LINUX_CGROUP_NAMESPACE_H

#include <linux/nsproxy.h>
#include <linux/cgroup.h>
#include <linux/types.h>
#include <linux/user_namespace.h>

extern struct cgroup_namespace init_cgroup_ns;

static inline struct cgroup *current_cgroupns_root(void)
{
	return current->nsproxy->cgroup_ns->root_cgrp;
}

extern void free_cgroup_ns(struct cgroup_namespace *ns);

static inline struct cgroup_namespace *get_cgroup_ns(
		struct cgroup_namespace *ns)
{
	if (ns)
		atomic_inc(&ns->count);
	return ns;
}

static inline void put_cgroup_ns(struct cgroup_namespace *ns)
{
	if (ns && atomic_dec_and_test(&ns->count))
		free_cgroup_ns(ns);
}

extern struct cgroup_namespace *copy_cgroup_ns(unsigned long flags,
					       struct user_namespace *user_ns,
					       struct cgroup_namespace *old_ns);

#endif  /* _LINUX_CGROUP_NAMESPACE_H */
