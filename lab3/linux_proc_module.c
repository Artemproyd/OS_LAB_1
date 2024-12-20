#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/version.h>

#define PROCFS_NAME "tsu"

static struct proc_dir_entry *our_proc_file = NULL;

// Статические переменные для хранения текущих чисел Фибоначчи
static unsigned long fib1 = 0, fib2 = 1;

// Функция чтения из файла /proc/tsu
static ssize_t procfile_read(struct file *file_pointer, char __user *buffer,
                             size_t buffer_length, loff_t *offset) {
    char output[64];
    size_t len;

    // Если смещение больше 0, завершаем чтение (поддержка single-read)
    if (*offset > 0)
        return 0;

    // Формируем строку с текущими числами Фибоначчи
    len = snprintf(output, sizeof(output), "%lu %lu\n", fib1, fib2);

    // Копируем данные в пользовательский буфер
    if (copy_to_user(buffer, output, len)) {
        return -EFAULT;
    }


    *offset += len; // Обновляем смещение

    pr_info("procfile_read: /proc/%s read, Fibonacci: %lu %lu\n", PROCFS_NAME, fib1, fib2);

    fib1 += fib2;
    fib2 += fib1;
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
MODULE_DESCRIPTION("Fibonacci Sequence Linux Kernel Module with /proc interface");


/*

make
sudo insmod linux_proc_cat
/proc/tsu
sudo rmmod linux_proc_module
sudo dmesg | grep "/proc/tsu"
make clean

*/
