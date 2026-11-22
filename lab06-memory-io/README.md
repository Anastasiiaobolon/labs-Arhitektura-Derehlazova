# Lab 6. Віртуальна пам'ять, File Descriptors та I/O

**Середовище:** GitHub Codespaces, Ubuntu, GCC, `/proc`, `strace`.
Executable-файли, згенеровані тестові файли (`input.txt`, `output.txt`, `example.txt`, `destination.txt` тощо) та core dumps в репозиторій не додаються.

## Файли

`memory_layout.c`, `memory_maps.c`, `page_size.c`, `heap_growth.c`, `invalid_memory.c`, `standard_fds.c`, `fd_observer.c`, `open_file.c`, `write_file.c`, `read_file.c`, `file_offset.c`, `streams.c`, `buffering.c`, `file_copy.c`.

Тестовий файл для `read_file` та `file_offset` створюється командою:
```bash
printf 'Hello from input file\nSecond line\n' > input.txt
```

# Частина I. Віртуальна пам'ять

## Завдання 1. Адреси (`memory_layout.c`)

```bash
gcc -Wall -Wextra memory_layout.c -o memory_layout && ./memory_layout
```
```text
function:             0x5a93e058d1c9
global initialized:   0x5a93e0590010
global uninitialized: 0x5a93e059001c
static:               0x5a93e0590014
heap:                 0x5a94178ad2a0
stack:                0x7fffbe39d73c
```

| Об'єкт | Адреса | Область |
|---|---|---|
| function | `0x5a93e058d1c9` | code (.text) |
| global initialized | `0x5a93e0590010` | .data |
| global uninitialized | `0x5a93e059001c` | .bss |
| static | `0x5a93e0590014` | .data |
| heap (`malloc`) | `0x5a94178ad2a0` | heap |
| stack (`local`) | `0x7fffbe39d73c` | stack |

Адреси code/data/bss близькі між собою, heap окремо, а stack (`0x7ff...`) у верхній частині адресного простору. Адреси відрізняються при кожному запуску (ASLR).

## Завдання 2. Memory map процесу (`memory_maps.c`)

```bash
gcc -Wall -Wextra memory_maps.c -o memory_maps
./memory_maps &            # запам'ятайте PID, який виведе програма
cat /proc/<PID>/maps
```
```text
PID:    14087
main:   0x64802af80229
global: 0x64802af83010
heap:   0x64803722b2a0
stack:  0x7ffede66957c

653bd244c000-653bd244d000 r-xp 00001000 07:04 1442179                    /workspaces/labs-Arhitektura-Derehlazova/lab06-memory-io/memory_maps
653bd244f000-653bd2450000 rw-p 00003000 07:04 1442179                    /workspaces/labs-Arhitektura-Derehlazova/lab06-memory-io/memory_maps
653bf7075000-653bf7096000 rw-p 00000000 00:00 0                          [heap]
75156bbe5000-75156bbe8000 rw-p 00000000 00:00 0 
75156bc10000-75156bd99000 r-xp 00028000 00:2d 787320                     /usr/lib/x86_64-linux-gnu/libc.so.6
75156bdec000-75156bdee000 rw-p 00203000 00:2d 787320                     /usr/lib/x86_64-linux-gnu/libc.so.6
75156bdee000-75156bdfb000 rw-p 00000000 00:00 0 
75156be02000-75156be04000 rw-p 00000000 00:00 0 
75156be05000-75156be30000 r-xp 00001000 00:2d 787300                     /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
75156be3c000-75156be3e000 rw-p 00038000 00:2d 787300                     /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
7ffc966de000-7ffc96700000 rw-p 00000000 00:00 0                          [stack]
7ffc96746000-7ffc96748000 r-xp 00000000 00:00 0                          [vdso]
```

Зіставлення (заповніть своїми адресами):

| Адреса з програми | Потрапляє в діапазон `/proc/PID/maps` | Mapping |
|---|---|---|
| `main` = `0x...` | `0x...-0x...` | executable, `r-xp` (код) |
| `global` = `0x...` | `0x...-0x...` | executable, `rw-p` (дані) |
| `heap` = `0x...` | `0x...-0x...` | `[heap]` |
| `stack` = `0x7ff...` | `0x...-0x...` | `[stack]` |

