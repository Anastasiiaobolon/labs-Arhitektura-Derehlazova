# Lab 5. Потоки, Race Conditions та Mutex

**Середовище:** GitHub Codespaces, Ubuntu, GCC, POSIX threads. Усі програми компілюються з прапорцем **`-pthread`**:
```bash
gcc -Wall -Wextra -pthread program.c -o program
```
Executable-файли в репозиторій не додаються.

> **Важливо про race condition.** Помилка гонки проявляється лише тоді, коли два threads справді виконуються одночасно на різних ядрах. Перевірте кількість ядер командою `nproc`. Якщо в Codespace лише 1 ядро, `race` може кілька разів дати правильний результат. Це не означає, що проблеми немає (див. останнє контрольне питання).

## Файли

`first_thread.c`, `multiple_threads.c`, `shared_data.c`, `thread_stacks.c`, `race.c`, `mutex_counter.c`, `race_four.c`, `mutex_four.c`, `shared_heap.c`, `thread_result.c`, `deadlock.c`, `parallel_sum.c`, а також додатковий `sequential_sum.c`.

## Завдання 1. Перший thread (`first_thread.c`)

```text
Main: creating thread
Hello from worker thread!
Main: worker finished
```
`pthread_create()` створює новий thread, який виконує функцію `worker`. `pthread_join()` змушує `main` дочекатися завершення thread. Без `join` програма могла б завершитись раніше, ніж thread виведе повідомлення.

## Завдання 2. Декілька threads (`multiple_threads.c`)

```bash
./multiple_threads
```
```text
Main: creating thread
Hello from worker thread!
Main: worker finished
Thread 1 is running
Thread 3 is running
Thread 2 is running
Thread 4 is running
All threads finished
Thread 1 is running
Thread 4 is running
Thread 2 is running
Thread 3 is running
All threads finished
Thread 2 is running
Thread 1 is running
Thread 3 is running
Thread 4 is running
All threads finished
```
Порядок `Thread 1..4 is running` **може бути різним** при різних запусках: порядок `pthread_create()` не гарантує порядок виконання. Це визначає планувальник ОС.

## Завдання 3–4. Shared global та local stack (`shared_data.c`, `thread_stacks.c`)

```text
Thread 1: value=100, address=0x651934ac0014
Thread 2: value=200, address=0x651934ac0014
Main: value=200, address=0x651934ac0014

Thread 1: local=10, address=0x70819a928eb0
Thread 2: local=20, address=0x70819a127eb0
```

| | Адреса | Спільна для threads? |
|---|---|---|
| глобальна змінна `value` | однакова в Thread 1, Thread 2 та `main` | **так**, одна змінна |
| локальна змінна `local` | **різна** в кожного thread | **ні**, у кожного thread власний stack |

Threads одного процесу **ділять** код, глобальні змінні (data/bss) та heap, але мають **власний stack** (і власні регістри/лічильник команд). Тому остання зміна `value` (`200`) видна в `main`, а локальні змінні різних threads розташовані за різними адресами.

## Завдання 5–6. Race condition (`race.c`)

```bash
gcc -Wall -Wextra -pthread race.c -o race
./race    # п'ять запусків
```

| Запуск | Expected | Actual |
|---|---|---|
| 1 | 2000000 | `1805939` |
| 2 | 2000000 | `1836644` |
| 3 | 2000000 | `1595738` |
| 4 | 2000000 | `1544604` |
| 5 | 2000000 | `1577222` |

Очікується, що `Actual` менший за `Expected` і **різний** у різних запусках.

**Чому `counter++` створює проблему? (lost update)** `counter++` не атомарна операція, вона складається з трьох кроків: прочитати `counter` з пам'яті в регістр, збільшити регістр на 1, записати регістр назад у пам'ять. Якщо два threads одночасно прочитають одне значення (наприклад, 100), обидва запишуть 101, і одне збільшення **втрачається** (lost update).

### Machine instructions для `counter++`

