#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

typedef struct {
    pthread_mutex_t mutex; // Мьютекс для синхронизации
    pthread_cond_t cond; // Условная переменная для ожидания/уведомления
    int ret;   // Флаг завершения
    int data;  // Последнее переданное число
    int num_numbers;  // Количество чисел, которые поставщик должен передать
} SharedResource;

// Функция поставщика
void* supplier(void* arg) {
    SharedResource* resource = (SharedResource*)arg;

    srand(time(NULL));

    for(int i = 0; i < resource->num_numbers; i++) {
        sleep(1); // Пауза между отправками

        pthread_mutex_lock(&resource->mutex);

        if (resource->ret) { // Если предыдущее число ещё не обработано
            printf("Sender: Error! Previous number is not processed yet.\n");
        } else {
        	resource->data = rand() % 111 + 1;
			printf("Supplier sent: %d\n", resource->data);
            resource->ret = 1;       // Устанавливаем флаг готовности
            pthread_cond_signal(&resource->cond); // Уведомляем поток-получатель
        }

        pthread_mutex_unlock(&resource->mutex);

    }

    return NULL;
}

// Функция потребителя
void* consumer(void* arg) {
    SharedResource* resource = (SharedResource*)arg;
    for(int i = 0; i < resource->num_numbers; ++i) {
        // Блокировка мьютекса
        pthread_mutex_lock(&resource->mutex);

        // Ожидание уведомления о новых данных
        while (!resource->ret) {
            pthread_cond_wait(&resource->cond, &resource->mutex);
        }

        // Проверяем сигнал завершения
        if (resource->ret == -1) {
            pthread_mutex_unlock(&resource->mutex);
            break;
        }

        // Обработка данных
        printf("Consumer received: %d\n", resource->data);

        // Сбрасываем флаг готовности
        resource->ret = 0;

        // Освобождение мьютекса
        pthread_mutex_unlock(&resource->mutex);
    }
    return NULL;
}

int main() {
    pthread_t supplier_thread, consumer_thread;
    SharedResource resource = {PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, 0, 0, 0};

    int num_numbers;

    printf("Enter the number of numbers to send: ");
    scanf("%d", &num_numbers);

    if (num_numbers <= 0) {
        printf("Invalid number. Exiting...\n");
        return -1;
    }

    resource.num_numbers = num_numbers;

    // Создание потока поставщика
    pthread_create(&supplier_thread, NULL, supplier, (void*)&resource);

    // Создание потока потребителя
    pthread_create(&consumer_thread, NULL, consumer, (void*)&resource);

    // Ожидание завершения потоков
    pthread_join(supplier_thread, NULL);
    pthread_join(consumer_thread, NULL);

    printf("All numbers have been processed. Press Enter to exit.\n");
    getchar();

    return 0;
}