У `/proc/PID/maps` також видно mappings shared libraries (наприклад, `libc.so.6`).

**Як читати range:** `55a10000-55a31000` означає початкову та кінцеву адреси області (кінець не включається). Адреса heap-алокації лежить усередині діапазону `[heap]`, а адреса локальної змінної всередині `[stack]`. (Невеликі `malloc` потрапляють у `[heap]`.)

**Permissions:** `r` читання, `w` запис, `x` виконання, `p` private mapping.
- Code не обов'язково `writable`: програма не має змінювати власні інструкції; захист від запису ускладнює пошкодження коду та атаки.
- Data не обов'язково `executable`: дані не мають виконуватися як код (захист від виконання даних, наприклад NX), що ускладнює експлуатацію помилок.
Це **memory protection**: ОС і апаратура (MMU) контролюють дозволені операції над кожною областю.

## Завдання 3. Page size (`page_size.c`)

```text
Page size: 4096 bytes
```
(перевірте у себе: `4096 bytes`). Адресу можна розкласти на **номер сторінки** та **зсув (offset)**: `page_number = address / 4096`, `offset = address % 4096` (молодші 12 біт адреси). Приклад: адреса `0x7ffdb096071c` має offset `0x71c`.

## Завдання 4. Heap та virtual memory (`heap_growth.c`)

```bash
gcc -Wall -Wextra heap_growth.c -o heap_growth
./heap_growth          # у другому терміналі: grep -E "VmSize|VmRSS" /proc/<PID>/status
```

| Момент | VmSize | VmRSS |
|---|---|---|
| після `malloc(100 MB)`, до торкання | `105092 kB` | `1684 kB` |
| після запису по pages | `105092 kB` | `104092 kB` |

