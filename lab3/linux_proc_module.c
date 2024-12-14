#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/version.h>

#define PROCFS_NAME "tsu"

static struct proc_dir_entry *our_proc_file = NULL;

// Функция чтения из файла /proc/tsu
static ssize_t procfile_read(struct file *file_pointer, char __user *buffer,
                             size_t buffer_length, loff_t *offset) {
    char s[] = "Tomsk\n";
    size_t len = sizeof(s) - 1;

    // Если смещение больше длины строки, завершаем чтение
    if (*offset >= len)
        return 0;

    // Убедимся, что пользовательский буфер достаточно большой
    if (buffer_length < len)
        return -EINVAL;

    // Копируем данные в пользовательский буфер
    if (copy_to_user(buffer, s, len)) {
        return -EFAULT;
    }

    *offset += len; // Обновляем смещение

    pr_info("procfile_read: /proc/%s read\n", PROCFS_NAME);
    return len;
}

// Указатели на функции для файла /proc/tsu
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
static const struct proc_ops proc_file_fops = {
    .proc_read = procfile_read,
};
#else
static const struct file_operations proc_file_fops = {
    .read = procfile_read,
};
#endif

// Функция инициализации модуля
static int __init procfs1_init(void) {
    our_proc_file = proc_create(PROCFS_NAME, 0644, NULL, &proc_file_fops);
    if (our_proc_file == NULL) {
        pr_err("Error: Could not initialize /proc/%s\n", PROCFS_NAME);
        return -ENOMEM;
    }

    pr_info("/proc/%s created\n", PROCFS_NAME);
    return 0;
}

// Функция очистки модуля
static void __exit procfs1_exit(void) {
    proc_remove(our_proc_file);
    pr_info("/proc/%s removed\n", PROCFS_NAME);
}

module_init(procfs1_init);
module_exit(procfs1_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Simple Linux Kernel Module with /proc interface");