```bash
gcc -Wall -Wextra -pthread -O0 -S -masm=intel race.c -o race.s
grep -n -B2 -A4 "counter" race.s
```
```text
mov     rax, QWORD PTR counter[rip]
add     rax, 1
mov     QWORD PTR counter[rip], rax
```
Типово видно послідовність `mov rax, QWORD PTR counter[rip]` → `add rax, 1` → `mov QWORD PTR counter[rip], rax`. Між першою і третьою інструкціями планувальник може перемкнути thread, і виникає race condition.

**Критична секція** це ділянка коду, яка працює зі спільним ресурсом (`counter++`) і не повинна виконуватись одночасно кількома threads.

## Завдання 8. Mutex (`mutex_counter.c`)

```bash
./mutex_counter    # п'ять запусків
```

| Запуск | Expected | Actual |
|---|---|---|
| 1–5 | 2000000 | `2000000 у кожному запуску` |

**Чому mutex виправив проблему?** `pthread_mutex_lock()` пускає в критичну секцію лише один thread; інші чекають. Тому послідовність «прочитати → збільшити → записати» виконується без втручання іншого thread. Це **mutual exclusion**.

## Завдання 9. Де ставити lock

Варіант 1: `lock`/`unlock` навколо кожного `counter++` (дрібна гранулярність, багато блокувань, але threads чергуються). Варіант 2: один `lock` навколо **всього циклу**, коли один thread тримає mutex весь час, а інший чекає, доки перший завершить 1 000 000 збільшень. Він теж коректний, але **concurrency втрачається**: threads фактично виконуються по черзі. Висновок: critical section має бути достатньою для коректності, але не надмірно великою.

## Завдання 10. Вартість синхронізації

```bash
time ./race
time ./mutex_counter
```

| Program | Run 1 | Run 2 | Run 3 |
|---|---|---|---|
| race | `0m0.010s` | `0m0.009s` | `0m0.002s` |
| mutex_counter | `0m0.085s` | `0m0.089s` | `0m0.055s` |

**Чому synchronized версія повільніша?** Кожен `lock`/`unlock` має накладні витрати (overhead), а коли threads змагаються за mutex, вони блокуються і чекають (**lock contention**), тому виконання частково послідовне. Це не точний benchmark, лише ілюстрація: коректність має ціну.

## Завдання 11. Більше threads (`race_four.c`, `mutex_four.c`)

4 threads × 500000 increments = 2 000 000.

```bash
./race_four          # п'ять запусків
./mutex_four
```
```text
Expected: 2000000
Actual:   678922
Expected: 2000000
Actual:   789358
Expected: 2000000
Actual:   519025
Expected: 2000000
Actual:   1577406
Expected: 2000000
Actual:   1031506

Expected: 2000000
Actual:   2000000
```

| Варіант | Результат |
|---|---|
| 2 threads без mutex | неправильний, різний |
| 4 threads без mutex | неправильний, зазвичай ще більше втрат |
| 2 threads з mutex | правильний (2000000) |
| 4 threads з mutex | правильний (2000000), але зазвичай повільніше (більше contention) |

**Чи означає більше threads швидшу програму?** Ні. Якщо threads змагаються за один спільний ресурс, вони чекають одне на одного; додаються витрати на створення threads та перемикання контексту.

## Завдання 12. Shared heap (`shared_heap.c`)

```text
Worker: value=110 address=0x63ef849d12a0
Main: value=110 address=0x63ef849d12a0
```
Типово: `Worker: value=110 address=0x...` та `Main: value=110 address=0x...` з **тією самою адресою**. Heap спільний для threads: thread змінив структуру, і `main` бачить цю зміну.

## Завдання 13. Значення, що повертає thread (`thread_result.c`)

`Result = 144`. Thread виділяє результат у heap (`malloc`) і повертає вказівник; `pthread_join(thread, &thread_result)` передає його в `main`, який потім робить `free`. Не можна повертати вказівник на локальну змінну thread, бо його stack зникне після завершення.