Очікується: `VmSize` зростає одразу приблизно на 100 МБ (віртуальний адресний простір зарезервовано), а `VmRSS` (фізична пам'ять) після `malloc` майже не змінюється і зростає на ~100 МБ лише після запису по сторінках.

**Висновок:** `malloc(100 MB)` це не негайне виділення 100 МБ фізичної RAM. Віртуальна пам'ять розділяє резервування адресного простору та фактичне використання фізичної пам'яті (сторінки виділяються при першому зверненні, page fault).

## Завдання 5–6. Недійсний доступ до пам'яті (`invalid_memory.c`)

```bash
gcc -Wall -Wextra invalid_memory.c -o invalid_memory
./invalid_memory ; echo "exit code: $?"
strace ./invalid_memory 2>&1 | tail -n 5
```
```text
About to access invalid memory
Segmentation fault
exit code: 139
--- SIGSEGV {si_signo=SIGSEGV, si_code=SEGV_MAPERR, si_addr=NULL} ---
+++ killed by SIGSEGV (core dumped) +++
```
Програма звертається до адреси 0 (`NULL`), яка не відображена в її віртуальному адресному просторі. MMU фіксує помилку доступу (fault), ядро надсилає процесу сигнал `SIGSEGV`, і процес завершується (код 139 = 128 + 11).

- **Чому ОС не дозволяє писати за адресою 0?** Ця сторінка не відображена, а доступ до неї майже завжди помилка (розіменування `NULL`).
- **Як це захищає систему?** Некоректний доступ не перетворюється на запис у чужу пам'ять: ушкодження обмежується процесом-порушником.
- **Чи Segmentation fault означає зламану RAM?** Ні, це програмна помилка (неправильна адреса або права доступу), а не апаратна несправність.

# Частина II. File Descriptors та I/O

## Завдання 7. Стандартні дескриптори (`standard_fds.c`)

```text
STDIN  = 0
STDOUT = 1
STDERR = 2
```
**0** це stdin (введення), **1** це stdout (звичайний вивід), **2** це stderr (повідомлення про помилки).

## Завдання 7б. Дескриптори процесу (`fd_observer.c`)

```bash
gcc -Wall -Wextra fd_observer.c -o fd_observer && ./fd_observer &
ls -l /proc/<PID>/fd
```
```text
lrwx------ 1 codespace codespace 64 Nov 22 05:23 0 -> /dev/pts/0
lrwx------ 1 codespace codespace 64 Nov 22 05:23 1 -> /dev/pts/0
lrwx------ 1 codespace codespace 64 Nov 22 05:23 2 -> /dev/pts/0
lrwx------ 1 codespace codespace 64 Nov 22 05:23 26 -> /dev/pts/ptmx
```
Дескриптори 0, 1, 2 вказують на термінал (або на файл/pipe при перенаправленні).

## Завдання 8. `open()` (`open_file.c`)

```bash
gcc -Wall -Wextra open_file.c -o open_file
./open_file &
ls -l /proc/<PID>/fd
```
```text
lrwx------ 1 codespace codespace 64 Nov 22 05:25 26 -> /dev/pts/ptmx
l-wx------ 1 codespace codespace 64 Nov 22 05:25 27 -> /home/codespace/.vscode-remote/data/logs/20261122T044458/mcpGateway.log
l-wx------ 1 codespace codespace 64 Nov 22 05:25 3 -> /workspaces/labs-Arhitektura-Derehlazova/lab06-memory-io/example.txt
```
**Чому file descriptor = 3?** `open()` повертає найменший вільний номер. Номери 0, 1, 2 вже зайняті stdin/stdout/stderr, тому наступний вільний це 3.

**Pathname та file descriptor.** Pathname (`example.txt`) це ім'я файла в файловій системі; його використовують лише в `open()`. File descriptor це числовий дескриптор відкритого файла в межах процесу, який використовують у `read()`, `write()`, `close()`. Після `open()` ядро працює вже з дескриптором, а не з іменем.

## Завдання 9. `write()` (`write_file.c`)

```text
Requested: 19 bytes
Written:   19 bytes
```
і `cat output.txt` показує `Hello from write()`. **Data flow:** рядок у пам'яті процесу → `write(fd, ...)` → ядро → файл на диску.

**`write()` може записати менше, ніж просили.** Це partial write (наприклад, при переповненні pipe, сигналі або обмеженнях ресурсів), тому правильна програма перевіряє повернуте значення й дописує залишок у циклі.

## Завдання 10. `read()` (`read_file.c`)

```bash
gcc -Wall -Wextra read_file.c -o read_file && ./read_file
```
```text
Hello from input file
Second line
```
Читання відбувається шматками по 16 байтів. `read()` повертає **кількість прочитаних байтів**, `0` означає кінець файла (EOF), `-1` це помилка.

## Завдання 11–12. Offset та `lseek()` (`file_offset.c`)

```bash
gcc -Wall -Wextra file_offset.c -o file_offset && ./file_offset
gcc -Wall -Wextra -DUSE_LSEEK file_offset.c -o file_offset_lseek && ./file_offset_lseek
```
Для `input.txt` з вмістом `Hello from input file...`:
```text
# без lseek                   # з lseek(fd, 0, SEEK_SET)
First:  [Hello]                First:  [Hello]
Second: [ from]                Second: [Hello]
```
Після першого `read()` **file offset** (поточна позиція в файлі) зсувається на 5 байтів, тому наступний `read()` продовжує звідти (`" from"`). `lseek(fd, 0, SEEK_SET)` повертає offset на початок, і другий `read()` знову читає `Hello`.

**Sequential та random access:** sequential це читання підряд, від початку до кінця, а random access це читання з довільної позиції (через `lseek`).

## Завдання 13. `strace` для I/O

```bash
strace -e trace=openat,read,write,close ./read_file
```
```text
openat(AT_FDCWD, "input.txt", O_RDONLY) = 3
read(3, "Hello from input", 16)         = 16
write(1, "Hello from input", 16Hello from input)        = 16
read(3, " file\nSecond lin", 16)        = 16
write(1, " file\nSecond lin", 16 file
Second lin)       = 16
read(3, "e\n", 16)                      = 2
write(1, "e\n", 2e
)                      = 2
read(3, "", 16)                         = 0
close(3)                                = 0
```

## Завдання 14. `stdout` та `stderr` (`streams.c`)

```bash
gcc -Wall -Wextra streams.c -o streams
./streams
./streams > out.txt
./streams 2> err.txt
./streams > out.txt 2> err.txt
```
```text
./streams > out.txt
./streams 2> err.txt
./streams > out.txt 2> err.txt
```
`> out.txt` перенаправляє лише дескриптор 1 (stdout), а повідомлення в stderr (2) лишається на екрані; `2> err.txt` навпаки. Redirection виконує shell, **не змінюючи код програми**: shell підміняє дескриптори 1 або 2 перед запуском програми, а програма й далі пише у «дескриптор 1».

## Завдання 15–16. Буферизація (`buffering.c`)

```bash
gcc -Wall -Wextra buffering.c -o buffering
./buffering | cat
strace -e write ./buffering | cat
```
```text
DABC
```
(у файл/pipe `printf` накопичує `ABC` у буфері й виводить при завершенні програми, а `write()` виводить `D` одразу). У терміналі stdout буферизується по рядках, тому результат може бути іншим; для порівняння використовуйте pipe `| cat`.

З `fflush(stdout)` перед `write()`:
```bash
gcc -Wall -Wextra -DUSE_FFLUSH buffering.c -o buffering_flush
./buffering_flush | cat
```
```text
ABCD
```
`fflush(stdout)` змушує бібліотеку негайно записати вміст буфера (виконати `write`), тому порядок збігається з порядком у коді.

`write(1, "D", 1D)                        = 1
write(1, "ABC", 3)                      = 3
ABC+++ exited with 0 +++
write(1, "ABC", 3ABC)                      = 3
write(1, "D", 1D)                        = 1
+++ exited with 0 +++`

**Чому порядок може здивувати:** `printf()` працює з бібліотечним буфером у user space, а `write()` одразу викликає ядро.

## Фінальне завдання. `file_copy.c`

```bash
gcc -Wall -Wextra file_copy.c -o file_copy
printf 'Operating Systems\nComputer Architecture\n' > source.txt
./file_copy source.txt destination.txt
cmp source.txt destination.txt ; echo $?          # 0 = файли однакові
./file_copy file_copy copy_of_file_copy
cmp file_copy copy_of_file_copy ; echo $?          # 0 = бінарні файли однакові
./file_copy                                        # помилка: неправильна кількість аргументів
./file_copy no_such_file x.txt                     # помилка: source не існує
```
```text
cmp: 0
cmp binary: 0
Usage: ./file_copy <source> <destination>
open source: No such file or directory
```

**Реалізація:** `open()` source (`O_RDONLY`) → `open()` destination (`O_WRONLY | O_CREAT | O_TRUNC`, права `0644`) → цикл `read()` у буфер 4096 байтів → внутрішній цикл `write()` до повного запису (partial write) → `close()` обох. Заборонені `fopen/fread/fwrite/system("cp")`. Обробляються помилки: кількість аргументів, `open` source/destination (при невдалому відкритті destination source закривається), `read`, `write`, `close`, а також `EINTR`.

**Чому один цикл копіює і text, і binary?** На рівні `read()/write()` програма працює з **байтами**, а не з «текстом». Текст і executable для неї лише послідовності байтів.

### `strace` для `file_copy`
```bash
strace -e trace=openat,read,write,close ./file_copy source.txt destination.txt
```
```text
openat(AT_FDCWD, "source.txt", O_RDONLY) = 3
openat(AT_FDCWD, "destination.txt", O_WRONLY|O_CREAT|O_TRUNC, 0644) = 4
read(3, "Operating Systems\nComputer Archi"..., 4096) = 40
write(4, "Operating Systems\nComputer Archi"..., 40) = 40
read(3, "", 4096) = 0
close(3) = 0
close(4) = 0
```
Зіставлення з кодом: `openat` це наші два `open()`, `read` та `write` це цикл копіювання, `read(...) = 0` це кінець файла (вихід із циклу), два `close` це закриття дескрипторів.

## Повний data path

```text
SOURCE FILE
     │ read()
     ▼
══════ Kernel ══════
     │
     ▼
Process Virtual Memory
     │  buffer[4096]
     ▼
══════ Kernel ══════
     │ write()
     ▼
DESTINATION FILE
```

## Інтеграційне питання: `read(fd, buffer, sizeof(buffer))`

`buffer` це масив у **віртуальній пам'яті** процесу; його virtual address MMU перекладає у фізичну адресу через таблицю сторінок (page). Програма викликає `read()`, тобто **системний виклик**, і керування переходить у ядро. Ядро за **file descriptor** `fd` знаходить відкритий файл процесу, виконує I/O та **копіює дані у user buffer** процесу (у його віртуальну пам'ять). Потім управління повертається в програму, а `read()` повертає кількість прочитаних байтів. Тобто: **fd** це дескриптор відкритого файла в процесі, **system call** це межа переходу в ядро, **virtual memory** це місце, куди ядро кладе дані.

