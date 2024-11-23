#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int data;  // Последнее переданное число
    int ready;  // Флаг, показывающий, что данные готовы для обработки
    int num_numbers;  // Количество чисел, которые поставщик должен передать
} SharedResource;

// Функция поставщика
void* supplier(void* arg) {
    SharedResource* resource = (SharedResource*)arg;

    srand(time(NULL));  

    for (int i = 0; i < resource->num_numbers; i++) {
        sleep(1); // Пауза между отправками

        // Генерация случайного числа
        resource->data = rand() % 100;  // Генерация числа от 0 до 99

        // Блокировка мьютекса
        pthread_mutex_lock(&resource->mutex);

        // Уведомление потребителя о новых данных
        resource->ready = 1;
        pthread_cond_signal(&resource->cond);

        // Освобождение мьютекса
        pthread_mutex_unlock(&resource->mutex);

        printf("Supplier sent: %d\n", resource->data);
    }

    // Завершаем работу потока поставщика
    return NULL;
}

// Функция потребителя
void* consumer(void* arg) {
    SharedResource* resource = (SharedResource*)arg;
    for (int i = 0; i < resource->num_numbers; ++i) {
        // Блокировка мьютекса
        pthread_mutex_lock(&resource->mutex);

        // Ожидание уведомления о новых данных
        while (resource->ready == 0) {
            pthread_cond_wait(&resource->cond, &resource->mutex);
        }

        // Обработка данных
        printf("Consumer received: %d\n", resource->data);

        // Сброс флага
        resource->ready = 0;

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