## Завдання 14–16. Deadlock (`deadlock.c`)

**Аналіз до запуску:**
```text
Thread A                  Thread B
lock mutex1               lock mutex2
sleep                     sleep
lock mutex2  ← чекає      lock mutex1  ← чекає
        ↓                      ↓
              DEADLOCK
```
Thread A тримає `mutex1` і чекає `mutex2`; Thread B тримає `mutex2` і чекає `mutex1`. Обидва чекають одне одного вічно.

```bash
gcc -Wall -Wextra -pthread deadlock.c -o deadlock
timeout 5 ./deadlock; echo "exit code: $?"
```
```text
Thread 1: locking mutex1
Thread 2: locking mutex2
Thread 1: locking mutex2
Thread 2: locking mutex1
(далі програма зависає; timeout завершує її з кодом 124)
```

**Схема deadlock (circular wait):**
```text
Thread 1 ──owns──► mutex1 ◄──waits── Thread 2
Thread 1 ──waits─► mutex2 ◄──owns─── Thread 2
Thread 1 → mutex2 → Thread 2 → mutex1 → Thread 1
```

**Виправлення:** обидва threads беруть mutex в **однаковому порядку** (`mutex1`, потім `mutex2`). Виправлена версія зібрана з макросом `-DFIXED`:
```bash
gcc -Wall -Wextra -pthread -DFIXED deadlock.c -o deadlock_fixed
./deadlock_fixed
```
```text
Thread 1: locking mutex2
Thread 1: critical section
Thread 2: locking mutex2
Thread 2: critical section
Both threads finished
```
Усунено умову **circular wait**: коли порядок блокувань однаковий, цикл очікування неможливий.

## Завдання 17. Race condition та Deadlock

| Характеристика | Race Condition | Deadlock |
|---|---|---|
| Threads продовжують execution? | так | ні (зупиняються назавжди) |
| Результат може бути неправильним? | так | немає результату (програма зависає) |
| Програма може повністю перестати прогресувати? | ні, зазвичай | так |
| Пов'язано з timing/order? | так | так (залежить від чергування) |
| Mutex може допомогти? | так | сам по собі ні, потрібен правильний порядок |
| Неправильне використання mutex може бути причиною? | так (забули lock) | так (різний порядок lock) |

Без синхронізації виникає race condition, при неправильній синхронізації виникає deadlock, тому синхронізацію треба проєктувати правильно.

## Фінальне завдання. `parallel_sum.c`

```bash
gcc -Wall -Wextra -pthread parallel_sum.c -o parallel_sum && ./parallel_sum
```
```text
Thread 1: range 1..250000, partial sum = 31250125000
Thread 2: range 250001..500000, partial sum = 93750125000
Thread 3: range 500001..750000, partial sum = 156250125000
Thread 4: range 750001..1000000, partial sum = 218750125000
Parallel result: 500000500000
Expected result: 500000500000
Correct: YES
```
(Результат перевіряється формулою N(N+1)/2 = 500000500000.)

**Аналіз:**
1. **Shared data:** масив `Task tasks[]` (доступний `main` і worker threads), але кожен thread працює лише зі **своїм** елементом.
2. **Thread-local:** змінна `sum` усередині `calculate_sum()` (на stack кожного thread).
3. **Чому не потрібен mutex у циклі:** кожен thread змінює лише власний локальний акумулятор і записує результат у власний `Task`, тому спільних змінних, що змінюються, немає.
4. **Коли `main` читає результати:** після `pthread_join()`, коли thread гарантовано завершився.
5. **Чому краще за глобальний лічильник із lock на кожній ітерації:** значно менше shared mutable state та synchronization overhead. Найкраще вирішення race condition часто полягає не в додаванні locks, а в зменшенні спільного змінюваного стану.

## Додаткове завдання: sequential та parallel