## Відповіді на контрольні питання

- **Virtual address:** адреса, яку бачить процес у своєму адресному просторі.
- **Відмінність від physical address:** physical address це реальне місце в RAM; MMU перетворює virtual на physical.
- **Virtual address space:** увесь діапазон віртуальних адрес процесу.
- **Однакова virtual address у двох процесів:** у кожного свій адресний простір і своя таблиця відображення, тому та сама virtual address вказує на різну фізичну пам'ять.
- **Page:** блок віртуальної пам'яті фіксованого розміру (зазвичай 4096 байтів).
- **Page frame:** блок фізичної пам'яті такого ж розміру.
- **Page table:** таблиця відповідності віртуальних сторінок фізичним frame.
- **MMU:** апаратний блок, що транслює віртуальні адреси у фізичні та перевіряє права доступу.
- **TLB:** кеш останніх трансляцій адрес.
- **TLB miss та page fault:** TLB miss, коли трансляції немає в кеші (її беруть з page table); page fault, коли сторінки немає в пам'яті або доступ недозволений, і керування отримує ОС.
- **Page fault:** виняток при зверненні до сторінки, що не готова (не в RAM, не відображена).
- **Чи завжди помилка?** Ні, звичайний page fault (завантаження сторінки, copy-on-write, перше звернення до виділеної пам'яті) це нормальний механізм.
- **Ізоляція процесів:** у кожного процесу свій адресний простір, і доступ до чужих сторінок неможливий.
- **r, w, x:** права на читання, запис і виконання.
- **Чому code не writable:** захист від випадкової або зловмисної зміни інструкцій.
- **`malloc()` та RAM:** `malloc` резервує віртуальні адреси, а фізична пам'ять виділяється при першому зверненні до сторінок.
- **File descriptor:** невід'ємне ціле число, що ідентифікує відкритий файл (або pipe, сокет) у процесі.
- **Pathname та fd:** pathname це ім'я в файловій системі, fd це дескриптор відкритого файла в процесі.
- **0, 1, 2:** stdin, stdout, stderr.
- **`open()`:** відкриває файл і повертає file descriptor (або -1).
- **`read()` повертає:** кількість прочитаних байтів, 0 при EOF, -1 при помилці.
- **`read() == 0`:** кінець файла.
- **`write()` повертає:** кількість записаних байтів або -1.
- **Partial writes:** `write()` може записати менше, ніж просили, тому треба дописувати залишок.
- **File offset:** поточна позиція читання/запису у відкритому файлі.
- **`lseek()`:** змінює offset у відкритому файлі.
- **`FILE *` та fd:** `FILE *` це буферизований потік бібліотеки C поверх дескриптора, fd це низькорівневий дескриптор ОС.
- **Чому `printf` не завжди викликає `write`:** він буферизує вивід і викликає `write` пізніше (по заповненню буфера, на новому рядку в терміналі, при `fflush` або завершенні).
- **Buffering:** накопичення даних у пам'яті, щоб зменшити кількість системних викликів.
- **`fflush()`:** записує вміст буфера негайно.
- **Чому `>` не потребує змін у програмі:** shell підміняє дескриптор 1 до запуску програми, а програма пише в fd 1 як завжди.
- **Чому `file_copy` копіює text і binary:** це лише байти.
- **Де buffer під час `read()`:** у віртуальній пам'яті процесу (user space).
- **Як взаємодіють fd, virtual memory та system call:** `read()` це system call, ядро за fd знаходить файл і копіює дані у віртуальну пам'ять процесу (buffer).

## Висновок

Підтверджено, що адреси процесу групуються за областями (code, data, heap, stack) і відображаються в `/proc/PID/maps` з правами `r/w/x`. `malloc` резервує віртуальну пам'ять, а фізична з'являється при зверненні (`VmRSS`). Некоректний доступ завершується `SIGSEGV` завдяки захисту пам'яті. File descriptor це ручка відкритого файла (0, 1, 2 це стандартні потоки), `read`/`write` працюють з байтами через буфер у віртуальній пам'яті, `printf` буферизує, а `fflush` скидає буфер. `file_copy` об'єднує віртуальну пам'ять та файловий I/O.