```bash
gcc -Wall -Wextra sequential_sum.c -o sequential_sum
time ./sequential_sum
time ./parallel_sum
```
```text
Expected result:   500000500000

real    0m0.011s
user    0m0.004s
sys     0m0.001s
Thread 1: range 1..250000, partial sum = 31250125000
Thread 2: range 250001..500000, partial sum = 93750125000
Thread 3: range 500001..750000, partial sum = 156250125000
Thread 4: range 750001..1000000, partial sum = 218750125000
Parallel result: 500000500000
Expected result: 500000500000
Correct: YES

real    0m0.003s
user    0m0.003s
sys     0m0.001s
```
Для такої малої задачі (1 000 000 додавань) паралельна версія не обов'язково швидша: створення threads має overhead, який порівняний із самою роботою. Parallelization має сенс, коли обчислення **достатньо великі**, частини **незалежні** (мало спільного змінюваного стану) і є кілька вільних ядер CPU.

## Відповіді на контрольні питання

- **Thread:** найменша одиниця виконання всередині процесу.
- **Thread та process:** процес має власний адресний простір, а threads одного процесу ділять його.
- **Що ділять threads:** код, глобальні дані, heap, відкриті файли.
- **Що власне:** stack, регістри, лічильник команд.
- **Власний stack:** так. **Спільний heap:** так.
- **`pthread_create()`:** створює і запускає новий thread.
- **`pthread_join()`:** чекає завершення thread і отримує його результат.
- **Порядок виконання:** не гарантується порядком `pthread_create()`.
- **Concurrency та parallelism:** concurrency це кілька задач у процесі виконання з перемиканням, parallelism це реально одночасне виконання на різних ядрах.
- **Shared state:** дані, доступні кільком threads.
- **Race condition:** результат залежить від порядку виконання threads, що працюють зі спільними даними без синхронізації.
- **Lost update:** одне з оновлень перезаписується іншим і втрачається.
- **Чому `counter++` не atomic:** складається з трьох кроків (read, modify, write).
- **Critical section:** ділянка коду, що працює зі спільним ресурсом.
- **Mutual exclusion:** лише один thread у critical section одночасно.
- **Mutex:** об'єкт синхронізації для взаємного виключення.
- **`pthread_mutex_lock()`:** захоплює mutex (або чекає, доки він звільниться).
- **Якщо mutex у іншого thread:** thread блокується й чекає.
- **Навіщо `unlock`:** звільнити mutex, щоб інші threads могли продовжити; без нього решта чекатиме вічно.
- **Надто велика critical section:** threads виконуються по черзі, і concurrency зникає.
- **Lock contention:** багато threads змагаються за один lock, і витрачають час на очікування.
- **Більше threads ≠ швидше:** overhead, contention, обмежена кількість ядер.
- **Deadlock:** threads чекають одне на одного нескінченно.
- **Deadlock та race condition:** при race результат неправильний, але програма працює, при deadlock програма зупиняється.
- **Circular wait:** замкнений ланцюжок очікування (A чекає B, B чекає A).
- **Однаковий порядок locks:** усуває circular wait.
- **Starvation:** thread постійно не отримує ресурс (CPU або lock), бо його обходять інші.
- **Local accumulation краще:** немає спільних змінних у циклі, не потрібна синхронізація на кожній ітерації.
- **Чому успішні 100 запусків не доводять відсутності race condition?** Race залежить від випадкового планування, воно може проявитися рідко. Правильний результат у багатьох запусках лише означає, що поганий порядок не трапився, а не що його не існує. Коректність доводять аналізом і синхронізацією, а не кількістю успішних запусків.

## Висновок

Підтверджено, що threads діляють глобальні дані та heap, але мають власні stack. `counter++` не атомарний, тому без синхронізації виникає lost update. Mutex відновлює коректність ціною overhead. Неправильний порядок блокувань призводить до deadlock (circular wait), а однаковий порядок захоплення locks його усуває. Найкращий дизайн, як у `parallel_sum`, мінімізує спільний змінюваний стан.
